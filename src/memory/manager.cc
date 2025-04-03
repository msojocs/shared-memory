#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <thread>
#include <chrono>

namespace SharedMemory {
    using namespace boost::interprocess;

    SharedMemoryManager::SharedMemoryManager(const std::string& key, bool create, size_t size) {
        try {
            spdlog::info("Creating SharedMemoryManager with key: {}, create: {}, size: {}", key, create, size);
            
            if (create) {
                // 如果已存在，先尝试删除
                try {
                    spdlog::info("Removing existing shared memory and mutex...");
                    shared_memory_object::remove(key.c_str());
                    named_mutex::remove((key + "_mutex").c_str());
                    spdlog::info("Existing shared memory and mutex removed successfully.");
                } catch (const std::exception& e) {
                    spdlog::warn("移除旧共享内存时出错: {}", e.what());
                }

                // 创建新的共享内存
                spdlog::info("Creating new shared memory...");
                shm_ = std::make_shared<shared_memory_object>(create_only, key.c_str(), read_write);
                shm_->truncate(size);
                spdlog::info("New shared memory created successfully.");
                
                // 创建互斥锁
                spdlog::info("Creating new mutex...");
                mutex_ = std::make_shared<named_mutex>(create_only, (key + "_mutex").c_str());
                spdlog::info("New mutex created successfully.");
            } else {
                // 打开已存在的共享内存
                spdlog::info("Opening existing shared memory...");
                shm_ = std::make_shared<shared_memory_object>(open_only, key.c_str(), read_write);
                spdlog::info("Existing shared memory opened successfully.");
                
                // 打开互斥锁
                spdlog::info("Opening existing mutex...");
                try {
                    // 先尝试删除可能存在的旧互斥锁
                    named_mutex::remove((key + "_mutex").c_str());
                } catch (...) {
                    // 忽略删除错误
                }
                
                // 重新创建互斥锁
                mutex_ = std::make_shared<named_mutex>(create_only, (key + "_mutex").c_str());
                spdlog::info("Mutex recreated successfully.");
            }

            // 映射内存区域
            spdlog::info("Mapping memory region...");
            region_ = std::make_shared<mapped_region>(*shm_, read_write);
            spdlog::info("Memory region mapped successfully. Address: {}, Size: {}", 
                region_->get_address(), region_->get_size());

            // 获取互斥锁
            spdlog::info("Acquiring mutex lock...");
            lock_ = std::make_unique<scoped_lock<named_mutex>>(*mutex_);
            spdlog::info("Mutex lock acquired successfully.");

            if (create) {
                // 初始化内存
                spdlog::info("Initializing memory...");
                std::memset(region_->get_address(), 0, region_->get_size());
                spdlog::info("Memory initialized successfully.");
            }
            
            spdlog::info("SharedMemoryManager created successfully.");
        } catch (const std::exception& e) {
            // 清理资源
            try {
                if (mutex_) {
                    lock_.reset();
                    mutex_.reset();
                    named_mutex::remove((key + "_mutex").c_str());
                }
            } catch (...) {
                // 忽略清理错误
            }
            
            spdlog::error("Error in SharedMemoryManager constructor: {}", e.what());
            throw std::runtime_error(std::string("共享内存操作失败: ") + e.what());
        } catch (...) {
            // 清理资源
            try {
                if (mutex_) {
                    lock_.reset();
                    mutex_.reset();
                    named_mutex::remove((key + "_mutex").c_str());
                }
            } catch (...) {
                // 忽略清理错误
            }
            
            spdlog::error("Unknown error in SharedMemoryManager constructor");
            throw std::runtime_error("共享内存操作失败: 未知错误");
        }
    }
} 