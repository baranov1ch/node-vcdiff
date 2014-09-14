#ifndef VCD_ENCODER_H_
#define VCD_ENCODER_H_

#include <memory>

#include <v8.h>

#include "vcdiff.h"

namespace open_vcdiff {
class VCDiffStreamingEncoder;
}

class VcdEncoder : public VcdCtx::Coder {
 public:
  VcdEncoder(v8::Local<v8::Object> hashed_dictionary,
             std::unique_ptr<open_vcdiff::VCDiffStreamingEncoder> encoder);
  ~VcdEncoder();

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
  std::unique_ptr<open_vcdiff::VCDiffStreamingEncoder> encoder_;
  v8::Persistent<v8::Object> hashed_dictionary_;

  VcdEncoder(const VcdEncoder& other) = delete;
  VcdEncoder& operator=(const VcdEncoder& other) = delete;
};

#endif  // VCD_ENCODER_H_
