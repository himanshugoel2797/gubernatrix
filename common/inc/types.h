// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef CARDINAL_TYPES_H
#define CARDINAL_TYPES_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAT_MSR (0x00000277)
#define EFER_MSR (0xC0000080)
#define IA32_APIC_BASE (0x0000001B)

__attribute__((always_inline)) static __inline void outb(const uint16_t port, const uint8_t val) {
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(val));
}

__attribute__((always_inline)) static __inline uint8_t inb(const uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

__attribute__((always_inline)) static __inline void outw(const uint16_t port, const uint16_t val) {
    __asm__ volatile("outw %1, %0" : : "dN"(port), "a"(val));
}

__attribute__((always_inline)) static __inline uint16_t inw(const uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

__attribute__((always_inline)) static __inline void outl(const uint16_t port, const uint32_t val) {
    __asm__ volatile("outl %1, %0" : : "dN"(port), "a"(val));
}

__attribute__((always_inline)) static __inline uint32_t inl(const uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

__attribute__((always_inline)) static __inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = val;
    uint32_t hi = (val >> 32);
    __asm__ volatile("wrmsr" ::"a"(lo), "d"(hi), "c"(msr));
}

__attribute__((always_inline)) static __inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

__attribute__((always_inline)) static __inline void halt(void) {
    __asm__ volatile("hlt");
}

__attribute__((always_inline)) static __inline int cli(void) {
    uint64_t flags = 0;
    __asm__ volatile(
        "pushf\n\t"
        "popq %0\n\t"
        "cli\n\t"
        : "=r"(flags)
    );

    return (flags & 0x200);
}

__attribute__((always_inline)) static __inline void sti(int state) {
    if(state)
        __asm__ volatile("sti");
}


#define MAX(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a > _b ? _a : _b;                                                         \
  })

#define MIN(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a < _b ? _a : _b;                                                         \
  })

void debug_handle_trap();

#define KiB(x) (x * 1024ull)
#define MiB(x) (KiB(1) * 1024ull * x)
#define GiB(x) (uint64_t)(MiB(1) * 1024ull * x)
#define TiB(x) (uint64_t)(GiB(1) * 1024ull * x)

#define ALIGN(x, pot) (((x - 1) | (pot - 1)) + 1)
#define PAGE_ALIGN(x) ALIGN(x, KiB(4))

#define ALIGNED(x) __attribute__((aligned(x)))
#define UNUSED __attribute__((__unused__))
#define NONNULL __attribute__((nonnull))
#define PUBLIC __attribute__((visibility("default")))
#define PRIVATE __attribute__((visibility("hidden")))
#define PURE __attribute__((pure))
#define CONST __attribute__((const))
#define IS_NULL(x)                                                             \
  if (!x)                                                                      \
  debug_handle_trap()

#define SECTION(x) __attribute__((section(x)))
#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))
#define NAKED __attribute__((naked))
#define NULLABLE
#define NONNULL_RETURN __attribute__((returns_nonnull))
#define WEAK __attribute__((weak))

#define NO_UBSAN __attribute__((no_sanitize("undefined")))

#define SWAP_ENDIAN_16(x)                                                      \
  (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8))

#define SWAP_ENDIAN_32(x)                                                      \
  (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) |                        \
   ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24))
#define SWAP_ENDIAN_64(x)                                                      \
  ((SWAP_ENDIAN_32((x >> 32)) << 32) | (SWAP_ENDIAN_32(x & 0xFFFFFFFF)))


#define TO_LE_FRM_BE_64(x) SWAP_ENDIAN_64(x)
#define TO_LE_FRM_BE_32(x) SWAP_ENDIAN_32(x)
#define TO_LE_FRM_BE_16(x) SWAP_ENDIAN_16(x)

#define TO_BE_FRM_LE_64(x) SWAP_ENDIAN_64(x)
#define TO_BE_FRM_LE_32(x) SWAP_ENDIAN_32(x)
#define TO_BE_FRM_LE_16(x) SWAP_ENDIAN_16(x)

#if CUR_ENDIAN == LE_ENDIAN
#define TO_BE_64(x) SWAP_ENDIAN_64(x)
#define TO_BE_32(x) SWAP_ENDIAN_32(x)
#define TO_BE_16(x) SWAP_ENDIAN_16(x)

#define TO_LE_64(x) (x)
#define TO_LE_32(x) (x)
#define TO_LE_16(x) (x)
#else
#define TO_BE_64(x) (x)
#define TO_BE_32(x) (x)
#define TO_BE_16(x) (x)

#define TO_LE_64(x) SWAP_ENDIAN_64(x)
#define TO_LE_32(x) SWAP_ENDIAN_32(x)
#define TO_LE_16(x) SWAP_ENDIAN_16(x)
#endif

void print_str(const char *s);
#define DEBUG_ECHO(msg) print_str(__FILE__ "," S__LINE__ ":" msg "\r\n")
#define DEBUG_PRINT(msg) print_str(msg)

#if !defined(NDEBUG)
// First set the trap message, then raise the trap
void set_trap_str(const char *str);

#define WARN(msg) print_str(__FILE__ "," S__LINE__ ":" msg "\r\n")

#define PANIC(msg)                                                             \
  set_trap_str(__FILE__ "," S__LINE__ ":" msg "\r\n"), debug_handle_trap()

#define ASSERT(x, msg)                                                         \
  if (!(x))                                                                    \
  PANIC(msg)

#else
#define ASSERT(x, msg)
#define PANIC(msg) debug_handle_trap()
#endif /* end of include guard: _OS_TYPES_H_ */

#ifdef __cplusplus
}
#endif

#endif