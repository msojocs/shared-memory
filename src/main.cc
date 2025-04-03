#include <sys/types.h>
#ifdef _WIN32
#include <process.h>
#endif
#include <napi.h>

/**
 * @brief 测试
 *
 * @param info
 * @return Napi::Boolean
 */
static Napi::Boolean test(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() == 0) {
    throw Napi::Error::New(env, "参数长度不能为0!");
  }
  auto msg = info[0];
  if (!msg.IsObject()) {
    throw Napi::Error::New(env, "参数必须是对象!");
  }
  return Napi::Boolean::New(env, true);
}

static Napi::Object Init(Napi::Env env, Napi::Object exports) {

  exports.Set(Napi::String::New(env, "test"),
              Napi::Function::New(env, test));

  return exports;
}

NODE_API_MODULE(cmnative, Init)