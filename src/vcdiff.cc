// node-vcdiff
// https://github.com/baranov1ch/node-vcdiff
//
// Copyright 2014 Alexey Baranov <me@kotiki.cc>
// Released under the MIT license

#include <memory>

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

#include "third-party/open-vcdiff/src/google/vcdecoder.h"
#include "third-party/open-vcdiff/src/google/vcencoder.h"
#include "vcd_decoder.h"
#include "vcd_encoder.h"
#include "vcd_hashed_dictionary.h"
#include "vcdiff.h"

v8::Persistent<v8::Function> VcdCtx::constructor;

VcdCtx::VcdCtx(std::unique_ptr<Coder> coder)
  : coder_(std::move(coder)) {
}

VcdCtx::~VcdCtx() {
  Reset();
}

void VcdCtx::Reset() {
  pending_close_ = false;
  write_in_progress_ = false;
  coder_.reset();
}

void VcdCtx::Close() {
  if (write_in_progress_) {
    pending_close_ = true;
    return;
  }
  Reset();
}

// static
void VcdCtx::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto mode = static_cast<Mode>(args[0]->Int32Value());
  assert((mode == Mode::ENCODE || mode == Mode::DECODE) &&
         "invalid operation mode (neither ENCODE nor DECODE");

  v8::Isolate* isolate = args.GetIsolate();

  std::unique_ptr<Coder> coder;
  if (mode == Mode::ENCODE) {
    auto hashed_dict = Unwrap<VcdHashedDictionary>(args[1]->ToObject());
    std::unique_ptr<open_vcdiff::VCDiffStreamingEncoder> encoder(
        new open_vcdiff::VCDiffStreamingEncoder(
            hashed_dict->hashed_dictionary(),
            args[3]->Uint32Value(),
            args[2]->BooleanValue()));
    coder.reset(new VcdEncoder(isolate, args[1]->ToObject(), std::move(encoder)));
  } else {
    assert(node::Buffer::HasInstance(args[1]) &&
           "Buffer required for decoder");
    std::unique_ptr<open_vcdiff::VCDiffStreamingDecoder> decoder(
        new open_vcdiff::VCDiffStreamingDecoder());
    decoder->SetAllowVcdTarget(args[2]->BooleanValue());
    decoder->SetMaximumTargetFileSize(args[3]->Uint32Value());
    decoder->SetMaximumTargetWindowSize(args[4]->Uint32Value());
    coder.reset(new VcdDecoder(isolate, args[1]->ToObject(), std::move(decoder)));
  }

  auto ctx = new VcdCtx(std::move(coder));
  ctx->Wrap(args.This());
}

// static
void VcdCtx::WriteAsync(const v8::FunctionCallbackInfo<v8::Value>& args) {
  WriteInternal(args, true);
}

// static
void VcdCtx::WriteSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
  WriteInternal(args, false);
}

// static
void VcdCtx::WriteInternal(
      const v8::FunctionCallbackInfo<v8::Value>& args, bool async) {
  v8::Isolate* isolate = args.GetIsolate();
  VcdCtx* ctx = Unwrap<VcdCtx>(args.Holder());
  assert(node::Buffer::HasInstance(args[1]));

  bool is_last = args[0]->BooleanValue();
  auto in_buf = args[1]->ToObject();
  auto result = ctx->Write(in_buf, is_last, async);

  if (result.IsEmpty()) {
    args.GetReturnValue().Set(v8::Undefined(isolate));
  } else {
    args.GetReturnValue().Set(result);
  }
}

v8::Local<v8::Object> VcdCtx::Write(
    v8::Local<v8::Object> buffer, bool is_last, bool async) {
  v8::Isolate* isolate = buffer->GetIsolate();

  assert(coder_.get() && "attempt to write after finalization");
  assert(!write_in_progress_ && "write already in progress");
  assert(!pending_close_ && "close is pending");

  write_in_progress_ = true;
  in_buffer_.Reset(isolate, buffer);
  auto data = node::Buffer::Data(buffer);
  auto len = node::Buffer::Length(buffer);

  if (!async) {
    // sync version
    v8::HandleScope handle_scope(isolate);
    Process(data, len, is_last);
    if (!CheckError(isolate))
      return v8::Local<v8::Object>();
    return FinishWrite(isolate);
  } else {
    work_req_.data = new WorkData { this, isolate, data, len, is_last };
    uv_queue_work(uv_default_loop(),
                  &work_req_,
                  ProcessShim,
                  AfterShim);
    return handle();
  }
}

v8::Local<v8::Array> VcdCtx::FinishWrite(v8::Isolate* isolate) {
  write_in_progress_ = false;
  in_buffer_.Reset();
  auto result = v8::Array::New(isolate, 2);
  result->Set(0, GetOutputBuffer(isolate));
  result->Set(1, v8::Boolean::New(isolate, state_ == State::DONE));
  output_buffer_.clear();
  return result;
}

