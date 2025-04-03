#include <sys/types.h>
#include "./memory.hh"
#include <napi.h>


static Napi::Object Init(Napi::Env env, Napi::Object exports) {

  exports.Set(Napi::String::New(env, "setMemory"),
              Napi::Function::New(env, SharedMemory::set_memory));
  exports.Set(Napi::String::New(env, "getMemory"),
              Napi::Function::New(env, SharedMemory::get_memory));
  exports.Set(Napi::String::New(env, "removeMemory"),
              Napi::Function::New(env, SharedMemory::remove_memory));

  return exports;
}

NODE_API_MODULE(cmnative, Init)