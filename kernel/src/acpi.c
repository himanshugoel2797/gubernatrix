/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "acpi/fadt.h"
#include "acpi/hpet.h"
#include "acpi/madt.h"
#include "acpi/mcfg.h"
#include "acpi/tables.h"

#include "boot_info.h"
#include "debug.h"
#include "memory.h"

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"

static RSDPDescriptor20 *rsdp;

void acpi_init(void) {
  RSDPDescriptor20 *l_rsdp = (RSDPDescriptor20 *)get_bootinfo()->RSDPAddress;

  // Copy the rsdp
  rsdp = malloc(sizeof(RSDPDescriptor20));
  memcpy(rsdp, l_rsdp, sizeof(RSDPDescriptor20));
}

bool acpi_tables_validate_csum(ACPISDTHeader *header) {
  uint8_t sum = 0;
  for (uint32_t i = 0; i < header->Length; i++) {
    sum += ((char *)header)[i];
  }

  return sum == 0;
}

void *acpi_tables_find_table(const char *table_name) {
  if (rsdp == NULL)
    return NULL;

  if (rsdp->firstPart.Revision != ACPI_VERSION_1 && rsdp->XsdtAddress) {
    print_str("RSDP Ver 2\r\n");
    XSDT *xsdt = (XSDT *)vmem_phystovirt((intptr_t)rsdp->XsdtAddress, MiB(2),
                                         vmem_flags_cachewriteback);
    if (!acpi_tables_validate_csum((ACPISDTHeader *)xsdt))
      return (void *)-1;

    int entries = XSDT_GET_POINTER_COUNT((xsdt->h));
    print_str("Entry Count: ");
    print_int32(entries, BASE_HEX);
    print_str("\r\n");

    for (int i = 0; i < entries; i++) {
      if (xsdt->PointerToOtherSDT[i] == 0)
        continue;
      ACPISDTHeader *h =
          (ACPISDTHeader *)vmem_phystovirt((intptr_t)xsdt->PointerToOtherSDT[i],
                                           MiB(2), vmem_flags_cachewriteback);
      if (!memcmp(h->Signature, table_name, 4) &&
          acpi_tables_validate_csum(h)) {
        return (void *)h;
      }
    }
  } else if ((rsdp->firstPart.Revision == ACPI_VERSION_1) |
             (!rsdp->XsdtAddress)) {
    print_str("RSDP Ver 1\r\n");
    RSDT *rsdt = (RSDT *)vmem_phystovirt((intptr_t)rsdp->firstPart.RsdtAddress,
                                         MiB(2), vmem_flags_cachewriteback);

    if (!acpi_tables_validate_csum((ACPISDTHeader *)rsdt))
      return NULL;

    int entries = RSDT_GET_POINTER_COUNT((rsdt->h));
    print_str("Entry Count: ");
    print_int32(entries, BASE_HEX);
    print_str("\r\n");

    for (int i = 0; i < entries; i++) {
      ACPISDTHeader *h =
          (ACPISDTHeader *)vmem_phystovirt((intptr_t)rsdt->PointerToOtherSDT[i],
                                           MiB(2), vmem_flags_cachewriteback);
      if (!memcmp(h->Signature, table_name, 4) &&
          acpi_tables_validate_csum(h)) {
        return (void *)h;
      }
    }
  }

  print_str("Table not found.");
  return NULL;
}