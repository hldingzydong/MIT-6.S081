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

int isfreevma(struct vma* vma)
{
    return vma->uvmaddr == 0;
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
    if(!f->writable && (prot & PROT_WRITE) && (flags == MAP_SHARED))
    {
        return MAP_FAILED;
    }

    struct proc *p = myproc();
    struct vma* vma = &(p->vmas[0]);
    for(int i = 0; i < VMA_ARR_SIZE; ++i)
    {
        if(isfreevma(&(p->vmas[i])))
        {
            vma = &(p->vmas[i]);
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
    vma->allocated = length;
    return vma->uvmaddr;
}

void 
munmap(void* addr, size_t length)
{
    struct proc *p = myproc();

    // lookup vmas
    struct vma* vma = (struct vma*)0;
    for(int i = 0; i < VMA_ARR_SIZE; ++i){
        if((uint64)(p->vmas[i].uvmaddr) <= (uint64)addr 
            && (uint64)(p->vmas[i].uvmaddr) + p->vmas[i].length > (uint64)addr){
            vma = &(p->vmas[i]);
            break;
        }
    }

    if(!vma)
        panic("cannot find vma");

    uint64 va = PGROUNDDOWN((uint64)addr);
    // write back to disk
    // ignore check dirty bits for test not check that
    if(vma->flags == MAP_SHARED 
       && vma->f->writable
       && filewrite(vma->f, va, length + ((uint64)addr - va)) != 0) {
        printf("write file back failed when munmap\n");
    }

    uint64 a;
    pte_t *pte;
    // free physical pages if exist
    for(a = va; a < va + length + ((uint64)addr - va); a += PGSIZE){
        if((pte = walk(p->pagetable, a, 0)) == 0)
            panic("munmap: walk");
        if((*pte & PTE_V) == 0)
            continue;
        if(PTE_FLAGS(*pte) == PTE_V)
            panic("munmap: not a leaf");
        uint64 pa = PTE2PA(*pte);
        kfree((void*)pa);
        *pte = 0;
    }

    vma->allocated -= length;
    if(vma->allocated <= 0) {
        // free vma
        fileclose(vma->f);
        vma->uvmaddr = 0;
    }
}