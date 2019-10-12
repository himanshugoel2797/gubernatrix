/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "debug.h"
#include "stddef.h"
#include "stdint.h"
#include "types.h"

static char *hex_str = "0123456789ABCDEF";
static char *dec_str = "0123456789";

#define SERIAL_OUT 0x3f8

void debug_init(void) {
  outb(SERIAL_OUT + 1, 0);    // disable interrupts
  outb(SERIAL_OUT + 3, 0x80); // DLAB
  outb(SERIAL_OUT, 1);        // divisor = 1
  outb(SERIAL_OUT + 1, 0);    // hi byte
  outb(SERIAL_OUT + 3, 0x03); // 8 bits, no parity, one stop bit
}

static bool is_transmit_empty(void) { return inb(SERIAL_OUT + 5) & 0x20; }

void print_str(const char *str) {
  do {
    if (*str != '\0') {
      while (is_transmit_empty() == 0)
        ;
      outb(SERIAL_OUT, *str);
    }
  } while (*(str++) != '\0');
}

void set_trap_str(const char *str) { print_str(str); }

void debug_handle_trap() {
  print_str("\r\nHALTED");
  halt();
}

void print_ptr(uintptr_t num) { print_uint64((uint64_t)num, BASE_HEX); }

#define PRINT_BITCNT(bitcnt)                                                   \
  if (base == BASE_HEX)                                                        \
    for (int i = (bitcnt - 4); i >= 0; i -= 4) {                               \
      while (is_transmit_empty() == 0)                                         \
        ;                                                                      \
      outb(SERIAL_OUT, hex_str[(num >> i) & 0xF]);                             \
    }

void print_int8(int8_t num, uint8_t base) { PRINT_BITCNT(8) }
void print_int16(int16_t num, uint8_t base) { PRINT_BITCNT(16) }
void print_int32(int32_t num, uint8_t base) { PRINT_BITCNT(32) }
void print_int64(int64_t num, uint8_t base) { PRINT_BITCNT(64) }

void print_uint8(uint8_t num, uint8_t base) { PRINT_BITCNT(8) }
void print_uint16(uint16_t num, uint8_t base) { PRINT_BITCNT(16) }
void print_uint32(uint32_t num, uint8_t base) { PRINT_BITCNT(32) }
void print_uint64(uint64_t num, uint8_t base) { PRINT_BITCNT(64) }

void print_hexdump(uint8_t *data, int len) {
  for (int off = 0; off < len; off += 16)
    for (int l_off = 0; l_off < 16 && off + l_off < len; l_off++) {
      if (l_off == 0) {
        print_uint64((uint64_t)&data[off + l_off], BASE_HEX);
        print_str("  ");
      }
      print_uint8(data[off + l_off], BASE_HEX);
      if (l_off == 7)
        print_str("  ");
      else if (l_off < 15)
        print_str(" ");
      else
        print_str("\r\n");
    }
  print_str("\r\n");
}