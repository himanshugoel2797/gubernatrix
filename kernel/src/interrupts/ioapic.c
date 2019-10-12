/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"

#include "debug.h"
#include "interrupts.h"
#include "memory.h"

#include "apic_data.h"

typedef struct {
  uint32_t id;
  uint32_t global_intr_base;
  uint32_t volatile *base_addr;
} ioapic_t;

static ioapic_t *ioapics;
static int ioapic_cnt;

static void ioapic_write(int idx, uint32_t off, uint32_t val) {
  ioapics[idx].base_addr[0] = off;
  ioapics[idx].base_addr[4] = val;
}

static uint32_t ioapic_read(int idx, uint32_t off) {
  ioapics[idx].base_addr[0] = off;
  return ioapics[idx].base_addr[4];
}

static void ioapic_map(uint32_t idx, uint32_t irq_pin, uint32_t irq,
                       bool active_low, bool level_trigger) {

  // configure this override
  const uint32_t low_index = 0x10 + irq_pin * 2;
  const uint32_t high_index = 0x10 + irq_pin * 2 + 1;

  uint32_t high = ioapic_read(idx, high_index);
  high &= ~0xff000000;
  // high |= (0xff000000);
  // bsp is the destination
  high |= (interrupt_get_cpuidx() << 24);
  ioapic_write(idx, high_index, high);

  uint32_t low = ioapic_read(idx, low_index);

  // set the polarity
  low &= ~(1 << 13);
  low |= ((active_low & 1) << 13);

  low &= ~(1 << 15);
  low |= ((level_trigger & 1) << 15);

  // set delivery vector
  low &= ~0xff;
  low |= irq & 0xff;

  // set to fixed destination mode
  low &= ~(1 << 11);
  low |= (0 << 11);

  // set to fixed delivery mode
  low &= ~0x700;
  low |= 0 << 8;

  // unmask the interrupt
  low &= ~(1 << 16);

  ioapic_write(idx, low_index, low);
}

static void ioapic_setmask(uint32_t idx, uint32_t irq_pin, bool mask) {
  // configure this override
  const uint32_t low_index = 0x10 + irq_pin * 2;
  uint32_t low = ioapic_read(idx, low_index);

  // unmask the interrupt
  low &= ~(1 << 16);
  if (mask)
    low |= (1 << 16);

  ioapic_write(idx, low_index, low);
}

void interrupt_mapinterrupt(uint32_t line, int irq, bool active_low,
                            bool level_trig) {
  int ioapic_idx = 0;
  uint32_t ioapic_close_intr_base = 0;

  for (int i = 0; i < ioapic_cnt; i++)
    if (ioapics[i].global_intr_base < line &&
        ioapics[i].global_intr_base > ioapic_close_intr_base) {
      ioapic_close_intr_base = ioapics[i].global_intr_base;
      ioapic_idx = i;
    }

  ioapic_map(ioapic_idx, line, irq, active_low, level_trig);
}

void interrupt_setmask(uint32_t line, bool mask) {
  int ioapic_idx = 0;
  uint32_t ioapic_close_intr_base = 0;

  for (int i = 0; i < ioapic_cnt; i++)
    if (ioapics[i].global_intr_base < line &&
        ioapics[i].global_intr_base > ioapic_close_intr_base) {
      ioapic_close_intr_base = ioapics[i].global_intr_base;
      ioapic_idx = i;
    }

  ioapic_setmask(ioapic_idx, line, mask);
}

void ioapic_init(void) {
  // Read the registry
  ioapic_cnt = get_ioapic_count();
  ioapics = malloc(sizeof(ioapic_t) * ioapic_cnt);

  for (int i = 0; i < ioapic_cnt; i++) {
    ioapic_info_t *ioapic_info = get_ioapic_info(i);

    uint64_t id = ioapic_info->io_apic_id;
    uint64_t base_addr = ioapic_info->io_apic_base_addr;
    uint64_t intr_base = ioapic_info->global_sys_int_base;

    ioapics[i].id = (uint32_t)id;
    ioapics[i].base_addr = (uint32_t volatile *)vmem_phystovirt(
        base_addr, KiB(8), vmem_flags_uncached);
    ioapics[i].global_intr_base = (uint32_t)intr_base;

    // Configure the detected overrides
    int available_redirs = (int)((ioapic_read(i, 0x01) >> 16) & 0xff) + 1;

    for (int j = 0; j < available_redirs; j++) {
      ioapic_map(i, j, j + intr_base + 0x20, false, false);
      ioapic_setmask(i, j, true);
    }

    for (int j = 0; j < ioapic_info->isaovr_cnt; j++) {
      uint64_t glbl_sys_int = ioapic_info->isaovrs[j].global_sys_int;
      uint8_t irq = ioapic_info->isaovrs[j].irq_src;
      uint64_t bus = ioapic_info->isaovrs[j].bus_src;
      bool active_low = ioapic_info->isaovrs[j].active_low;
      bool level_trigger = ioapic_info->isaovrs[j].level_trigger;

      print_str("ISA Override Mapping: ");
      print_uint8(irq, BASE_HEX);
      print_str("=>");
      print_uint32(glbl_sys_int, BASE_HEX);
      if (active_low)
        print_str(":active_low");
      if (level_trigger)
        print_str(":level_trigger");
      print_str("\r\n");

      ioapic_map(i, glbl_sys_int - intr_base, irq + 0x20, active_low,
                 level_trigger);
      ioapic_setmask(i, glbl_sys_int - intr_base, true);

      int irq_num = irq + 0x20;
      // if(interrupt_allocate(1, interrupt_flags_exclusive |
      // interrupt_flags_fixed, &irq_num) != 0)
      //    PANIC("Failed to reserve specified interrupt.");
    }
  }
}