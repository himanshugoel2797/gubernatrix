/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "boot_info.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdqueue.h"
#include "string.h"
#include "types.h"

#include "debug.h"

// Compute the space needed for three levels of bitmaps, 1GiB, 2MiB, 4KiB and
// allocate them
// For the higher levels, 1GiB and 2MiB, allocate an additional bitmap, use to
// determine if the subtree is in use

// If SUBMP_PRESENT, don't touch, treat as used for allocations, recurse for
// free

// If SUBMP_ABSENT, may touch, further state depends on FREE/USED

#define BTM_LEVEL (KiB(4))
#define ADDR_SHAMT (8)
#define MAX_ENTRIES (1 << (12 + ADDR_SHAMT))

#define BUILD_ADDR(addr) ((addr & ~(BTM_LEVEL - 1)) << ADDR_SHAMT)
#define BUILD_PAGES(pages) ((pages - 1) & (MAX_ENTRIES - 1))
#define BUILD_ENTRY(addr, pages) (BUILD_ADDR(addr) | BUILD_PAGES(pages))

#define GET_ADDR(e) ((e >> ADDR_SHAMT) & ~(BTM_LEVEL - 1))
#define GET_PG_CNT(e) ((e & (MAX_ENTRIES - 1)) + 1)

typedef enum {
  physmem_alloc_flags_reclaimable = (1 << 0),
  physmem_alloc_flags_data = (1 << 1),
  physmem_alloc_flags_instr = (1 << 2),
  physmem_alloc_flags_pagetable = (1 << 3),
  physmem_alloc_flags_zero = (1 << 4),
  physmem_alloc_flags_32bit = (1 << 5),
} physmem_alloc_flags_t;

static queue_t btm_level;
static uint64_t mem_size;
static uint64_t free_mem;

static uint64_t data_multiple;
static uint64_t instr_multiple;
static uint64_t pagetable_multiple;

static PURE uint64_t roundUp_po2(uint64_t val, uint64_t mult) {
  return ALIGN(val, mult);
}
static int32_t partition_pgs(uint64_t *data, int32_t lo, int32_t hi,
                             int32_t sz) {
  int32_t piv_loc = (lo + hi) / 2;
  uint64_t pivot = GET_ADDR(data[piv_loc % sz]);
  int32_t i = lo - 1;
  int32_t j = hi + 1;
  while (true) {
    do
      j--;
    while (GET_ADDR(data[j % sz]) > pivot);
    do
      i++;
    while (GET_ADDR(data[i % sz]) < pivot);

    if (i >= j)
      return j;

    data[i % sz] ^= data[j % sz];
    data[j % sz] ^= data[i % sz];
    data[i % sz] ^= data[j % sz];
  }
}
static void quicksort_pgs(uint64_t *data, int32_t lo, int32_t hi, int32_t sz) {
  while (hi > lo) {
    int32_t p = partition_pgs(data, lo, hi, sz);

    // Ensure log(n) stack usage by always recursing the smaller partition
    if (p - lo > hi - p) {
      quicksort_pgs(data, lo, p, sz);
      lo = p + 1;
    } else {
      quicksort_pgs(data, p + 1, hi, sz);
      hi = p;
    }
  }
}

static void compact_queue() {
  int32_t lo = btm_level.head;
  int32_t hi = btm_level.tail - 1;
  if (hi < lo)
    hi += btm_level.size;

  quicksort_pgs(btm_level.queue, lo, hi, btm_level.size);

  for (; lo < hi - 1; lo++) {
    int32_t pg_cnt = GET_PG_CNT(btm_level.queue[lo % btm_level.size]);
    uint64_t pg_addr = GET_ADDR(btm_level.queue[lo % btm_level.size]);
    uint64_t n_pg_addr = GET_ADDR(btm_level.queue[(lo + 1) % btm_level.size]);
    int32_t n_pg_cnt =
        pg_cnt + GET_PG_CNT(btm_level.queue[(lo + 1) % btm_level.size]);

    if (pg_cnt * BTM_LEVEL + pg_addr == n_pg_addr && n_pg_cnt < MAX_ENTRIES) {
      uint64_t n_ent = BUILD_ENTRY(pg_addr, n_pg_cnt);
      btm_level.queue[(lo + 1) % btm_level.size] = n_ent;
      btm_level.queue[lo % btm_level.size] = 0;
      btm_level.head = (btm_level.head + 1) % btm_level.size;
      btm_level.ent_cnt--;
    }
  }
}

static void insert_queue(uint64_t val) {
  if (!queue_tryenqueue(&btm_level, val)) {
    compact_queue();

    if (!queue_tryenqueue(&btm_level, val))
      PANIC("Failed to enqueue entry.");
  }
}

static void insert_queue_front(uint64_t val) {
  if (!queue_tryenqueue_front(&btm_level, val)) {
    compact_queue();

    if (!queue_tryenqueue_front(&btm_level, val))
      PANIC("Failed to enqueue entry.");
  }
}

void pagealloc_free(uintptr_t addr, uint64_t size) {

  if (addr % BTM_LEVEL != 0)
    PANIC("Misaligned address");

  if (size % BTM_LEVEL != 0)
    PANIC("Misaligned size");

  uint64_t page_cnt = size / BTM_LEVEL;
  free_mem += size;

#ifdef PHYSMEM_DEBUG_VERBOSE_HIGH
  {
    char tmp_buf[20];
    DEBUG_PRINT("SysPhysicalMemory: Free addr=");
    DEBUG_PRINT(ltoa(addr, tmp_buf, 16));
    DEBUG_PRINT(" size=");
    DEBUG_PRINT(ltoa(size, tmp_buf, 16));
    DEBUG_PRINT("\r\n");
  }
#endif

  for (uint64_t pg_0 = 0; pg_0 < page_cnt; pg_0 += MAX_ENTRIES) {
    uint64_t cur_pg = 0;
    if (page_cnt - pg_0 >= MAX_ENTRIES)
      cur_pg = MAX_ENTRIES;
    else
      cur_pg = page_cnt - pg_0;

    uint64_t val = BUILD_ENTRY(addr, cur_pg);
    addr += cur_pg * BTM_LEVEL;
    insert_queue(val);
  }
}

