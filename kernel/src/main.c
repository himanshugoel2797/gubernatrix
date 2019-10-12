/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "boot_info.h"
#include "cpuid.h"
#include "debug.h"
#include "memory.h"
#include "stddef.h"
#include "stdint.h"
#include "types.h"

void setup_core(void) {
  tls_init();  // Setup the TLS
  pmem_init(); // Setup physical memory
  vmem_init(); //Setup virtual memory
  // Setup GDT
  // Setup IOAPIC
  // Setup LAPIC
  // Setup IDT
  // Setup TSS
  // Setup Timers
}

SECTION(".entry_point") int32_t main(void *param, uint64_t magic) {

  debug_init();
  set_bootinfo(param, magic);
  cpuid_init();

  setup_core();

  print_str("Initialized\r\n");
  halt();
  return 0;
}