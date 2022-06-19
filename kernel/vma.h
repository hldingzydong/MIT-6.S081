#include <stddef.h>
#include <sys/types.h>

struct vma
{
    void*        uvmaddr;
    void*        taraddr;
    size_t       length;
    int          prot;
    int          flags;
    struct file  *f;
    off_t        offset;
    int          allocated;
};

#define MAP_FAILED ((char *) -1)

void* mmap(void* addr, size_t length, int prot, int flags, struct file *f, off_t offset);

void munmap(void* addr, size_t length);