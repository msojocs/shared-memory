#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHM_NAME "/my_shm"
#define SHM_SIZE 1024

int main() {
    int shm_fd;
    char *shm_ptr;

    // 1. 创建共享内存对象 (O_CREAT | O_EXCL 确保新建)
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // 2. 调整共享内存大小
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(1);
    }

    // 3. 内存映射
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // 4. 写入数据
    printf("Writing to shared memory...\n");
    strncpy(shm_ptr, "Hello POSIX SHM!", SHM_SIZE);

    sleep(5); // 等待读取进程

    // 5. 清理
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME); // 删除共享内存对象

    return 0;
}