/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"
#include "types.h"

#include "debug.h"

#include "acpi/tables.h"
#include "boot_info.h"
#include "cpuid.h"
#include "fpu.h"
#include "interrupts.h"
#include "memory.h"

static void spurious_irq_handler(int int_num) { int_num = 0; }

static void pagefault_handler(int int_num) {
  int_num = 0;

  interrupt_register_state_t reg_state;
  interrupt_getregisterstate(&reg_state);

  print_str("Page fault at: ");
  print_uint64(reg_state.rip, BASE_HEX);
  halt();
}

void setup_core(void) {
  acpi_init(); // Init ACPI

  tls_init();  // Setup the TLS
  pmem_init(); // Setup physical memory
  vmem_init(); // Setup virtual memory

  pic_fini(); // Disable PIC
  gdt_init(); // Setup GDT
  idt_init(); // Setup IDT

  int irq0 = 0x27;
  int irq1 = 0x2f;
  int pf_intr = 0x0e;
  interrupt_allocate(1, interrupt_flags_exclusive, &irq0);
  interrupt_allocate(1, interrupt_flags_exclusive, &irq1);
  interrupt_allocate(1, interrupt_flags_exclusive, &pf_intr);

  interrupt_registerhandler(irq0, spurious_irq_handler);
  interrupt_registerhandler(irq1, spurious_irq_handler);
  interrupt_registerhandler(pf_intr, pagefault_handler);

  acpi_intr_init(); // Initialize IOAPIC + LAPIC from acpi tables
  ioapic_init();
  apic_init();

  fp_platform_init(); // Setup FPU

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