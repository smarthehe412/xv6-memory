extern int mm_init(void);
extern void *mm_malloc(uint size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, uint size);
void *new_alloc(int wcnt);
void *merge(void* p);
void place(void *p, uint size);
void *find(uint size);
void insert(void *ptr);
void delete(void *ptr);