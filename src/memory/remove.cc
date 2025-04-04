#include "napi.h"
#include "../memory.hh"
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <shlobj.h> // 用于CSIDL_PERSONAL
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#endif

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
            
#ifdef _WIN32
            // Windows实现
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
            
            // 尝试删除实际文件
            // 获取用户目录
            char user_path[MAX_PATH];
            std::string file_path;
            
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, user_path))) {
                log("User path: %s", user_path);
                file_path = std::string(user_path) + "\\SharedMemory\\" + key + ".dat";
            } else {
                // 如果获取用户目录失败，使用当前目录
                GetCurrentDirectoryA(MAX_PATH, user_path);
                log("Using current directory: %s", user_path);
                file_path = std::string(user_path) + "\\SharedMemory\\" + key + ".dat";
            }
            
            // 尝试删除文件
            if (DeleteFileA(file_path.c_str())) {
                log("Deleted file: %s", file_path.c_str());
            } else {
                DWORD error = GetLastError();
                if (error != ERROR_FILE_NOT_FOUND) {
                    log("Failed to delete file: %s, error code: %lu", file_path.c_str(), error);
                }
            }
            
            // 尝试删除目录（如果为空）
            std::string dir_path = std::string(user_path) + "\\SharedMemory";
            if (RemoveDirectoryA(dir_path.c_str())) {
                log("Removed directory: %s", dir_path.c_str());
            }
#else
            // Linux实现
            // 尝试删除共享内存
            std::string shm_name = "/" + key;
            if (shm_unlink(shm_name.c_str()) == 0) {
                log("Removed shared memory: %s", shm_name.c_str());
            } else if (errno != ENOENT) { // 忽略"不存在"错误
                log("Failed to remove shared memory: %s, error: %s", 
                    shm_name.c_str(), strerror(errno));
            }
            
            // 尝试删除互斥锁
            std::string mutex_name = "/SharedMemoryMutex_" + key;
            if (sem_unlink(mutex_name.c_str()) == 0) {
                log("Removed mutex: %s", mutex_name.c_str());
            } else if (errno != ENOENT) { // 忽略"不存在"错误
                log("Failed to remove mutex: %s, error: %s", 
                    mutex_name.c_str(), strerror(errno));
            }
            
            // 强制删除共享内存文件（如果存在）
            std::string file_path = "/dev/shm/" + key + ".dat";
            if (unlink(file_path.c_str()) == 0) {
                log("Removed shared memory file: %s", file_path.c_str());
            } else if (errno != ENOENT) { // 忽略"不存在"错误
                log("Failed to remove shared memory file: %s, error: %s", 
                    file_path.c_str(), strerror(errno));
            }
            
            // 强制删除信号量文件（如果存在）
            std::string sem_file_path = "/dev/shm/sem.SharedMemoryMutex_" + key;
            if (unlink(sem_file_path.c_str()) == 0) {
                log("Removed semaphore file: %s", sem_file_path.c_str());
            } else if (errno != ENOENT) { // 忽略"不存在"错误
                log("Failed to remove semaphore file: %s, error: %s", 
                    sem_file_path.c_str(), strerror(errno));
            }
            
            // 尝试打开并关闭信号量，以确保它被完全删除
            sem_t* sem = sem_open(mutex_name.c_str(), 0);
            if (sem != SEM_FAILED) {
                sem_close(sem);
                log("Opened and closed semaphore to ensure it's deleted");
            }
#endif
            
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