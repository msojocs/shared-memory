const sharedMemory = require('../build/sharedMemory.node');
const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

process.on('uncaughtException', (err) => {
    console.error('未捕获的异常:', err);
    process.exit(1);
});

// 添加退出处理程序
process.on('exit', () => {
    console.info('程序正在退出，清理资源...');
    try {
        // 尝试清理所有可能的共享内存
        const keys = ['mySharedMemory', 'testKey'];
        for (const key of keys) {
            try {
                sharedMemory.removeMemory(key);
                console.info(`已清理共享内存: ${key}`);
            } catch (e) {
                console.error(`清理共享内存 ${key} 失败:`, e);
            }
        }
    } catch (e) {
        console.error('清理资源时出错:', e);
    }
});

(async () => {
    try {
        await sleep(1000);
        const key = "mySharedMemory";
        const length = 20; // 共享内存的大小（字节）
        
        console.info('-------set--------')
        sharedMemory.setConsole(console.info);
        
        // 创建共享内存
        console.info('创建共享内存...');
        let memSet = sharedMemory.setMemory(key, length);
        console.info('共享内存创建成功', memSet);
        
        // 创建一个Uint8Array视图，用于访问ArrayBuffer数据
        let memSetView = new Uint8Array(memSet);
        for (let i = 0; i < length; i++) {
            memSetView[i] = i;
        }
        console.info('写入数据完成', memSetView);
        
        // 打印初始状态
        console.info('------print initial state---------')
        console.log('Buffer length:', memSet.byteLength);
        
        // 等待一段时间
        console.info('-------sleep 1s--------')
        await sleep(1000);
        
        // 获取共享内存
        console.info('-------get--------')
        console.info('获取共享内存...');
        let memGet = sharedMemory.getMemory(key);
        console.info('共享内存获取成功', memGet);
        
        // 创建一个Uint8Array视图，用于访问ArrayBuffer数据
        let memGetView = new Uint8Array(memGet);
        for (let i = 0; i < length; i++) {
            if (memGetView[i] !== i)
            {
                console.info('数据不一致！', memGetView[i], i);
            }
        }
        
        // 打印数据
        console.info('------print data---------')
        console.log('Buffer length:', memGet.byteLength);
        
        // 等待一段时间
        console.info('-------sleep 5s--------')
        await sleep(5000);
        
        // 清理共享内存
        console.info('-------cleanup--------')
        try {
            const removed = sharedMemory.removeMemory(key);
            console.log('共享内存已清理:', removed);
        } catch (cleanupError) {
            console.error('清理共享内存失败:', cleanupError);
        }
        
        console.info('测试完成');
        
        // 确保在退出前释放所有引用
        memSet = null;
        memGet = null;
        memSetView = null;
        memGetView = null;
        
        // 等待一段时间，确保所有资源都被释放
        await sleep(1000);
        
        // 正常退出
        process.exit(0);
    } catch (error) {
        console.error('操作失败:', error);
        process.exit(1);
    }
})();
