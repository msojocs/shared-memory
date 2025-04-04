#include "napi.h"
#include <windows.h>
#include "../memory.hh"
#include <cstring>
#include <memory>

namespace SharedMemory {
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        
        // 参数检查
        if (info.Length() < 1) {
            throw Napi::Error::New(env, "需要一个参数: key");
        }
        
        if (!info[0].IsString()) {
            throw Napi::Error::New(env, "参数必须是字符串类型的key");
        }
        
        std::string key = info[0].As<Napi::String>().Utf8Value();
        
        try {
            log("Remove memory call.");
            log("Read arguments.");
            
            // 从全局管理器中移除地址
            AddressManager::getInstance().removeAddress(key);
            
            // 尝试打开共享内存
            HANDLE hMapFile = OpenFileMappingA(
                FILE_MAP_ALL_ACCESS,  // 读写权限
                FALSE,               // 不继承句柄
                ("SharedMemory_" + key).c_str()  // 共享内存名称
            );
            
            if (hMapFile != NULL) {
                // 关闭句柄，这会在最后一个引用被关闭时自动删除共享内存
                CloseHandle(hMapFile);
                log("Closed file mapping handle");
            }
            
            // 尝试删除互斥锁
            std::string mutex_name = key + "_mutex";
            HANDLE hMutex = OpenMutexA(
                DELETE,              // 请求删除权限
                FALSE,              // 不继承句柄
                mutex_name.c_str()  // 互斥锁名称
            );
            
            if (hMutex != NULL) {
                CloseHandle(hMutex);
                log("Closed mutex handle");
            }
            
            log("Shared memory removed: key=%s", key.c_str());
            
            return Napi::Boolean::New(env, true);
            
        } catch (const std::exception& e) {
            log("Error: %s", e.what());
            throw Napi::Error::New(env, e.what());
        } catch (...) {
            log("Unknown error occurred");
            throw Napi::Error::New(env, "删除共享内存时发生未知错误");
        }
    }
} 