// node-vcdiff
// https://github.com/baranov1ch/node-vcdiff
//
// Copyright 2014 Alexey Baranov <me@kotiki.cc>
// Released under the MIT license

#include "vcd_decoder.h"

#include <node_buffer.h>

#include "third-party/open-vcdiff/src/google/vcdecoder.h"

VcdDecoder::VcdDecoder(
    v8::Isolate* isolate,
    v8::Local<v8::Object> dictionary_handle,
    std::unique_ptr<open_vcdiff::VCDiffStreamingDecoder> decoder)
    : decoder_(std::move(decoder)),
      dictionary_handle_(isolate, dictionary_handle),
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
