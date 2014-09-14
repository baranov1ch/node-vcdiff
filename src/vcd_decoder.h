#ifndef VCD_DECODER_H_
#define VCD_DECODER_H_

#include <memory>

#include <v8.h>

#include "vcdiff.h"

namespace open_vcdiff {
class VCDiffStreamingDecoder;
}

class VcdDecoder : public VcdCtx::Coder {
 public:
  VcdDecoder(v8::Local<v8::Object> dictionary_handle,
             std::unique_ptr<open_vcdiff::VCDiffStreamingDecoder> decoder);
  ~VcdDecoder();

  // VcdCtx::Coder implementation:
  virtual VcdCtx::Error Start(
      open_vcdiff::OutputStringInterface* out) override;
  virtual VcdCtx::Error Process(
      const char* data,
      size_t len,
      open_vcdiff::OutputStringInterface* out) override;
  virtual VcdCtx::Error Finish(
      open_vcdiff::OutputStringInterface* out) override;

 private:
  std::unique_ptr<open_vcdiff::VCDiffStreamingDecoder> decoder_;

  // Store it to prevent GCing.
  v8::Persistent<v8::Object> dictionary_handle_;
  const char* dictionary_buffer_;
  size_t dictionary_len_;

  VcdDecoder(const VcdDecoder& other) = delete;
  VcdDecoder& operator=(const VcdDecoder& other) = delete;
};

#endif  // VCD_DECODER_H_
