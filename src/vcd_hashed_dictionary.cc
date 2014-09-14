#include "vcd_hashed_dictionary.h"

#include <google/vcencoder.h>
#include <node_buffer.h>

v8::Persistent<v8::Function> VcdHashedDictionary::constructor;

VcdHashedDictionary::VcdHashedDictionary(
    std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dictionary)
    : hashed_dictionary_(std::move(hashed_dictionary)) {
}

VcdHashedDictionary::~VcdHashedDictionary() {
}

// static
void VcdHashedDictionary::Init(v8::Handle<v8::Object> exports) {
  v8::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("HashedDictionary"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
  exports->Set(v8::String::NewSymbol("HashedDictionary"), constructor);
}

// static
v8::Handle<v8::Value> VcdHashedDictionary::New(const v8::Arguments& args) {
  v8::HandleScope scope;
  assert(args.Length() == 1 && "new HashedDictionary(buffer)");
  assert(node::Buffer::HasInstance(args[0]) &&
         "should pass Buffer to constructor");
  std::unique_ptr<open_vcdiff::HashedDictionary> dictionary(
      new open_vcdiff::HashedDictionary(node::Buffer::Data(args[0]),
                                        node::Buffer::Length(args[0])));
  if (!dictionary->Init())
    return v8::ThrowException(v8::String::New(
        "Error initializing hashed dictionary"));
  auto vcd_hashed_dict = new VcdHashedDictionary(std::move(dictionary));
  vcd_hashed_dict->Wrap(args.This());
  return args.This();
}
