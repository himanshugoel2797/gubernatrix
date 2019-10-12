// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_SYSMP_H
#define GUBERNATRIX_SYSMP_H

#include "types.h"

void smp_init(void);

int smp_corecount(void);

int smp_platform_getstatesize(void);

void smp_platform_getstate(void* buf);

void smp_platform_setstate(void* buf);

void smp_platform_getdefaultstate(void *buf, void *stackpointer, void *instr_ptr, void *args);

void smp_signalready(void);
#endif