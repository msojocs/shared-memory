#include "napi.h"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <iostream>
#include <memory>

namespace SharedMemory {

    using namespace boost::interprocess;
    
    // 全局变量来保存共享内存资源
    static std::shared_ptr<shared_memory_object> g_shm;
    static std::shared_ptr<mapped_region> g_region;
    static std::shared_ptr<named_mutex> g_mutex;
    
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
            g_shm = std::make_shared<shared_memory_object>(open_only, key.c_str(), read_write);
            
            spdlog::info("Get size.");
            // 获取共享内存大小
            offset_t size;
            g_shm->get_size(size);
            
            spdlog::info("Map region.");
            // 映射整个共享内存区域
            g_region = std::make_shared<mapped_region>(*g_shm, read_write);
            void* shared_memory = g_region->get_address();
            
            spdlog::info("Create mutex.");
            // 使用互斥锁保护共享内存访问
            g_mutex = std::make_shared<named_mutex>(open_only, (key + "_mutex").c_str());
            scoped_lock<named_mutex> lock(*g_mutex);
            
            spdlog::info("Create buffer.");
            // 创建一个ArrayBuffer，直接映射到共享内存
            auto deleter = [](void* /*data*/, void* /*hint*/) {
                spdlog::info("Buffer cleanup callback called.");
                // 这里不需要做任何事情，因为共享内存和互斥锁会在程序退出时自动清理
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