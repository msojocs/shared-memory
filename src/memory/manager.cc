#include "../memory.hh"
#include <cstring>

namespace SharedMemory {

SharedMemoryManager::SharedMemoryManager(const std::string& key, bool create, size_t size) 
    : key_(key), size_(size) {
    
    // 计算实际需要分配的大小（头部 + 数据区）
    size_t total_size = sizeof(SharedMemoryHeader) + size;
    
    // 创建互斥锁
    std::string mutex_name = key + "_mutex";
    mutex_ = CreateMutexA(NULL, FALSE, mutex_name.c_str());
    
    if (mutex_ == NULL) {
        DWORD error = GetLastError();
        log("Failed to create mutex, error code: %lu", error);
        throw std::runtime_error("Failed to create mutex");
    }
    
    // 获取互斥锁
    DWORD result = WaitForSingleObject(mutex_, 5000);
    if (result != WAIT_OBJECT_0) {
        log("Failed to acquire mutex, error code: %lu", result);
        CloseHandle(mutex_);
        mutex_ = NULL;
        throw std::runtime_error("Failed to acquire mutex");
    }
    
    try {
        std::string map_name = "SharedMemory_" + key;
        
        if (create) {
            // 创建文件映射
            log("Creating file mapping: %s, size: %zu", map_name.c_str(), total_size);
            file_mapping_ = CreateFileMappingA(
                INVALID_HANDLE_VALUE,  // 使用页面文件
                NULL,                 // 默认安全属性
                PAGE_READWRITE,       // 读写权限
                0,                    // 最大大小的高32位
                total_size,           // 最大大小的低32位
                map_name.c_str()      // 共享内存名称
            );
            
            if (file_mapping_ == NULL) {
                DWORD error = GetLastError();
                log("Failed to create file mapping, error code: %lu", error);
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = NULL;
                throw std::runtime_error("Failed to create file mapping");
            }
            
            log("File mapping created successfully");
        } else {
            // 打开已存在的文件映射
            log("Opening file mapping: %s", map_name.c_str());
            file_mapping_ = OpenFileMappingA(
                FILE_MAP_ALL_ACCESS,  // 读写权限
                FALSE,               // 不继承句柄
                map_name.c_str()     // 共享内存名称
            );
            
            if (file_mapping_ == NULL) {
                DWORD error = GetLastError();
                log("Failed to open file mapping, error code: %lu", error);
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = NULL;
                throw std::runtime_error("Failed to open file mapping");
            }
            
            log("File mapping opened successfully");
        }
        
        // 映射视图
        log("Mapping view of file");
        address_ = MapViewOfFile(
            file_mapping_,           // 文件映射对象句柄
            FILE_MAP_ALL_ACCESS,  // 读写权限
            0,                    // 偏移量的高32位
            0,                    // 偏移量的低32位
            total_size            // 映射的大小
        );
        
        if (address_ == NULL) {
            DWORD error = GetLastError();
            log("Failed to map view of file, error code: %lu", error);
            CloseHandle(file_mapping_);
            file_mapping_ = NULL;
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = NULL;
            throw std::runtime_error("Failed to map view of file");
        }
        
        log("View of file mapped successfully");
        
        // 初始化头部
        if (create) {
            SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
            header->size = size;
            log("Shared memory header initialized");
        }
        
        // 将地址添加到全局管理器
        AddressManager::getInstance().setAddress(key, address_);
        
        log("Shared memory %s: key=%s, size=%zu, address=%p", 
            create ? "created" : "opened", 
            key.c_str(), 
            size, 
            address_);
            
    } catch (...) {
        // 确保在发生异常时释放资源
        if (address_ != NULL) {
            UnmapViewOfFile(address_);
            address_ = NULL;
        }
        
        if (file_mapping_ != NULL) {
            CloseHandle(file_mapping_);
            file_mapping_ = NULL;
        }
        
        if (mutex_ != NULL) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = NULL;
        }
        
        throw;
    }
    
    // 释放互斥锁
    ReleaseMutex(mutex_);
}

SharedMemoryManager::~SharedMemoryManager() {
    log("Destructor called for SharedMemoryManager: key=%s", key_.c_str());
    
    try {
        // 释放互斥锁
        if (mutex_ != NULL) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = NULL;
            log("Mutex released and closed");
        }
        
        // 解除映射视图
        if (address_ != NULL) {
            UnmapViewOfFile(address_);
            address_ = NULL;
            log("View of file unmapped");
        }
        
        // 关闭文件映射句柄
        if (file_mapping_ != NULL) {
            CloseHandle(file_mapping_);
            file_mapping_ = NULL;
            log("File mapping handle closed");
        }
        
        // 从全局管理器中移除地址
        AddressManager::getInstance().removeAddress(key_);
        log("Address removed from global manager");
        
    } catch (const std::exception& e) {
        log("Error in destructor: %s", e.what());
    } catch (...) {
        log("Unknown error in destructor");
    }
    
    log("Destructor completed for SharedMemoryManager: key=%s", key_.c_str());
}

} // namespace SharedMemory 