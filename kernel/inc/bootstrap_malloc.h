// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef CARDINAL_BOOTSTRAP_ALLOC_H
#define CARDINAL_BOOTSTRAP_ALLOC_H

#include <stddef.h>
#include <stdint.h>

void bootstrap_malloc_init(uintptr_t kernel_end_virt);
void *bootstrap_malloc(size_t s);
void bootstrap_free(void *mem, size_t s);
void bootstrap_malloc_update_handlers(void*(*malloc_hndl)(size_t), void (*free_hndl)(void*));


#endif