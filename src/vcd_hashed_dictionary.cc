// node-vcdiff
// https://github.com/baranov1ch/node-vcdiff
//
// Copyright 2014 Alexey Baranov <me@kotiki.cc>
// Released under the MIT license

#include "vcd_hashed_dictionary.h"

#include <node_buffer.h>

#include "third-party/open-vcdiff/src/google/vcencoder.h"

v8::Persistent<v8::Function> VcdHashedDictionary::constructor;

VcdHashedDictionary::VcdHashedDictionary(
    std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dictionary)
    : hashed_dictionary_(std::move(hashed_dictionary)) {
}

VcdHashedDictionary::~VcdHashedDictionary() {
}

// static
void VcdHashedDictionary::Init(v8::Handle<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();

  v8::Local<v8::String> className = v8::String::NewFromUtf8(isolate, "HashedDictionary", v8::String::kInternalizedString);
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(className);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  exports->Set(className, tpl->GetFunction());
}

// static
void VcdHashedDictionary::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  assert(args.Length() == 1 && "new HashedDictionary(buffer)");
  assert(node::Buffer::HasInstance(args[0]) &&
         "should pass Buffer to constructor");

  v8::Isolate *isolate = args.GetIsolate();

  std::unique_ptr<open_vcdiff::HashedDictionary> dictionary(
      new open_vcdiff::HashedDictionary(node::Buffer::Data(args[0]),
                                        node::Buffer::Length(args[0])));
  if (!dictionary->Init()) {
    isolate->ThrowException(v8::String::NewFromUtf8(isolate,
        "Error initializing hashed dictionary"));
    return;
  }
  auto vcd_hashed_dict = new VcdHashedDictionary(std::move(dictionary));
  vcd_hashed_dict->Wrap(args.This());
}
