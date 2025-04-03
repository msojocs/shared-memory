#include "../memory.hh"
#include "spdlog/spdlog.h"

namespace SharedMemory {
    using namespace boost::interprocess;

    SharedMemoryManager::SharedMemoryManager(const std::string& key, bool create, size_t size) {
        try {
            if (create) {
                // 如果已存在，先尝试删除
                try {
                    shared_memory_object::remove(key.c_str());
                    named_mutex::remove((key + "_mutex").c_str());
                } catch (const std::exception& e) {
                    spdlog::warn("移除旧共享内存时出错: {}", e.what());
                }

                // 创建新的共享内存
                shm_ = std::make_shared<shared_memory_object>(create_only, key.c_str(), read_write);
                shm_->truncate(size);
                
                // 创建互斥锁
                mutex_ = std::make_shared<named_mutex>(create_only, (key + "_mutex").c_str());
            } else {
                // 打开已存在的共享内存
                shm_ = std::make_shared<shared_memory_object>(open_only, key.c_str(), read_write);
                
                // 打开互斥锁
                mutex_ = std::make_shared<named_mutex>(open_only, (key + "_mutex").c_str());
            }

            // 映射内存区域
            region_ = std::make_shared<mapped_region>(*shm_, read_write);

            // 获取互斥锁
            lock_ = std::make_unique<scoped_lock<named_mutex>>(*mutex_);

            if (create) {
                // 初始化内存
                std::memset(region_->get_address(), 0, region_->get_size());
            }
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("共享内存操作失败: ") + e.what());
        }
    }
} 