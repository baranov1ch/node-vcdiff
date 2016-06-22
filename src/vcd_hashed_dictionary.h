// node-vcdiff
// https://github.com/baranov1ch/node-vcdiff
//
// Copyright 2014 Alexey Baranov <me@kotiki.cc>
// Released under the MIT license

#ifndef VCD_HASHED_DICTIONARY_H_
#define VCD_HASHED_DICTIONARY_H_

#include <memory>

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

namespace open_vcdiff {
class HashedDictionary;
}

class VcdHashedDictionary : public node::ObjectWrap {
 public:
  VcdHashedDictionary(
      std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dictionary);
  virtual ~VcdHashedDictionary();

  open_vcdiff::HashedDictionary* hashed_dictionary() {
    return hashed_dictionary_.get();
  }

  static void Init(v8::Handle<v8::Object> exports);

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static v8::Persistent<v8::Function> constructor;

  std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dictionary_;

  VcdHashedDictionary(const VcdHashedDictionary& other) = delete;
  VcdHashedDictionary& operator=(const VcdHashedDictionary& other) = delete;
};

#endif  // VCD_HASHED_DICTIONARY_H_
