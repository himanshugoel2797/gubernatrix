/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "bootstrap_malloc.h"
#include "stddef.h"
#include "stdint.h"

#include "debug.h"

#define TLS_SIZE ((int)KiB(16))
#define GS_BASE_MSR (0xC0000101)
#define KERNEL_GS_BASE_MSR (0xC0000102)

static TLS uint64_t *tls_mem;
static uint64_t tls_alloc_off;

void tls_init(void) {
  uint8_t *tls_block = bootstrap_malloc(TLS_SIZE);
  tls_mem = NULL;
  tls_alloc_off = 0;
  wrmsr(GS_BASE_MSR, (uint64_t)tls_block);
}

TLS void *tls_alloc(size_t sz) {
  sz = ALIGN(sz, sizeof(uint64_t));
  if (tls_alloc_off + sz < TLS_SIZE) {
    TLS uint64_t *alloc_off = (TLS uint64_t *)tls_alloc_off;
    tls_alloc_off += sz;
    return (TLS void *)alloc_off;
  }

  PANIC("TLS Allocation Failed!");
  return NULL;
}