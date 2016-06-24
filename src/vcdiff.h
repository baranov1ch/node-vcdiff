// node-vcdiff
// https://github.com/baranov1ch/node-vcdiff
//
// Copyright 2014 Alexey Baranov <me@kotiki.cc>
// Released under the MIT license

#ifndef VCDIFF_H_
#define VCDIFF_H_

#include <memory>
#include <string>

#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>

namespace open_vcdiff {
class OutputStringInterface;
}

class VcdCtx : public node::ObjectWrap {
 public:
  enum class Mode {
    ENCODE,
    DECODE,
  };

  enum class Error {
    OK,
    INIT_ERROR,
    ENCODE_ERROR,
    DECODE_ERROR,
  };

  class Coder {
   public:
    virtual Error Start(open_vcdiff::OutputStringInterface* out) = 0;
    virtual Error Process(const char* data,
                          size_t len,
                          open_vcdiff::OutputStringInterface* out) = 0;
    virtual Error Finish(open_vcdiff::OutputStringInterface* out) = 0;
    
    virtual ~Coder() {}
  };

  VcdCtx(std::unique_ptr<Coder> coder);
  virtual ~VcdCtx();

  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteAsync(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteSync(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Close(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  enum class State {
    IDLE,
    PROCESSING,
    FINALIZING,
    DONE,
  };

  struct WorkData {
    VcdCtx* ctx;
    v8::Isolate* isolate;
    const char* data;
    size_t len;
    bool is_last;
  };

  static void WriteInternal(
      const v8::FunctionCallbackInfo<v8::Value>& args, bool async);
  v8::Local<v8::Object> Write(
      v8::Local<v8::Object> buffer, bool is_last, bool async);
  v8::Local<v8::Array> FinishWrite(v8::Isolate* isolate);
  void Process(const char* data, size_t len, bool is_last);
  bool CheckError(v8::Isolate* isolate);
  void SendError(v8::Isolate* isolate);
  void Close();
  void Reset();
  bool HasError() const;
  v8::Local<v8::Object> GetOutputBuffer(v8::Isolate* isolate);

  static const char* GetErrorString(Error err);
  static void ProcessShim(uv_work_t* work_req);
  static void AfterShim(uv_work_t* work_req, int status);

  std::unique_ptr<Coder> coder_;

  v8::Persistent<v8::Object> in_buffer_; // hold reference when async

  uv_work_t work_req_;
  bool write_in_progress_ = false;
  bool pending_close_ = false;
  State state_ = State::IDLE;
  Error err_ = Error::OK;
  std::string output_buffer_;

  VcdCtx(const VcdCtx& other) = delete;
  VcdCtx& operator=(const VcdCtx& other) = delete;
};

#endif  // VCDIFF_H_
