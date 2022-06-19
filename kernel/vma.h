#include <stddef.h>
#include <sys/types.h>

#define VMA_ARR_SIZE 16

#define MAP_FAILED ((char *) -1)

struct vma
{
    void*        uvmaddr;
    void*        taraddr;
    size_t      length;
    int         prot;
    int         flags;
    struct file *f;
    off_t       offset;
};

void vmainit();

void* mmap(void* addr, size_t length, int prot, int flags, struct file *f, off_t offset);

void munmap(void* addr, size_t length);