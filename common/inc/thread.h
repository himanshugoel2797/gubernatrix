// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "stdint.h"
#include "stddef.h"
#include "types.h"

#include "memory.h"
#include "interrupts.h"

typedef struct {
    char name[256];
    void *kern_stack_top;
    interrupt_register_state_t registers;
} thread_t;