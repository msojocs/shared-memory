#include "napi.h"
#include "../memory.hh"
#include "spdlog/spdlog.h"
#include <boost/interprocess/errors.hpp>

namespace SharedMemory {
    class SharedMemoryWrapper {
    public:
        explicit SharedMemoryWrapper(std::shared_ptr<SharedMemoryManager> manager) 
            : manager_(std::move(manager)) {}
        
        void* get_address() const { return manager_->get_address(); }
        size_t get_size() const { return manager_->get_size(); }
        
    private:
        std::shared_ptr<SharedMemoryManager> manager_;
    };

    Napi::Value get_memory(const Napi::CallbackInfo &info) {
        spdlog::info("Get memory call.");
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
            
            // 检查键名是否为空
            if (key.empty()) {
                throw std::runtime_error("键名不能为空!");
            }
            
            spdlog::info("Opening shared memory with key: {}", key);
            
            try {
                // 使用 SharedMemoryManager 打开和管理共享内存
                spdlog::info("Creating SharedMemoryManager...");
                auto manager = std::make_shared<SharedMemoryManager>(key, false);
                spdlog::info("SharedMemoryManager created successfully.");
                
                // 获取共享内存的地址和大小
                spdlog::info("Getting shared memory address and size...");
                void* address = manager->get_address();
                size_t size = manager->get_size();
                spdlog::info("Shared memory address: {}, size: {}", address, size);
                
                // 创建一个ArrayBuffer，直接映射到共享内存
                spdlog::info("Creating ArrayBuffer...");
                auto deleter = [](void* /*data*/, void* /*hint*/) {
                    spdlog::info("Buffer cleanup callback called.");
                };
                
                spdlog::info("Creating result...");
                Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, address, size, deleter);
                
                // 将 manager 存储在 buffer 的属性中，确保它在 buffer 被垃圾回收之前不会被销毁
                spdlog::info("Storing manager in buffer...");
                auto manager_ptr = new std::shared_ptr<SharedMemoryManager>(manager);
                auto finalizer = [](Napi::Env env, void* data) -> void {
                    auto ptr = static_cast<std::shared_ptr<SharedMemoryManager>*>(data);
                    spdlog::info("Finalizing shared memory manager.");
                    delete ptr;
                };
                
                buffer.Set("_manager", Napi::External<std::shared_ptr<SharedMemoryManager>>::New(
                    env, 
                    manager_ptr,
                    finalizer
                ));
                
                spdlog::info("Return result.");
                return buffer;
            } catch (const boost::interprocess::interprocess_exception& e) {
                spdlog::error("Boost interprocess exception: {}", e.what());
                if (e.get_error_code() == boost::interprocess::not_found_error) {
                    spdlog::error("Shared memory not found: {}", e.what());
                    throw std::runtime_error("共享内存不存在，请先使用 setMemory 创建");
                } else {
                    spdlog::error("Error opening shared memory: {}", e.what());
                    throw std::runtime_error(std::string("打开共享内存失败: ") + e.what());
                }
            } catch (const std::exception& e) {
                spdlog::error("Error opening shared memory: {}", e.what());
                throw std::runtime_error(std::string("打开共享内存失败: ") + e.what());
            }
        } catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
            throw Napi::Error::New(env, std::string("获取共享内存失败: ") + e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred");
            throw Napi::Error::New(env, "获取共享内存时发生未知错误");
        }
    }
} 