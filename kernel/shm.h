struct shm_page {
    int id;                   // 共享内存页的ID
    int count;                // 使用计数
    char *frame;              // 指向物理内存的指针
    int perm;                 // 权限
};

void shminit();
int shm_alloc(int perm);
int shm_attach(int id, void *addr);
int shm_detach(int id);
int shm_close(int id);
int shm_query_permissions(int shmid);
int shm_set_permissions(int shmid, int permissions);