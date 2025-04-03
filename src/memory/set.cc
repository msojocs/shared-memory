#include "napi.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "../memory.hh"

namespace SharedMemory {

    using namespace boost::interprocess;
    Napi::Uint8Array set_memory(const Napi::CallbackInfo &info) {
        auto env = info.Env();
        if (info.Length() != 2) {
          throw Napi::Error::New(env, "参数长度必须为2!");
        }
        auto keyValue = info[0];
        if (!keyValue.IsString()) {
          throw Napi::Error::New(env, "第一个参数必须是字符串!");
        }
        auto lengthValue = info[1];
        if (!lengthValue.IsNumber()) {
          throw Napi::Error::New(env, "第二个参数必须是数字!");
        }
        
        auto key = keyValue.As<Napi::String>().Utf8Value();
        int length = lengthValue.As<Napi::Number>().Int32Value();
        
        // 创建或打开托管共享内存
        managed_shared_memory segment(open_or_create, key.c_str(), length);
        
        // 获取共享内存指针
        void* shared_memory = segment.get_address();
        
        // 使用互斥锁保护共享内存访问
        named_mutex mutex(open_or_create, (key + "_mutex").c_str());
        scoped_lock<named_mutex> lock(mutex);
        
        // 创建一个ArrayBuffer，直接映射到共享内存
        Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, shared_memory, length, [](void* data, void* hint) {
            // 这个回调函数在ArrayBuffer被垃圾回收时调用
            // 我们不需要在这里做任何事情，因为共享内存是由boost::interprocess管理的
        });
        
        // 从ArrayBuffer创建一个Uint8Array
        Napi::Uint8Array result = Napi::Uint8Array::New(env, length, buffer, 0);
        
        return result;
    }
}