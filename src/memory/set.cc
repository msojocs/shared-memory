#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

namespace SharedMemory {

    using namespace boost::interprocess;
    void set_memory()
    {

        // 创建或打开托管共享内存
        managed_shared_memory segment(open_or_create, "MySharedMemory", 65536);
        
        // 自动构造对象并同步
        // MyClass* obj = segment.construct<MyClass>("MyObject")();
        // named_mutex mutex(open_or_create, "MyMutex");
    }
}