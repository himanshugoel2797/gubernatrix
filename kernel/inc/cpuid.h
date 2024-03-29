// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_CPUID_H
#define GUBERNATRIX_CPUID_H

#include "stdint.h"

typedef struct {
    uint64_t smep : 1;
    uint64_t smap : 1;
    uint64_t hugepage : 1;
    uint64_t x2apic : 1;
    uint64_t xsave : 1;
    uint64_t tsc_valid : 1;
    uint64_t tsc_deadline : 1;
    uint64_t tsc_invar : 1;
    char processor_name[12];
    uint64_t tsc_freq;
    uint64_t apic_freq;
    uint64_t xsave_bits;
    uint32_t xsave_sz;
} cpuinfo_t;

void cpuid_init(void);
cpuinfo_t* get_cpuid(void);

#endif