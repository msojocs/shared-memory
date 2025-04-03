#include "napi.h"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <memory>

namespace SharedMemory {
    class SharedMemoryManager {
    public:
        SharedMemoryManager(const std::string& key, bool create, size_t size = 0);
        ~SharedMemoryManager() = default;

        void* get_address() const { return region_->get_address(); }
        size_t get_size() const { return region_->get_size(); }
        
    private:
        std::shared_ptr<boost::interprocess::shared_memory_object> shm_;
        std::shared_ptr<boost::interprocess::mapped_region> region_;
        std::shared_ptr<boost::interprocess::named_mutex> mutex_;
        std::unique_ptr<boost::interprocess::scoped_lock<boost::interprocess::named_mutex>> lock_;
    };

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
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info);
}