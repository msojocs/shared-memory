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
    
    Napi::Uint8Array set_memory(const Napi::CallbackInfo &info) {
        spdlog::info("Set memory call.");
        auto env = info.Env();
        
        try {
            if (info.Length() != 2) {
                throw std::runtime_error("参数长度必须为2!");
            }
            auto keyValue = info[0];
            if (!keyValue.IsString()) {
                throw std::runtime_error("第一个参数必须是字符串!");
            }
            auto lengthValue = info[1];
            if (!lengthValue.IsNumber()) {
                throw std::runtime_error("第二个参数必须是数字!");
            }
            
            spdlog::info("Read arguments.");
            auto key = keyValue.As<Napi::String>().Utf8Value();
            int length = lengthValue.As<Napi::Number>().Int32Value();
            
            spdlog::info("Remove old segment if exists.");
            try {
                shared_memory_object::remove(key.c_str());
                named_mutex::remove((key + "_mutex").c_str());
            } catch (const std::exception& e) {
                spdlog::warn("移除旧共享内存时出错: {}", e.what());
            }
            
            spdlog::info("Create shared memory object.");
            // 创建共享内存对象
            g_shm = std::make_shared<shared_memory_object>(create_only, key.c_str(), read_write);
            g_shm->truncate(length);
            
            spdlog::info("Map region.");
            // 映射整个共享内存区域
            g_region = std::make_shared<mapped_region>(*g_shm, read_write);
            void* shared_memory = g_region->get_address();
            
            // 初始化内存
            spdlog::info("Initialize memory.");
            std::memset(shared_memory, 0, length);
            
            spdlog::info("Create mutex.");
            // 创建互斥锁
            g_mutex = std::make_shared<named_mutex>(create_only, (key + "_mutex").c_str());
            
            spdlog::info("Create buffer.");
            // 创建一个ArrayBuffer，直接映射到共享内存
            auto deleter = [](void* /*data*/, void* /*hint*/) {
                spdlog::info("Buffer cleanup callback called.");
                // 这里不需要做任何事情，因为共享内存和互斥锁会在程序退出时自动清理
            };
            
            Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, shared_memory, length, deleter);
            
            spdlog::info("Create result.");
            // 从ArrayBuffer创建一个Uint8Array
            Napi::Uint8Array result = Napi::Uint8Array::New(env, length, buffer, 0);
            
            spdlog::info("Return result.");
            return result;
        } catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
            throw Napi::Error::New(env, std::string("创建共享内存失败: ") + e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred");
            throw Napi::Error::New(env, "创建共享内存时发生未知错误");
        }
    }
}