#include "shm.h"
#include "spinlock.h"
#include "param.h"

struct shm_page shm_table[MAXSHMPAGES];

struct spinlock shm_table_lock;

void shm_alloc(int perm) {
    acquire(&shm_table_lock);
    release(&shm_table_lock);
}

void shm_attach(int id, void *addr) {
    acquire(&shm_table_lock);
    release(&shm_table_lock);
}

void shm_detach(int id) {
    acquire(&shm_table_lock);
    release(&shm_table_lock);
}

void shm_close(int id) {
    acquire(&shm_table_lock);
    release(&shm_table_lock);
}