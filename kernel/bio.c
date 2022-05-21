// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#include <stddef.h>

/*
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;
*/

// Three level locks
// - global lock, bcache.lock
// - - bucket lock, bucket.lock
// - - - buffer lock, buf.lock
struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct bucket hashtable[BUCKETSIZE];
  struct buf buf[NBUF];
} bcache;


void
updatebuftimestamp(struct buf *b) {
  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
}


/*
void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}
*/


void
binit(void)
{
  // init global lock
  initlock(&bcache.lock, "bcache");

  // init bucket lock
  for(int i = 0; i < BUCKETSIZE; ++i) {
    bcache.hashtable[i].head.next = &(bcache.hashtable[i].head);
    bcache.hashtable[i].head.prev = &(bcache.hashtable[i].head);
    initlock(&bcache.hashtable[i].lock, "bcachebucket");
  }

  // init buf (and lock), and put them all on bucket 0
  struct buf *b;
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.hashtable[0].head.next;
    b->prev = &bcache.hashtable[0].head;
    initsleeplock(&b->lock, "bcachebuffer");
    bcache.hashtable[0].head.next->prev = b;
    bcache.hashtable[0].head.next = b;
  }
}

/*
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}
*/


static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct bucket *bucket;

  bucket = &(bcache.hashtable[blockno % BUCKETSIZE]);
  acquire(&(bucket->lock));
  for(b = bucket->head.next; b != &(bucket->head); b = b->next){
    // printf("blockno: %d, bucket address: %x\n", blockno, b);
    // find the hit buf in bucket
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&(bucket->lock));
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&(bucket->lock));

  // now we need to eviction
  struct buf *evictb = NULL;
  acquire(&bcache.lock);
  for(int i = 0; i < BUCKETSIZE; ++i) {
    struct buf *tempb;
    struct bucket *tempbucket;
    
    tempbucket = &(bcache.hashtable[i]);
    // if (tempbucket != bucket)
    acquire(&(tempbucket->lock));

    for(tempb = tempbucket->head.next; tempb != &(tempbucket->head); tempb = tempb->next) {
      if (tempb->refcnt != 0) 
        continue;

      if (evictb) {
        if (evictb->timestamp > tempb->timestamp) {
          evictb = tempb;
        }
      } else {
        evictb = tempb;
      }
    }

    // if (tempbucket != bucket)
    release(&(tempbucket->lock));
  }
  // release(&bcache.lock);

  if (!evictb)
    panic("bget: no buffers");
  
  struct bucket *evictionbucket = &(bcache.hashtable[evictb->blockno % BUCKETSIZE]);

  if (bucket == evictionbucket) { // the eviction one already in this bucket
    acquire(&(bucket->lock));

    evictb->dev = dev;
    evictb->blockno = blockno;
    evictb->valid = 0;
    evictb->refcnt = 1;

    release(&(bucket->lock));
  } else {
    acquire(&(bucket->lock));
    acquire(&(evictionbucket->lock));

    evictb->dev = dev;
    evictb->blockno = blockno;
    evictb->valid = 0;
    evictb->refcnt = 1;

    evictb->prev->next = evictb->next;
    evictb->next->prev = evictb->prev;

    evictb->next = bucket->head.next;
    evictb->prev = &(bucket->head);
    evictb->next->prev = evictb;
    evictb->prev->next = evictb;

    release(&(evictionbucket->lock));
    release(&(bucket->lock));
  }
  release(&bcache.lock);
  acquiresleep(&evictb->lock);
  return evictb;
}


/*
// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}
*/


struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  updatebuftimestamp(b);
  return b;
}


/*
// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}
*/


void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
  updatebuftimestamp(b);
}


/*
// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}
*/


void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
  b->refcnt--;
  release(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
}


/*
void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}
*/


void
bpin(struct buf *b) {
  acquire(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
  b->refcnt++;
  release(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
}


/*
void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}
*/


void
bunpin(struct buf *b) {
  acquire(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
  b->refcnt--;
  release(&bcache.hashtable[b->blockno % BUCKETSIZE].lock);
}