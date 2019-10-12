// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_DEBUG_H
#define GUBERNATRIX_DEBUG_H

#include "stdint.h"

#define BASE_HEX (16)

void debug_init(void);
void print_str(const char *str);
void print_ptr(uintptr_t num);

void print_int8(int8_t num, uint8_t base);
void print_int16(int16_t num, uint8_t base);
void print_int32(int32_t num, uint8_t base);
void print_int64(int64_t num, uint8_t base);

void print_uint8(uint8_t num, uint8_t base);
void print_uint16(uint16_t num, uint8_t base);
void print_uint32(uint32_t num, uint8_t base);
void print_uint64(uint64_t num, uint8_t base);

void print_hexdump(uint8_t *data, int len);

#endif