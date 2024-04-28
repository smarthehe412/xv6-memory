struct shm_page {
    int id;                   // 共享内存页的ID
    int count;                // 使用计数
    char *frame;              // 指向物理内存的指针
    int perm;                 // 权限
};

uint64 shm_alloc(int perm);
uint64 shm_attach(int id, void *addr);
uint64 shm_detach(int id);
uint64 shm_close(int id);