#ifndef VCD_HASHED_DICTIONARY_H_
#define VCD_HASHED_DICTIONARY_H_

#include <memory>

#include <node.h>
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

  // v8 bindings
  static void Init(v8::Handle<v8::Object> exports);
  static v8::Persistent<v8::Function> constructor;
  static v8::Handle<v8::Value> New(const v8::Arguments& args);

 private:
  std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dictionary_;

  VcdHashedDictionary(const VcdHashedDictionary& other) = delete;
  VcdHashedDictionary& operator=(const VcdHashedDictionary& other) = delete;
};

#endif  // VCD_HASHED_DICTIONARY_H_
