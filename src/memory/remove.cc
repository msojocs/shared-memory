#include "napi.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "../memory.hh"

namespace SharedMemory {

    using namespace boost::interprocess;
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info) {
        auto env = info.Env();
        if (info.Length() != 1) {
          throw Napi::Error::New(env, "参数长度必须为1!");
        }
        auto keyValue = info[0];
        if (!keyValue.IsString()) {
          throw Napi::Error::New(env, "参数必须是字符串!");
        }
        
        auto key = keyValue.As<Napi::String>().Utf8Value();
        
        try {
            // 尝试删除互斥锁
            try {
                named_mutex::remove((key + "_mutex").c_str());
            } catch (const std::exception& e) {
                // 忽略互斥锁删除错误
            }
            
            // 尝试删除共享内存
            bool removed = shared_memory_object::remove(key.c_str());
            
            return Napi::Boolean::New(env, removed);
        } catch (const std::exception& e) {
            throw Napi::Error::New(env, "无法删除共享内存: " + std::string(e.what()));
        }
    }
} 