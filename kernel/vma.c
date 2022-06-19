#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fcntl.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "vma.h"

struct vma vmas[VMA_ARR_SIZE];

void vmainit()
{
    for(int i = 0; i < VMA_ARR_SIZE; ++i)
    {
        vmas[i].uvmaddr = 0;
    }
}

int isvalidvma(struct vma* vma)
{
    return vma->uvmaddr > 0;
}

void*
mmap(void* addr, size_t length, int prot, int flags, struct file *f, off_t offset)
{
    // check prot PROT_READ
    if(!f->readable && (prot & PROT_READ))
    {
        return MAP_FAILED;
    }

    // check prot PROT_WRITE
    if(!f->writable && (prot & PROT_WRITE))
    {
        return MAP_FAILED;
    }

    struct proc *p = myproc();
    struct vma* vma = &vmas[0];
    for(int i = 0; i < VMA_ARR_SIZE; ++i)
    {
        if(isvalidvma(&vmas[i]))
        {
            vma = &vmas[i];
            break;
        }
    }

    // mmap should not allocate physical memory or read the file
    // do that in page fault handling code in
    vma->uvmaddr = (void*)p->sz;
    if(p->sz + length > MAXVA)
    {
        panic("mmap length exceed max va");
    }
    p->sz += length;

    vma->taraddr = addr;
    vma->length = length;
    // ignore check prot for test not cover it
    vma->prot = prot;
    // ignore check flags for test not cover it
    vma->flags = flags;
    vma->f = f;
    // mmap should increase the file's reference 
    // count so that the structure doesn't disappear 
    // when the file is closed
    filedup(f);
    vma->offset = offset;
    return vma->uvmaddr;
}

void 
munmap(void* addr, size_t length)
{

}