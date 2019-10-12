// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_STDLIB_H
#define GUBERNATRIX_STDLIB_H

#include "stdint.h"

void* malloc(size_t s);

void free(void* ptr);

char *itoa(int val, char *dst, int base);

char *ltoa(long long val, char *dst, int base);

int atoi(const char * ptr, int base);

#endif