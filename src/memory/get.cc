#include "napi.h"
#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <iostream>

namespace SharedMemory {
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
            
            // 使用 SharedMemoryManager 打开和管理共享内存
            auto manager = std::make_shared<SharedMemoryManager>(key, false);
            
            spdlog::info("Create buffer.");
            // 创建一个ArrayBuffer，直接映射到共享内存
            auto deleter = [manager](void* /*data*/, void* /*hint*/) {
                spdlog::info("Buffer cleanup callback called.");
            };
            
            Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, manager->get_address(), manager->get_size(), deleter);
            
            spdlog::info("Create result.");
            // 从ArrayBuffer创建一个Uint8Array
            Napi::Uint8Array result = Napi::Uint8Array::New(env, manager->get_size(), buffer, 0);
            
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