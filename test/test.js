const sharedMemory = require('../build/sharedMemory.node');
const key = "2015";

try {
    // 获取共享内存
    sharedMemory.setConsole(console.info)
    console.info('-------set--------')
    let result = sharedMemory.setMemory(key, 21066248);
    console.info('\nSet result:', result)
    console.info('\n-------get--------')
    sharedMemory.getMemory(key);
    console.info('\nGet result:', result)
    
} catch (error) {
    console.error('操作失败:', error.message);
    process.exit(1);
}