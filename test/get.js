const sharedMemory = require('../build/sharedMemory.node');
const key = "skyline_16_1743670121111";

try {
    // 获取共享内存
    console.info('-------get--------')
    const result = sharedMemory.getMemory(key);
    if (!result || !result.length) {
        throw new Error('获取共享内存失败或共享内存为空');
    }
    
    console.info('------print data---------')
    console.log('Buffer length:', result.length);
    console.log('Data:', result);
    
    // 验证数据
    console.info('------verify data---------')
    let isValid = true;
    for (let i = 0; i < Math.min(100, result.length); i++) {
        if (result[i] !== i % 256) {
            console.error(`数据验证失败: 位置 ${i} 的值为 ${result[i]}，期望值为 ${i % 256}`);
            isValid = false;
            break;
        }
    }
    if (isValid) {
        console.log('数据验证成功');
    }
    
    // 清理共享内存
    console.info('-------cleanup--------')
    try {
        const removed = sharedMemory.removeMemory(key);
        console.log('共享内存已清理:', removed);
    } catch (cleanupError) {
        console.error('清理共享内存失败:', cleanupError.message);
        throw cleanupError;
    }
} catch (error) {
    console.error('操作失败:', error.message);
    process.exit(1);
}