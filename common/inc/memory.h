// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_KERN_MEMORY_H
#define GUBERNATRIX_KERN_MEMORY_H

#include "stdint.h"
#include "stddef.h"
#include "types.h"

typedef struct vmem vmem_t;

typedef enum {
    vmem_flags_read = 0,
    vmem_flags_write = (1 << 0),
    vmem_flags_exec = (1 << 1),
    vmem_flags_cachewritethrough = (1 << 2),
    vmem_flags_cachewriteback = (1 << 3),
    vmem_flags_cachewritecomplete = (1 << 4),
    vmem_flags_uncached = (1 << 5),

    vmem_flags_kernel = (1 << 10),
    vmem_flags_user = (1 << 11),

    vmem_flags_rw = (vmem_flags_read | vmem_flags_write),
} vmem_flags;

typedef enum {
    vmem_err_none = 0,
    vmem_err_alreadymapped = -1,
    vmem_err_continue = -2,
    vmem_err_nomapping = -3,
} vmem_errs;

void tls_init(void);

TLS void* tls_alloc(size_t sz);

void pmem_init(void);

uintptr_t pmem_allocpage(void);

uintptr_t pmem_allocdma(uint32_t sz);

void pmem_free(uintptr_t addr);

void pmem_freedma(uintptr_t addr, uint32_t sz);

int vmem_init(void);

int vmem_mp_init(void);

int vmem_map(vmem_t *vm, intptr_t virt, intptr_t phys, size_t size, int perms, int flags);

int vmem_unmap(vmem_t *vm, intptr_t virt, size_t size);

int vmem_create(vmem_t **vm);

int vmem_setactive(vmem_t *vm);

int vmem_getactive(vmem_t **vm);

int vmem_flush(intptr_t virt, size_t sz);

int vmem_virttophys(intptr_t virt, intptr_t *phys);

intptr_t vmem_phystovirt(intptr_t phys, size_t sz, int flags);

intptr_t vmem_vmalloc(size_t sz);

void vmem_vfree(intptr_t virt, size_t sz);

#endif