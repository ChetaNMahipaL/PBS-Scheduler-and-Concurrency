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
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock; // multi cpus so to protect below shared variable
  int count[PGROUNDUP(PHYSTOP)/PGSIZE]; // to store count of physical page references
} pg_ref;


// functions for page references

void
kfreincr(void *pa)
{
  acquire(&pg_ref.lock);
  pg_ref.count[((uint64)pa)/PGSIZE]++;
  release(&pg_ref.lock);  
}


int
kfreget(void* pa)
{
  int temp;
  acquire(&pg_ref.lock);
  temp = pg_ref.count[((uint64)pa)/PGSIZE];
  release(&pg_ref.lock);
  return temp;
}

void
kfredecr(void *pa)
{
  acquire(&pg_ref.lock);
  pg_ref.count[((uint64)pa)/PGSIZE]--;
  release(&pg_ref.lock);
}

void
kinit()
{
  initlock(&pg_ref.lock, "pg_ref");
  acquire(&pg_ref.lock);
  for(int i=0;i<PGROUNDUP(PHYSTOP)/PGSIZE;++i)
  {
    pg_ref.count[i]=0;
  }
  release(&pg_ref.lock);
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    // acquire(&pg_ref.lock);
    // pg_ref.count[(uint64)p/PGSIZE]=1;
    // release(&pg_ref.lock);
    kfreincr((void*)p);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP) {
    // printf("Hello\n");
    panic("kfree");
  }

  // check for proper functioning
  // acquire(&kmem.lock);
  if(kfreget(pa) <= 0)
    panic("kfree decr");
  // release(&kmem.lock);

  // acquire(&kmem.lock);
  // number of references 0 or less means the physical memory is released
  kfredecr(pa);
  // release(&kmem.lock);

  // acquire(&kmem.lock);
  if(kfreget(pa) > 0)
    return;
  // release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    // set physical references to 1 for signaling allocation
    acquire(&pg_ref.lock);
    pg_ref.count[(uint64)r/PGSIZE] = 1;
    release(&pg_ref.lock);
  }
  return (void*)r;
}
