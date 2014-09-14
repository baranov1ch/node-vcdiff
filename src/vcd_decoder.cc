#include "vcd_decoder.h"

#include <google/vcdecoder.h>
#include <node_buffer.h>

VcdDecoder::VcdDecoder(
    v8::Local<v8::Object> dictionary_handle,
    std::unique_ptr<open_vcdiff::VCDiffStreamingDecoder> decoder)
    : decoder_(std::move(decoder)),
      dictionary_handle_(v8::Persistent<v8::Object>::New(dictionary_handle)),
      dictionary_buffer_(node::Buffer::Data(dictionary_handle)),
      dictionary_len_(node::Buffer::Length(dictionary_handle)) {
}

VcdDecoder::~VcdDecoder() {
}

VcdCtx::Error VcdDecoder::Start(open_vcdiff::OutputStringInterface* out) {
  decoder_->StartDecoding(dictionary_buffer_, dictionary_len_);
  return VcdCtx::Error::OK;
}

VcdCtx::Error VcdDecoder::Process(
    const char* data,
    size_t len,
    open_vcdiff::OutputStringInterface* out) {
  if (!decoder_->DecodeChunkToInterface(data, len, out))
    return VcdCtx::Error::DECODE_ERROR;
  return VcdCtx::Error::OK;
}

VcdCtx::Error VcdDecoder::Finish(open_vcdiff::OutputStringInterface* out) {
  if (!decoder_->FinishDecoding())
    return VcdCtx::Error::DECODE_ERROR;
  return VcdCtx::Error::OK;
}