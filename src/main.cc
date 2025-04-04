#include <sys/types.h>
#include "./memory.hh"
#include <napi.h>

Napi::Value version(const Napi::CallbackInfo &info) {
  
  return Napi::String::New(info.Env(), "1352");
}

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "setConsole"),
              Napi::Function::New(env, SharedMemory::set_console));
  exports.Set(Napi::String::New(env, "setMemory"),
              Napi::Function::New(env, SharedMemory::set_memory));
  exports.Set(Napi::String::New(env, "getMemory"),
              Napi::Function::New(env, SharedMemory::get_memory));
  exports.Set(Napi::String::New(env, "removeMemory"),
              Napi::Function::New(env, SharedMemory::remove_memory));
  exports.Set(Napi::String::New(env, "version"),
              Napi::Function::New(env, version));

  return exports;
}

NODE_API_MODULE(cmnative, Init)