#ifndef MEMORY_HH
#define MEMORY_HH
#include "napi.h"
#include <windows.h>
#include <memory>
#include <string>
#include <map>

namespace SharedMemory {
    // 全局回调函数
    extern Napi::FunctionReference console_callback;

    // 日志辅助函数
    void log(const std::string& message);
    void log(const char* format, ...);

    // 共享内存头部结构
    struct SharedMemoryHeader {
        size_t size;          // 用户数据大小
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

    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& key, bool create, size_t size = 0);
        ~SharedMemoryManager();

        void* get_address() const { return address_; }
        void* get_data_address() const { return static_cast<char*>(address_) + sizeof(SharedMemoryHeader); }
        size_t get_size() const { return size_; }  // 返回用户请求的大小
        
    private:
        HANDLE file_mapping_;
        HANDLE mutex_;
        void* address_;
        size_t size_;          // 用户请求的大小
        size_t actual_size_;   // 实际分配的大小（页面对齐）
        std::string key_;
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