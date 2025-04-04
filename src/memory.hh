#pragma once

#ifndef MEMORY_HH
#define MEMORY_HH
#include "napi.h"
#include <memory>
#include <string>
#include <map>
#include <mutex>

// 平台特定的头文件
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#endif

namespace SharedMemory {
    // 全局回调函数
    extern Napi::FunctionReference console_callback;

    // 日志辅助函数
    void log(const std::string& message);
    void log(const char* format, ...);

    // 清理控制台回调函数
    void cleanup_console();

    // 共享内存头部结构
    struct SharedMemoryHeader {
        size_t size;          // 用户数据大小
        int version;          // 版本号
    };

    // 共享内存地址管理器
    class AddressManager {
    public:
        static AddressManager& getInstance() {
            static AddressManager instance;
            return instance;
        }

        void* getAddress(const std::string& key) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = addresses_.find(key);
            if (it != addresses_.end()) {
                return it->second;
            }
            return nullptr;
        }

        void setAddress(const std::string& key, void* addr) {
            std::lock_guard<std::mutex> lock(mutex_);
            addresses_[key] = addr;
        }

        void removeAddress(const std::string& key) {
            std::lock_guard<std::mutex> lock(mutex_);
            addresses_.erase(key);
        }

    private:
        AddressManager() {}
        std::map<std::string, void*> addresses_;
        std::mutex mutex_;
    };

    // 共享内存管理器类
    class SharedMemoryManager : public std::enable_shared_from_this<SharedMemoryManager> {
    public:
        // 构造函数
        SharedMemoryManager(const std::string& key, bool create = false, size_t size = 0);
        
        // 析构函数
        ~SharedMemoryManager();
        
        // 获取共享内存地址
        void* get_address() const { return address_; }
        
        // 获取共享内存大小
        size_t get_size() const { return size_; }
        
        // 获取文件路径
        const std::string& get_file_path() const { return file_path_; }
        
        // 获取版本号
        int get_version() const { 
            if (address_) {
                return static_cast<SharedMemoryHeader*>(address_)->version;
            }
            return 0;
        }
        
    private:
        std::string key_;           // 共享内存键名
        size_t size_;               // 数据区大小
        void* address_;             // 共享内存地址
        std::string file_path_;     // 文件路径

#ifdef _WIN32
        HANDLE file_mapping_;       // 文件映射句柄
        HANDLE mutex_;              // 互斥锁句柄
        
        // 创建文件映射
        bool create_mapping(HANDLE file_handle, size_t mapping_size);
#else
        sem_t* mutex_;              // 互斥锁
#endif
    };

    /**
     * 设置控制台回调函数
     * @param info 回调信息
     * @return undefined
     */
    Napi::Value set_console(const Napi::CallbackInfo &info);

    /**
     * 设置共享内存 
     * @param info 回调信息
     * @return 共享内存的视图
     */
    Napi::Value set_memory(const Napi::CallbackInfo &info);

    /**
     * 获取共享内存
     * @param info 回调信息
     * @return 共享内存的视图
     */
    Napi::Value get_memory(const Napi::CallbackInfo &info);

    /**
     * 删除共享内存
     * @param info 回调信息
     * @return 是否成功
     */
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info);
}
#endif