// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

/*
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
*/

struct kmem {
  struct spinlock lock;
  struct run *freelist;
  void *memstart;
  void *memend;
};

struct kmem kmems[NCPU];

/*
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}
*/

int getinitfreepagecount() {
  int initpagecount = 0;
  char *p = (char*)PGROUNDUP((uint64)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
    ++initpagecount;
  return initpagecount;
}

/*
 * kinit() is only called by CPU#0 in main.c
 */
void
kinit()
{
  int initfreepagecount = getinitfreepagecount();
  // printf("initfreepagecount:%d\n", initfreepagecount);

  // allocate free page to each CPU
  int allocatedfreepagecount[NCPU];
  for(int hartid = 0; hartid < NCPU; ++hartid) {
    allocatedfreepagecount[hartid] = initfreepagecount / NCPU;
    if(hartid < initfreepagecount - allocatedfreepagecount[hartid] * NCPU) {
      ++allocatedfreepagecount[hartid];
    }
    // printf("allocatedfreepagecount[%d]:%d\n", hartid, allocatedfreepagecount[hartid]);
  }

  for(int hartid = 0; hartid < NCPU; ++hartid) {
    char lockname[6];
    int ret;
    ret = snprintf(lockname, 6, "kmem%d", hartid);
    if (ret < 0) {
      panic("format memory lock name failed");
    }
    initlock(&kmems[hartid].lock, lockname);

    if (hartid == 0) {
      kmems[hartid].memstart = (char*)PGROUNDUP((uint64)end);
      kmems[hartid].memend = kmems[hartid].memstart + PGSIZE * allocatedfreepagecount[hartid];
    } else if (hartid == NCPU-1) {
      kmems[hartid].memstart = kmems[hartid - 1].memend;
      kmems[hartid].memend = (void*)PHYSTOP;
    } else {
      kmems[hartid].memstart = kmems[hartid - 1].memend;
      kmems[hartid].memend = kmems[hartid].memstart + PGSIZE * allocatedfreepagecount[hartid];
    }

    // printf(
    //   "allocate cpu[%d] free memory from %p to %p\n", 
    //   hartid,
    //   kmems[hartid].memstart,
    //   kmems[hartid].memend
    // );

    for(char *p = kmems[hartid].memstart; p + PGSIZE <= (char*)(kmems[hartid].memend); p += PGSIZE) {
      struct run *r = (struct run *)p;
      r->next = kmems[hartid].freelist;
      kmems[hartid].freelist = r;
    }
  }
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
/*
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}
*/

void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int hart = cpuid();
  pop_off();

  acquire(&kmems[hart].lock);
  r->next = kmems[hart].freelist;
  kmems[hart].freelist = r;
  release(&kmems[hart].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
/*
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
*/

void *
kalloc(void)
{
  push_off();
  int hart = cpuid();
  pop_off();

  struct run *r;

  acquire(&kmems[hart].lock);
  r = kmems[hart].freelist;
  if(r)
    kmems[hart].freelist = r->next;
  release(&kmems[hart].lock);

  if(!r) { // steal free pages from other CPUs
    for(int hartid = 0; hartid < NCPU; ++hartid) {
      if(hartid == hart) {
        continue;
      }
      acquire(&kmems[hartid].lock);
      r = kmems[hartid].freelist;
      if(r) {
        kmems[hartid].freelist = r->next;
        release(&kmems[hartid].lock);
        break;
      }
      release(&kmems[hartid].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}