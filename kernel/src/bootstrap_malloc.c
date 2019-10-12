/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "types.h"

#include "debug.h"
#include "local_spinlock.h"

#define BOOTSTRAP_ALLOC_AREA_SIZE (MiB(128))

static uint8_t bootstrap_alloc_area[BOOTSTRAP_ALLOC_AREA_SIZE];
static uint64_t bootstrap_alloc_pos = 0;
static int bootstrap_alloc_lock = 0;

static void* (*malloc_hndl_l)(size_t);
static void (*free_hndl_l)(void*);

void bootstrap_malloc_init(uintptr_t kernel_end_virt){
    kernel_end_virt = 0;
    //bootstrap_alloc_area = (uint8_t*)PAGE_ALIGN(kernel_end_virt);
    malloc_hndl_l = NULL;
    free_hndl_l = NULL;
}

void *bootstrap_malloc(size_t s) {

    void *mem = NULL;

    if (s > 0) {
        s = ALIGN(s, 16);
        local_spinlock_lock(&bootstrap_alloc_lock);
        if (bootstrap_alloc_pos + s < BOOTSTRAP_ALLOC_AREA_SIZE) {
            mem = &bootstrap_alloc_area[bootstrap_alloc_pos];
            bootstrap_alloc_pos += s;
        }
        local_spinlock_unlock(&bootstrap_alloc_lock);
    }
    return mem;
}

void bootstrap_free(void *mem, size_t s) {
    // If another allocation has not been made yet, we can free the memory
    if (mem == NULL)
        return;

    if (s == 0)
        return;

    if (s % 16 != 0) // Require all allocations to be 16-byte aligned
        s = ((s >> 4) + 1) << 4;

    local_spinlock_lock(&bootstrap_alloc_lock);
    if (bootstrap_alloc_pos > s &&
            &bootstrap_alloc_area[bootstrap_alloc_pos - s] == (uint8_t *)mem)
        bootstrap_alloc_pos -= s;

    local_spinlock_unlock(&bootstrap_alloc_lock);
}

void* malloc(size_t s){
    if(malloc_hndl_l == NULL) return bootstrap_malloc(s);
    return malloc_hndl_l(s);
}

void free(void* ptr) {
    if (ptr == NULL)
        return;

    if (free_hndl_l != NULL) free_hndl_l(ptr);
}

void bootstrap_malloc_update_handlers(void*(*malloc_hndl)(size_t), void (*free_hndl)(void*)) {
    malloc_hndl_l = malloc_hndl;
    free_hndl_l = free_hndl;
}