// thread pool!
// This function may be called multiple times on the uv_work pool
// for a single write() call, until all of the input bytes have
// been consumed.
void VcdCtx::Process(const char* data, size_t len, bool is_last) {
  assert(coder_.get() && "attempt to write after finalization");
  open_vcdiff::OutputString<std::string> out(&output_buffer_);
  if (state_ == State::IDLE) {
    assert(output_buffer_.empty());
    err_ = coder_->Start(&out);
    if (err_ != Error::OK) {
      state_ = State::DONE;
      return;
    } else {
      state_ = State::PROCESSING;
    }
  }

  if (state_ == State::PROCESSING) {
    err_ = coder_->Process(data, len, &out);
    if (err_ != Error::OK) {
      state_ = State::DONE;
      return;
    } else {
      if (is_last)
        state_ = State::FINALIZING;
    }
  }

  if (state_ == State::FINALIZING) {
    err_ = coder_->Finish(&out);
    // Error will be processed anyway. Nothing to do here.
    state_ = State::DONE;
  }

}

bool VcdCtx::CheckError(v8::Isolate* isolate) {
  if (!HasError())
    return true;

  SendError(isolate);
  return false;
}

void VcdCtx::SendError(v8::Isolate* isolate) {
  v8::Local<v8::Value> args[2] = {
    v8::String::NewFromUtf8(isolate, GetErrorString(err_)),
    v8::Number::New(isolate, static_cast<int>(err_)),
  };
  node::MakeCallback(isolate, handle(), "onerror", 2, args);

  // no hope of rescue.
  write_in_progress_ = false;
  if (pending_close_)
    Close();
}

bool VcdCtx::HasError() const {
  return err_ != Error::OK;
}

v8::Local<v8::Object> VcdCtx::GetOutputBuffer(v8::Isolate* isolate) {
  // TODO: avoid copying when return data.
  auto buffer = node::Buffer::Copy(isolate,
      output_buffer_.data(), output_buffer_.size());
  return buffer.ToLocalChecked();
}

// static
void VcdCtx::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto ctx = Unwrap<VcdCtx>(args.Holder());
  ctx->Close();
  args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

// static
void VcdCtx::ProcessShim(uv_work_t* work_req) {
  auto work = static_cast<WorkData*>(work_req->data);
  work->ctx->Process(work->data, work->len, work->is_last);
}

// static
void VcdCtx::AfterShim(uv_work_t* work_req, int status) {
  assert(status == 0);

  std::unique_ptr<WorkData> data(
      static_cast<WorkData*>(work_req->data));
  v8::Isolate* isolate = data->isolate;
  v8::HandleScope handle_scope(isolate);
  VcdCtx* ctx = data->ctx;

  if (!ctx->CheckError(isolate))
    return;

  auto result = ctx->FinishWrite(isolate);
  v8::Local<v8::Value> args[2] = {
    result->Get(0),
    result->Get(1),
  };
  node::MakeCallback(isolate, ctx->handle(), "callback", 2, args);

  if (ctx->pending_close_)
    ctx->Close();
}

// static
const char* VcdCtx::GetErrorString(VcdCtx::Error err) {
  switch (err) {
    case Error::INIT_ERROR:
      return "Vcdiff initialization error";
    case Error::ENCODE_ERROR:
      return "Vcdiff encode error";
    case Error::DECODE_ERROR:
      return "Vcdiff decode error";
    default:
      return "Vcdiff unknown error";
  }
}

// static
void VcdCtx::Init(v8::Handle<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();
  v8::Local<v8::String> className = v8::String::NewFromUtf8(isolate, "Vcdiff", v8::String::kInternalizedString);

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(className);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "write", WriteAsync);
  NODE_SET_PROTOTYPE_METHOD(tpl, "writeSync", WriteSync);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);

  exports->Set(className, tpl->GetFunction());

#define NODE_SET_CONSTANT_FROM_ENUM(target, name, value) \
  (target)->ForceSet(v8::String::NewFromUtf8(isolate, #name, v8::String::kInternalizedString), \
                     v8::Number::New(isolate, static_cast<double>((value))), \
                     static_cast<v8::PropertyAttribute>( \
                         v8::ReadOnly | v8::DontDelete)); \

  NODE_SET_CONSTANT_FROM_ENUM(exports, ENCODE, Mode::ENCODE);
  NODE_SET_CONSTANT_FROM_ENUM(exports, DECODE, Mode::DECODE);
  NODE_SET_CONSTANT_FROM_ENUM(exports, INIT_ERROR, Error::INIT_ERROR);
  NODE_SET_CONSTANT_FROM_ENUM(exports, ENCODE_ERROR, Error::ENCODE_ERROR);
  NODE_SET_CONSTANT_FROM_ENUM(exports, DECODE_ERROR, Error::DECODE_ERROR);
  NODE_SET_CONSTANT_FROM_ENUM(exports,
                              VCD_STANDARD_FORMAT,
                              open_vcdiff::VCD_STANDARD_FORMAT);
  NODE_SET_CONSTANT_FROM_ENUM(exports,
                              VCD_FORMAT_INTERLEAVED,
                              open_vcdiff::VCD_FORMAT_INTERLEAVED);
  NODE_SET_CONSTANT_FROM_ENUM(exports,
                              VCD_FORMAT_CHECKSUM,
                              open_vcdiff::VCD_FORMAT_CHECKSUM);
  NODE_SET_CONSTANT_FROM_ENUM(exports,
                              VCD_FORMAT_JSON,
                              open_vcdiff::VCD_FORMAT_JSON);
#undef NODE_SET_CONSTANT_FROM_ENUM
}

void InitVcdiff(v8::Handle<v8::Object> exports) {
  VcdCtx::Init(exports);
  VcdHashedDictionary::Init(exports);
}

NODE_MODULE(vcdiff, InitVcdiff)
