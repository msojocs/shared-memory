#include "../memory.hh"
#include <cstring>
#include <algorithm>
#include <memory>
#include <direct.h> // 用于Windows目录创建
#include <shlobj.h> // 用于获取用户目录
#include <mutex>

namespace SharedMemory {

    // 全局管理器，用于跟踪所有共享内存
    static std::map<std::string, std::shared_ptr<SharedMemoryManager>> g_managers;
    static std::mutex g_managers_mutex;

    SharedMemoryManager::SharedMemoryManager(const std::string& key, bool create, size_t size) 
        : key_(key), size_(size), address_(nullptr), file_mapping_(nullptr), mutex_(nullptr) {
        // 计算实际需要分配的大小（包括头部）
        size_t total_size = sizeof(SharedMemoryHeader) + size;
        
        // 创建互斥锁名称
        std::string mutex_name = "Global\\SharedMemoryMutex_" + key;
        
        // 获取用户目录
        char user_path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, user_path))) {
            log("User path: %s", user_path);
        } else {
            // 如果获取用户目录失败，使用当前目录
            GetCurrentDirectoryA(MAX_PATH, user_path);
            log("Using current directory: %s", user_path);
        }
        
        // 创建文件路径
        std::string shared_memory_dir = std::string(user_path) + "\\SharedMemory";
        std::string file_path = shared_memory_dir + "\\" + key + ".dat";
        
        // 确保目录存在
        _mkdir(shared_memory_dir.c_str());
        
        // 创建或打开互斥锁
        mutex_ = CreateMutexA(NULL, FALSE, mutex_name.c_str());
        if (!mutex_) {
            DWORD error = GetLastError();
            log("Failed to create mutex, error code: %lu", error);
            throw std::runtime_error("Failed to create mutex");
        }
        
        // 获取互斥锁
        if (WaitForSingleObject(mutex_, 5000) != WAIT_OBJECT_0) {
            DWORD error = GetLastError();
            log("Failed to acquire mutex, error code: %lu", error);
            CloseHandle(mutex_);
            mutex_ = nullptr;
            throw std::runtime_error("Failed to acquire mutex");
        }
        
        try {
            // 创建或打开文件
            HANDLE file_handle = INVALID_HANDLE_VALUE;
            if (create) {
                // 创建新文件
                file_handle = CreateFileA(
                    file_path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  // 允许其他进程读写
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                
                if (file_handle == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    log("Failed to create file, error code: %lu", error);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to create file");
                }
                
                // 设置文件大小
                LARGE_INTEGER file_size;
                file_size.QuadPart = total_size;
                if (!SetFilePointerEx(file_handle, file_size, NULL, FILE_BEGIN) || 
                    !SetEndOfFile(file_handle)) {
                    DWORD error = GetLastError();
                    log("Failed to set file size, error code: %lu", error);
                    CloseHandle(file_handle);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to set file size");
                }
            } else {
                // 打开现有文件
                file_handle = CreateFileA(
                    file_path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  // 允许其他进程读写
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                
                if (file_handle == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    log("Failed to open file, error code: %lu", error);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to open file");
                }
            }
            
            // 创建文件映射
            file_mapping_ = CreateFileMappingA(
                file_handle,          // 使用实际文件
                NULL,                 // 默认安全属性
                PAGE_READWRITE,       // 读写权限
                0,                    // 最大大小的高32位
                total_size,           // 最大大小的低32位
                NULL                  // 不使用命名映射
            );
            
            // 关闭文件句柄，文件映射会保持文件打开
            CloseHandle(file_handle);
            
            if (!file_mapping_) {
                DWORD error = GetLastError();
                log("Failed to create file mapping, error code: %lu", error);
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = nullptr;
                throw std::runtime_error("Failed to create file mapping");
            }
            
            // 映射视图
            address_ = MapViewOfFile(
                file_mapping_,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                total_size
            );
            
            if (!address_) {
                DWORD error = GetLastError();
                log("Failed to map view of file, error code: %lu", error);
                CloseHandle(file_mapping_);
                file_mapping_ = nullptr;
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = nullptr;
                throw std::runtime_error("Failed to map view of file");
            }
            
            // 如果是新创建的共享内存，初始化头部
            if (create) {
                SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
                header->size = size;
                header->version = 1;
                log("Initialized shared memory header: size=%zu, version=%d", size, header->version);
            }
            
            // 存储文件路径
            file_path_ = file_path;
            
            // 不再将管理器添加到全局映射中，避免循环引用
            // 这可能是导致堆损坏的原因
            
            log("Shared memory %s: key=%s, size=%zu, address=%p, file=%s", 
                create ? "created" : "opened", 
                key.c_str(), 
                size, 
                address_,
                file_path_.c_str());
                
        } catch (...) {
            // 确保在发生异常时释放资源
            if (address_) {
                UnmapViewOfFile(address_);
                address_ = nullptr;
            }
            
            if (file_mapping_) {
                CloseHandle(file_mapping_);
                file_mapping_ = nullptr;
            }
            
            if (mutex_) {
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = nullptr;
            }
            
            throw;
        }
        
        // 释放互斥锁
        ReleaseMutex(mutex_);
    }
    
    SharedMemoryManager::~SharedMemoryManager() {
        // 释放资源
        if (address_) {
            UnmapViewOfFile(address_);
            address_ = nullptr;
        }
        
        if (file_mapping_) {
            CloseHandle(file_mapping_);
            file_mapping_ = nullptr;
        }
        
        if (mutex_) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = nullptr;
        }
        
        // 不再从全局管理器中移除，因为我们不再添加
        
        log("Shared memory manager destroyed: key=%s, file=%s", 
            key_.c_str(), 
            file_path_.c_str());
    }
} 