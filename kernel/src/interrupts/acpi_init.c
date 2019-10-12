/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "types.h"

#include "debug.h"

#include "acpi/madt.h"
#include "acpi/tables.h"

#include "apic_data.h"

static lapic_info_t *lapic_data = NULL;
static ioapic_info_t *ioapic_data = NULL;
static int lapic_cnt = 0;
static int ioapic_cnt = 0;

int get_lapic_count(void) { return lapic_cnt; }
int get_ioapic_count(void) { return ioapic_cnt; }

lapic_info_t *get_lapic_info(int idx) {
  if (idx < lapic_cnt)
    return &lapic_data[idx];
  return NULL;
}

ioapic_info_t *get_ioapic_info(int idx) {
  if (idx < ioapic_cnt)
    return &ioapic_data[idx];
  return NULL;
}

static void save_lapic(uint32_t idx, MADT_EntryLAPIC *lapic) {
  lapic_data[idx].apic_id = lapic->apic_id;
  lapic_data[idx].processor_id = lapic->processor_id;
}

static void save_ioapic(uint32_t idx, MADT_EntryIOAPIC *ioapic) {
  ioapic_data[idx].global_sys_int_base = ioapic->global_sys_int_base;
  ioapic_data[idx].io_apic_base_addr = ioapic->io_apic_base_addr;
  ioapic_data[idx].io_apic_id = ioapic->io_apic_id;
  ioapic_data[idx].isaovr_cnt = 0;
  ioapic_data[idx].isaovr_idx = 0;
  ioapic_data[idx].isaovrs = NULL;
}

static void save_isaovr(MADT_EntryISAOVR *isaovr) {
  int idx = 0;
  for (int i = 0; i < ioapic_cnt; i++) {
    if (isaovr->global_sys_int >= ioapic_data[i].global_sys_int_base &&
        (isaovr->global_sys_int - ioapic_data[i].global_sys_int_base) <=
            (uint32_t)idx)
      idx = i;
  }

  if (ioapic_data[idx].isaovrs == NULL)
    ioapic_data[idx].isaovrs =
        malloc(sizeof(isaovr_info_t) * ioapic_data[idx].isaovr_cnt);

  int isaovr_idx = ioapic_data[idx].isaovr_idx++;

  ioapic_data[idx].isaovrs[isaovr_idx].global_sys_int = isaovr->global_sys_int;
  ioapic_data[idx].isaovrs[isaovr_idx].bus_src = isaovr->bus_src;
  ioapic_data[idx].isaovrs[isaovr_idx].irq_src = isaovr->irq_src;

  ioapic_data[idx].isaovrs[isaovr_idx].active_low =
      isaovr->flags & 2 ? true : false;
  ioapic_data[idx].isaovrs[isaovr_idx].level_trigger =
      isaovr->flags & 8 ? true : false;
}

static void isaovr_count_update(MADT_EntryISAOVR *isaovr) {
  int ioapic_idx = 0;
  for (int i = 0; i < ioapic_cnt; i++) {
    if (isaovr->global_sys_int >= ioapic_data[i].global_sys_int_base &&
        (isaovr->global_sys_int - ioapic_data[i].global_sys_int_base) <=
            (uint32_t)ioapic_idx)
      ioapic_idx = i;
  }

  ioapic_data[ioapic_idx].isaovr_cnt++;
}

void acpi_intr_init(void) {
  MADT *madt = acpi_tables_find_table(MADT_SIG);
  if (madt == NULL)
    PANIC("MADT missing!");

  uint32_t len = madt->h.Length - 8 - sizeof(ACPISDTHeader);
  uint32_t lapic_idx = 0;
  uint32_t ioapic_idx = 0;

  // Count LAPIC and IOAPIC entries
  for (uint32_t i = 0; i < len;) {
    MADT_EntryHeader *hdr = (MADT_EntryHeader *)&madt->entries[i];

    switch (hdr->type) {
    case MADT_LAPIC_ENTRY_TYPE:
      lapic_cnt++;
      break;
    case MADT_IOAPIC_ENTRY_TYPE:
      ioapic_cnt++;
      break;
    }

    i += hdr->entry_size;
    if (hdr->entry_size == 0)
      i += 8;
  }

  print_str("LAPIC Count: ");
  print_int32(lapic_cnt, BASE_HEX);
  print_str(" IOAPIC Count: ");
  print_int32(ioapic_cnt, BASE_HEX);
  print_str("\r\n");

  lapic_data = malloc(sizeof(lapic_info_t) * lapic_cnt);
  ioapic_data = malloc(sizeof(ioapic_info_t) * ioapic_cnt);

  // Apply LAPICs and IOAPICs
  for (uint32_t i = 0; i < len;) {
    MADT_EntryHeader *hdr = (MADT_EntryHeader *)&madt->entries[i];

    switch (hdr->type) {
    case MADT_LAPIC_ENTRY_TYPE:
      save_lapic(lapic_idx++, (MADT_EntryLAPIC *)hdr);
      break;
    case MADT_IOAPIC_ENTRY_TYPE:
      save_ioapic(ioapic_idx++, (MADT_EntryIOAPIC *)hdr);
      break;
    }

    i += hdr->entry_size;
    if (hdr->entry_size == 0)
      i += 8;
  }

  // ISA override count
  for (uint32_t i = 0; i < len;) {
    MADT_EntryHeader *hdr = (MADT_EntryHeader *)&madt->entries[i];

    if (hdr->type == MADT_ISAOVER_ENTRY_TYPE)
      isaovr_count_update((MADT_EntryISAOVR *)hdr);

    i += hdr->entry_size;
    if (hdr->entry_size == 0)
      i += 8;
  }

  // Apply ISA overrides
  for (uint32_t i = 0; i < len;) {
    MADT_EntryHeader *hdr = (MADT_EntryHeader *)&madt->entries[i];

    if (hdr->type == MADT_ISAOVER_ENTRY_TYPE)
      save_isaovr((MADT_EntryISAOVR *)hdr);

    i += hdr->entry_size;
    if (hdr->entry_size == 0)
      i += 8;
  }
}

uint32_t msi_register_addr(int cpu_idx) {
  return 0xFEE00000 | (cpu_idx & 0xff) << 12 | (1 << 3) |
         (0 << 2); // fixed destination mode
}

uint64_t msi_register_data(int vec) { return (vec & 0xff); }