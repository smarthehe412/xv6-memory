#include "shm.h"
#include "spinlock.h"
#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"

struct shm_page shm_table[MAXSHMPAGES];

struct spinlock shm_table_lock;

void shminit(void) {
    initlock(&shm_table_lock, "shmtable");
    for(int i = 0; i < MAXSHMPAGES; i++) {
        shm_table[i].id = -1;
    }
}

int shm_alloc(int perm) {
    acquire(&shm_table_lock);
    for(int i = 0; i < MAXSHMPAGES; i++) {
        if(shm_table[i].id == -1) {
            shm_table[i].frame = kalloc();
            if(shm_table[i].frame == 0) {
                kfree(shm_table[i].frame);
                release(&shm_table_lock);
                return -1;
            }
            shm_table[i].id = i;
            shm_table[i].count = 1;
            shm_table[i].perm = perm;
            memset(shm_table[i].frame, 0, PGSIZE);
            release(&shm_table_lock);
            return i;
        }
    }
    release(&shm_table_lock);
    return -1;
}

int shm_attach(int id, void *addr) {
    struct proc *p = myproc();
    if(id < 0 || id >= MAXSHMPAGES || shm_table[id].id == -1)
        return -1; //exception
    acquire(&shm_table_lock);
    mappages(p->pagetable, (uint64)addr, PGSIZE, (uint64)shm_table[id].frame, shm_table[id].perm);
    p->shm_pages[id] = &shm_table[id];
    shm_table[id].count++;
    release(&shm_table_lock);
    return 0;
}

int shm_detach(int id) {
    struct proc *p = myproc();
    if(id < 0 || id >= MAXSHMPAGES || shm_table[id].id == -1)
        return -1; //exception
    acquire(&shm_table_lock);
    uvmunmap(p->pagetable, p->shm_pages[id]->frame, 1, 0);
    p->shm_pages[id] = 0;
    shm_table[id].count--;
    shm_close(id);
    release(&shm_table_lock);
    return 0;
}

int shm_close(int id) {
    if(id < 0 || id >= MAXSHMPAGES || shm_table[id].id == -1 || shm_table[id].count != 0)
        return -1;
    acquire(&shm_table_lock);
    kfree(shm_table[id].frame);
    release(&shm_table_lock);
    return 0;
}

int shm_query_permissions(int id) {
    if(id < 0 || id >= MAXSHMPAGES || shm_table[id].id == -1)
        return -1; //exception
    return shm_table[id].perm;
}
int shm_set_permissions(int id, int perm) {
    if(id < 0 || id >= MAXSHMPAGES || shm_table[id].id == -1)
        return -1; //exception
    acquire(&shm_table_lock);
    shm_table[id].perm = perm;
    release(&shm_table_lock);
    return 0;
};
