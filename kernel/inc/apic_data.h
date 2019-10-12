// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_APIC_DATA_H
#define GUBERNATRIX_APIC_DATA_H

#include "stddef.h"
#include "stdint.h"
#include "types.h"

typedef struct {
  uint8_t processor_id;
  uint8_t apic_id;
} lapic_info_t;

typedef struct {
  uint8_t bus_src;
  uint8_t irq_src;
  uint32_t global_sys_int;
  bool active_low;
  bool level_trigger;
} isaovr_info_t;

typedef struct {
  uint8_t io_apic_id;
  uint32_t io_apic_base_addr;
  uint32_t global_sys_int_base;
  int isaovr_cnt;
  int isaovr_idx;
  isaovr_info_t* isaovrs;
} ioapic_info_t;

int get_lapic_count(void);
int get_ioapic_count(void);

lapic_info_t* get_lapic_info(int idx);
ioapic_info_t* get_ioapic_info(int idx);

#endif