#include "../memory.hh"
#include <cstdarg>
#include <cstdio>

namespace SharedMemory {
    // 全局回调函数
    Napi::FunctionReference console_callback;

    // 设置控制台回调函数
    Napi::Value set_console(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        
        if (info.Length() != 1) {
            throw Napi::Error::New(env, "参数长度必须为1!");
        }
        if (!info[0].IsFunction()) {
            throw Napi::Error::New(env, "参数必须是函数!");
        }

        // 保存回调函数
        console_callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        return env.Undefined();
    }

    // 日志辅助函数 - 字符串版本
    void log(const std::string& message) {
        if (!console_callback.IsEmpty()) {
            Napi::Env env = console_callback.Env();
            console_callback.Call({Napi::String::New(env, message)});
        }
    }

    // 日志辅助函数 - 格式化版本
    void log(const char* format, ...) {
        if (!console_callback.IsEmpty()) {
            char buffer[1024];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            Napi::Env env = console_callback.Env();
            console_callback.Call({Napi::String::New(env, buffer)});
        }
    }
} 