uintptr_t pagealloc_alloc(int domain, int color, physmem_alloc_flags_t flags,
                          uint64_t size) {

  domain = 0;
  color = 0;
  flags = 0;

  // determine how to compute cache coloring
  // Get associated cache size
  // Divide by page size to get the number of slots
  // Divide by the number of associativity to get the number of colors
  // Page index % number of colors to get the page color
  // Thus, change the page increment appropriately
  // Coloring only matters for single page allocations of cache age size

  // allocations are multiples of BTM_LEVEL pages
  size = roundUp_po2(size, BTM_LEVEL);
  int32_t pg_cnt = size / BTM_LEVEL;

  // dequeue and enqueue until a unit large enough for this allocation is found
  for (int32_t j = 0; j < 2; j++) {
    int32_t max_iter = queue_entcnt(&btm_level);
    for (int32_t i = 0; i < max_iter; i++) {
      uint64_t deq = 0;
      if (queue_trydequeue(&btm_level, &deq) == false)
        PANIC("Out of Memory!");

      /*{
              char tmp_buf[20];
              DEBUG_PRINT("SysPhysicalMemory: Deq=");
              DEBUG_PRINT(ltoa(deq, tmp_buf, 16));
              DEBUG_PRINT("\r\n");
      }*/

      // Ensure allocations required to be 32bit are such
      if (flags & physmem_alloc_flags_32bit)
        if (GET_ADDR(deq) & ~0xffffffffull) {
          insert_queue(deq);
          continue;
        }

      if ((int32_t)GET_PG_CNT(deq) >= pg_cnt) {
        uint64_t ret_addr = GET_ADDR(deq);
        uint64_t n_addr = ret_addr + pg_cnt * BTM_LEVEL;
        int32_t n_pg_cnt = GET_PG_CNT(deq) - pg_cnt;
        if (n_pg_cnt > 0)
          insert_queue_front(BUILD_ENTRY(n_addr, n_pg_cnt));

        free_mem -= size;

#ifdef PHYSMEM_DEBUG_VERBOSE_HIGH
        {
          char tmp_buf[20];
          DEBUG_PRINT("SysPhysicalMemory: Allocated addr=");
          DEBUG_PRINT(ltoa(ret_addr, tmp_buf, 16));
          DEBUG_PRINT("\r\n");
        }
#endif

        return ret_addr;
      } else
        insert_queue(deq);
    }
    compact_queue();
  }

  return -1;
}

void pmem_init(void) {
  // TODO: use registry to allocate memory for the queue, ensuring enough is
  // available
  // initialize allocator as normal
  BootInfo *b_info = get_bootinfo();

  mem_size = b_info->MemorySize;

  print_str("Memory Size: ");
  print_uint64(mem_size, BASE_HEX);
  print_str(" Memory Map Count: ");
  print_uint64(b_info->MemoryMapCount, BASE_HEX);
  print_str("\r\n");

  // Compute the number of bits
  int32_t btm_sz = roundUp_po2(mem_size, BTM_LEVEL) / BTM_LEVEL;
  // TODO: queue size becomes large with memory size increase, figure out how to
  // keep it manageable
  // TODO: also fix memsize calculation so it reflects usable memory size,
  // rather than address space size

  // Allocate the blocks
  if (queue_init(&btm_level,
                 btm_sz / 2 + 1) !=
      0) // Worst case requires half the number of entries as are pages
    PANIC("Queue init failure!");

  // parse each memory map entry and free the regions
  {
    uint64_t entry_cnt = b_info->MemoryMapCount;

    for (uint64_t i = 0; i < entry_cnt; i++) {
      uint64_t addr = b_info->MemoryMap[i].addr;
      uint64_t len = b_info->MemoryMap[i].len;

      if (len % BTM_LEVEL != 0)
        len -= len % BTM_LEVEL;

      addr = roundUp_po2(addr, BTM_LEVEL);

      //#ifdef PHYSMEM_DEBUG_VERBOSE_MID
      {
        print_str("SysPhysicalMemory: Free zones, addr=");
        print_uint64(addr, BASE_HEX);
        print_str(" len=");
        print_uint64(len, BASE_HEX);
        print_str("\r\n");
      }
      //#endif

      // if(addr <= initrd_addr && addr + len >= initrd_addr + initrd_len) {
      //    if(initrd_addr > addr) pagealloc_free(addr, initrd_addr - addr);
      //    if(initrd_addr + initrd_len < addr + len) pagealloc_free(initrd_addr
      //    + initrd_len, (addr + len) - (initrd_addr + initrd_len));
      //}else
      pagealloc_free(addr, len);
    }
  }

  data_multiple = 1;
  instr_multiple = 1;
  pagetable_multiple = 1;
  // TODO: Load these with actual data
}

uintptr_t pmem_allocpage(void) {
  return pagealloc_alloc(0, 0, physmem_alloc_flags_data, KiB(4));
}

uintptr_t pmem_allocdma(uint32_t sz) {
  return pagealloc_alloc(0, 0, physmem_alloc_flags_32bit, sz);
}

void pmem_free(uintptr_t addr) { pagealloc_free(addr, KiB(4)); }

void pmem_freedma(uintptr_t addr, uint32_t sz) { pagealloc_free(addr, sz); }