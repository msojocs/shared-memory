#include "napi.h"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <iostream>

namespace SharedMemory {

    using namespace boost::interprocess;
    Napi::Uint8Array get_memory(const Napi::CallbackInfo &info) {
        spdlog::info("Get memory call.");
        auto env = info.Env();
        
        try {
            if (info.Length() != 1) {
                throw std::runtime_error("参数长度必须为1!");
            }
            auto keyValue = info[0];
            if (!keyValue.IsString()) {
                throw std::runtime_error("参数必须是字符串!");
            }
            
            spdlog::info("Read arguments.");
            auto key = keyValue.As<Napi::String>().Utf8Value();
            
            spdlog::info("Open shared memory object.");
            // 打开共享内存对象
            shared_memory_object shm(open_only, key.c_str(), read_write);
            
            spdlog::info("Get size.");
            // 获取共享内存大小
            offset_t size;
            shm.get_size(size);
            
            spdlog::info("Map region.");
            // 映射整个共享内存区域
            mapped_region region(shm, read_write);
            void* shared_memory = region.get_address();
            
            spdlog::info("Create mutex.");
            // 使用互斥锁保护共享内存访问
            named_mutex mutex(open_only, (key + "_mutex").c_str());
            scoped_lock<named_mutex> lock(mutex);
            
            spdlog::info("Create buffer.");
            // 创建一个ArrayBuffer，直接映射到共享内存
            auto deleter = [](void* /*data*/, void* /*hint*/) {
                spdlog::info("Buffer cleanup callback called.");
            };
            
            Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, shared_memory, size, deleter);
            
            spdlog::info("Create result.");
            // 从ArrayBuffer创建一个Uint8Array
            Napi::Uint8Array result = Napi::Uint8Array::New(env, size, buffer, 0);
            
            spdlog::info("Return result.");
            return result;
        } catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
            throw Napi::Error::New(env, std::string("获取共享内存失败: ") + e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred");
            throw Napi::Error::New(env, "获取共享内存时发生未知错误");
        }
    }
} 