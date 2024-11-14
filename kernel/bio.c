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

#define HASHNUM 13
struct head{
  int hashnum;
  int count;
};
struct hashList{
  struct head head;
  struct buf *buf;
};
struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct hashList hashlists[HASHNUM];
  struct spinlock hashlock[HASHNUM];
  int allocated;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.lock, "bcache");
  bcache.allocated = 0;
  for(int i=0; i<HASHNUM; i++)
  {
    initlock(&bcache.hashlock[i], "bcache"+i);
    bcache.hashlists[i].head.count = 0;
    bcache.hashlists[i].head.hashnum = i;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->timestamp = ticks;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{  
  struct buf *b;
  int hashnum = blockno % HASHNUM;
  //printf("block:%d, hash:",blockno);
  //printf("%d\n", hashnum);
  acquire(&bcache.hashlock[hashnum]);
  struct hashList *hlist = &bcache.hashlists[hashnum];
  // Is the block already cached?
  // cached
  b = hlist->buf;  
  for(int i=0; i < hlist->head.count; i++){
    
    if(b && b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->timestamp = ticks;
      
      release(&bcache.hashlock[hashnum]);
      
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  // not cached
  //printf("ask block:%d\n", blockno);
  release(&bcache.hashlock[hashnum]);
  acquire(&bcache.lock);
  acquire(&bcache.hashlock[hashnum]);
  hlist = &bcache.hashlists[hashnum];
  b = hlist->buf;
  for(int i=0; i < hlist->head.count; i++){
    
    if(b && b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->timestamp = ticks;
      
      release(&bcache.hashlock[hashnum]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  if(bcache.allocated < NBUF){
    
    b = &bcache.buf[bcache.allocated];
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->timestamp = ticks;
    
    b->next = hlist->buf;
    
    hlist->buf = b;
    
    hlist->head.count++;
       
    bcache.allocated ++;
    release(&bcache.hashlock[hashnum]);
    release(&bcache.lock);
    
    acquiresleep(&b->lock);
    return b;
  }
  
  
  
  
  /*for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    if(b->refcnt == 0 && b->timestamp < leasttime) {
      leasttime = b->timestamp;
      leastb = b;
    }
  }*/
  release(&bcache.hashlock[hashnum]);
  for(int i = 0; i < HASHNUM; i++){
    //printf("i:%d\n",i);
    struct buf *leastb;
    uint leasttime = __UINT64_MAX__;
    acquire(&bcache.hashlock[i]);
    for(b = bcache.hashlists[i].buf; b; b = b->next){
      if(b->refcnt == 0 && b->timestamp < leasttime){
        leastb = b;
        leasttime = b->timestamp;
       
      }
    }
    if(leastb){
      int old_hashnum = i;
      //printf("old:%d, new:%d\n",old_hashnum, hashnum);
      if(old_hashnum != hashnum)
      {
        struct hashList *leasth = &bcache.hashlists[old_hashnum];
        struct buf *old_buf = leasth->buf;
        if((old_buf->blockno == leastb->blockno) && (old_buf->dev == leastb->dev)){
            leasth->buf = leasth->buf->next;
        }
        else{
          for(int i = 0; i < leasth->head.count - 1; i++)
          {         
            if((old_buf->next->blockno == leastb->blockno) && (old_buf->next->dev == leastb->dev)){
              old_buf->next = old_buf->next->next;
              break;
            }
            
            else old_buf = old_buf->next;
          }
        }
        //printf("5\n");
        leasth->head.count--;
        release(&bcache.hashlock[old_hashnum]);
        acquire(&bcache.hashlock[hashnum]);
      }
      //printf("1\n");
      
      hlist = &bcache.hashlists[hashnum];
      leastb->dev = dev;
      leastb->blockno = blockno;
      leastb->valid = 0;
      leastb->refcnt = 1;
      leastb->timestamp = ticks;
      if(old_hashnum != hashnum){
        leastb->next = hlist->buf;
        hlist->buf = leastb;
        hlist->head.count++;
      }
      
      
      //printf("2\n");
      release(&bcache.hashlock[hashnum]);
      release(&bcache.lock);
      

      acquiresleep(&leastb->lock);
      return leastb;
    }
    else{
      release(&bcache.hashlock[i]);
      
    }
  }
  //printf("0\n");
  
  panic("bget: no buffers");
}

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

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  
  int hashnum = b->blockno % HASHNUM;
  acquire(&bcache.hashlock[hashnum]);
  b->refcnt--;
  /*if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  */
  release(&bcache.hashlock[hashnum]);
  //printf("0\n");
}

void
bpin(struct buf *b) {
  int hashnum = b->blockno % HASHNUM;
  acquire(&bcache.hashlock[hashnum]);
  b->refcnt++;
  release(&bcache.hashlock[hashnum]);
}

void
bunpin(struct buf *b) {
  int hashnum = b->blockno % HASHNUM;
  acquire(&bcache.hashlock[hashnum]);
  b->refcnt--;
  release(&bcache.hashlock[hashnum]);
}


