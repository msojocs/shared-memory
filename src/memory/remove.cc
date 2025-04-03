#include "napi.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <iostream>

namespace SharedMemory {

    using namespace boost::interprocess;
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info) {
        spdlog::info("Remove memory call.");
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
            
            spdlog::info("Remove mutex.");
            // 尝试删除互斥锁
            try {
                named_mutex::remove((key + "_mutex").c_str());
                spdlog::info("Mutex removed successfully.");
            } catch (const std::exception& e) {
                spdlog::warn("Failed to remove mutex: {}", e.what());
                // 忽略互斥锁删除错误
            }
            
            spdlog::info("Remove shared memory.");
            // 尝试删除共享内存
            bool removed = shared_memory_object::remove(key.c_str());
            spdlog::info("Shared memory removed: {}", removed);
            
            return Napi::Boolean::New(env, removed);
        } catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
            throw Napi::Error::New(env, std::string("删除共享内存失败: ") + e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred");
            throw Napi::Error::New(env, "删除共享内存时发生未知错误");
        }
    }
} 