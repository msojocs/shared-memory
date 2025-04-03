#include "napi.h"
namespace SharedMemory {
    /**
     * 设置共享内存 
     * @param info 回调信息
     * @return 共享内存的视图
     */
    Napi::Uint8Array set_memory(const Napi::CallbackInfo &info);
    /**
     * 获取共享内存
     * @param info 回调信息
     * @return 共享内存的视图
     */
    Napi::Uint8Array get_memory(const Napi::CallbackInfo &info);
    Napi::Boolean remove_memory(const Napi::CallbackInfo &info);
}