// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU];
} kmem;

int rand = 0;

void
kinit()
{
  for(int i=0; i<NCPU; i++)
  {
    initlock(&kmem.lock[i], "kmem"+i);
  }
  freerange(end, (void*)PHYSTOP);
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
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  /*
  int i = rand % 4;
  acquire(&kmem.lock[i]);
  r->next = kmem.freelist[i];
  kmem.freelist[i] = r;
  release(&kmem.lock[i]);
  */
  push_off();
  int cpu_id = cpuid();
  acquire(&kmem.lock[cpu_id]);
  r->next = kmem.freelist[cpu_id];
  kmem.freelist[cpu_id] = r;
  release(&kmem.lock[cpu_id]);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  struct run *tmp;
  push_off();

  int cpu_id = cpuid();

  acquire(&kmem.lock[cpu_id]);
  r = kmem.freelist[cpu_id];
  if(r)
    kmem.freelist[cpu_id] = r->next;
  else{
    for(int i=0; i<NCPU; i++){
      if(cpu_id != i){
        acquire(&kmem.lock[i]);
        for(int m=0; m<10; m++){
          tmp = kmem.freelist[i];
          if(tmp){
            kmem.freelist[i] = kmem.freelist[i]->next;
            tmp->next = kmem.freelist[cpu_id];
            kmem.freelist[cpu_id] = tmp;
          }
          else break;
        }
        release(&kmem.lock[i]);
      }
      else continue;
    }
    r = kmem.freelist[cpu_id];
    if(r)
      kmem.freelist[cpu_id] = r->next;
  }
  release(&kmem.lock[cpu_id]);
  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
