/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stdint.h"
#include "stdlib.h"
#include "types.h"

#include "debug.h"

#include "acpi/mcfg.h"
#include "acpi/tables.h"
#include "memory.h"
#include "pci.h"

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

static pci_device_t *device_set;
static int device_count;

static uint32_t pci_read_dword(uint32_t bus, uint32_t device, uint32_t function,
                               uint32_t offset) {
  outl(PCI_ADDR,
       0x80000000 | bus << 16 | device << 11 | function << 8 | (offset & 0xfc));
  return inl(PCI_DATA);
}

static int pci_get_next_device(uint32_t *bus, uint32_t *device) {
  uint32_t b = *bus;
  uint32_t d = *device;

  while (b < 256) {
    for (; d < 32; d++) {
      uint32_t v_id = pci_read_dword(b, d, 0, 0);
      if (v_id != 0xFFFFFFFF) {
        *bus = b;
        *device = d;
        return 0;
      }
    }
    if (d >= 32) {
      d = 0;
      b++;
    }
  }
  return -1;
}

static uint32_t pci_get_func_count(uint32_t bus, uint32_t device) {
  uint32_t hdrType = pci_read_dword(bus, device, 0, 0x0C);
  if ((hdrType >> 23) & 1)
    return 8;
  return 1;
}

static void pci_get_device(uint32_t bus, uint32_t device, uint32_t function,
                           pci_device_t *devInfo) {
  devInfo->ClassCode = pci_read_dword(bus, device, function, 8) >> 24;
  devInfo->SubClassCode =
      (pci_read_dword(bus, device, function, 8) >> 16) & 0xFF;
  devInfo->ProgIF = (pci_read_dword(bus, device, function, 8) >> 8) & 0xFF;
  devInfo->HdrType = (pci_read_dword(bus, device, function, 0x0C) >> 16) & 0xFF;

  devInfo->DeviceID =
      (uint16_t)(pci_read_dword(bus, device, function, 0) >> 16);
  devInfo->VendorID = (uint16_t)pci_read_dword(bus, device, function, 0);

  devInfo->Bus = bus;
  devInfo->Device = device;
  devInfo->Function = function;

  if (devInfo->HdrType == 0)
    devInfo->BarCount = 6;
  else
    devInfo->BarCount = 2;
}

uint64_t pci_parsebar(pci_config_t *device, int idx) {
  if (idx % 2 == 0 && (device->bar[idx] & 0x4) == 0x4) {
    return ((uint64_t)device->bar[idx + 1] << 32) |
           (device->bar[idx] & 0xfffffff0);
  }
  return device->bar[idx] & 0xfffffff0;
}

uint64_t pci_getbarsize(pci_config_t *device, int idx) {
  uint32_t orig_val = device->bar[idx];
  device->bar[idx] = 0xffffffff;
  size_t sz = ~device->bar[idx] + 1;
  device->bar[idx] = orig_val;
  return sz;
}

bool pci_isbario(pci_config_t *device, int idx) { return device->bar[idx] & 1; }

int pci_getmsiinfo(pci_config_t *device, int *cnt) {
  // Search cap list
  bool hasmsi = false;
  bool hasmsix = false;
  if (device->capabilitiesPtr != 0) {
    uint8_t ptr = device->capabilitiesPtr;
    uint8_t *pci_base = (uint8_t *)device;

    do {
      pci_cap_header_t *capEntry = (pci_cap_header_t *)(pci_base + ptr);

      if (capEntry->capID == pci_cap_msi) {
        hasmsi = true;

        pci_msi_32_t *msi_space = (pci_msi_32_t *)capEntry;
        *cnt = 1 << msi_space->ctrl.requested_vector_num;

      } else if (capEntry->capID == pci_cap_msix) {
        hasmsi = true;
        hasmsix = true;

        pci_msix_t *msi_space = (pci_msix_t *)capEntry;
        *cnt = msi_space->ctrl.table_sz + 1;
      }
      ptr = capEntry->nextPtr;
    } while (ptr != 0);
  }

  if (hasmsix)
    return 1;
  if (hasmsi)
    return 0;
  return -1;
}

int pci_setmsiinfo(pci_config_t *device, int msix, uintptr_t *msi_addr,
                   uint32_t *msi_msg, int cnt) {
  // Search cap list
  if (device->capabilitiesPtr != 0) {
    uint8_t ptr = device->capabilitiesPtr;
    uint8_t *pci_base = (uint8_t *)device;

    do {
      pci_cap_header_t *capEntry = (pci_cap_header_t *)(pci_base + ptr);

      if (capEntry->capID == pci_cap_msi && msix == 0) {

        pci_msi_32_t *msi_space = (pci_msi_32_t *)capEntry;
        if (msi_space->ctrl.support_64bit) {
          pci_msi_64_t *msi64_space = (pci_msi_64_t *)capEntry;
          msi64_space->msg_addr = (uint32_t)*msi_addr;
          msi64_space->msg_addr_hi = (uint32_t)(*msi_addr >> 32);
          msi64_space->msg_data = *msi_msg;
        } else {
          msi_space->msg_addr = (uint32_t)*msi_addr;
          msi_space->msg_data = *msi_msg;
        }

        uint32_t coded_vecnum = 0;
        for (int i = 30; i >= 0; i--)
          if (cnt & (1 << i)) {
            coded_vecnum = i;
            break;
          }

        msi_space->ctrl.avail_vector_num = coded_vecnum;
        msi_space->ctrl.enable = 1;

      } else if (capEntry->capID == pci_cap_msix && msix == 1) {

        pci_msix_t *msi_space = (pci_msix_t *)capEntry;

        if (cnt != 1 && (msi_space->ctrl.table_sz + 1) != cnt)
          return -1;

        // get the specified bar, get its virtual address
        uint64_t bar = 0;
        for (int i = 0; i < 6; i++) {
          if ((device->bar[i] & 0x6) == 0x4) // Is 64-bit
            bar = (device->bar[i] & 0xFFFFFFF0) +
                  ((uint64_t)device->bar[i + 1] << 32);
          else if ((device->bar[i] & 0x6) == 0x0) // Is 32-bit
            bar = (device->bar[i] & 0xFFFFFFF0);
          if (i == msi_space->table_off.bir)
            break;
        }

        uint32_t *table = (uint32_t *)vmem_phystovirt(
            (intptr_t)(bar + (msi_space->table_off.offset << 3)), KiB(4),
            vmem_flags_uncached | vmem_flags_kernel | vmem_flags_rw);

        // fill the offset table
        for (int i = 0; i <= msi_space->ctrl.table_sz; i++) {
          table[i * 4 + 0] = (uint32_t)msi_addr[cnt == 1 ? 0 : i];
          table[i * 4 + 1] = (uint32_t)(msi_addr[cnt == 1 ? 0 : i] >> 32);
          table[i * 4 + 2] = msi_msg[cnt == 1 ? 0 : i];
          table[i * 4 + 3] &= ~1;
        }

        msi_space->ctrl.func_mask = 0;
        msi_space->ctrl.enable = 1;
        //__asm__("cli\n\thlt" :: "a"(table));
      }
      ptr = capEntry->nextPtr;
    } while (ptr != 0);
  }

  return 0;
}

int get_pcidevice_count() { return device_count; }

pci_device_t *get_pcidevice(int idx) { return &device_set[idx]; }

void pci_reg_init(void) {
  uint32_t bus = 0;
  uint32_t device = 0;
  uint32_t idx = 0;
  device_count = 0;

  // Find the ECAM address if possible
  MCFG *mcfg = acpi_tables_find_table(MCFG_SIG);
  uint32_t len = mcfg->h.Length - 8 - sizeof(ACPISDTHeader);

  // Get device count
  while (1) {
    if (pci_get_next_device(&bus, &device) != 0)
      break;
    uint32_t funcs = pci_get_func_count(bus, device);
    for (uint32_t f = 0; f < funcs; f++) {
      pci_device_t devInfo;
      pci_get_device(bus, device, f, &devInfo);
      if ((devInfo.VendorID == 0xffff) | (devInfo.DeviceID == 0xffff))
        continue;
      device_count++;
      print_str("PCI[");
      print_uint8((uint8_t)devInfo.Bus, BASE_HEX);
      print_str(":");
      print_uint8((uint8_t)devInfo.Device, BASE_HEX);
      print_str(".");
      print_uint8((uint8_t)devInfo.Function, BASE_HEX);
      print_str("] ");
      print_uint32(devInfo.VendorID, BASE_HEX);
      print_str(":");
      print_uint32(devInfo.DeviceID, BASE_HEX);
      print_str("\r\n");
    }
    device++;
  }
  device_set = malloc(sizeof(pci_device_t) * device_count);

  // Store device info
  bus = 0;
  device = 0;
  while (1) {
    if (pci_get_next_device(&bus, &device) != 0)
      break;
    uint32_t funcs = pci_get_func_count(bus, device);
    for (uint32_t f = 0; f < funcs; f++) {
      pci_get_device(bus, device, f, &device_set[idx]);
      if ((device_set[idx].VendorID == 0xffff) |
          (device_set[idx].DeviceID == 0xffff))
        continue;

      device_set[idx].ecam_addr = NULL;
      for (uint32_t mcfg_idx = 0; mcfg_idx < len / sizeof(MCFG_Entry);
           mcfg_idx++)
        if (mcfg->entries[mcfg_idx].start_bus_number <= bus &&
            mcfg->entries[mcfg_idx].end_bus_number >= bus) {
          uint64_t ecam_addr =
              mcfg->entries[mcfg_idx].baseAddr +
              (((bus - mcfg->entries[mcfg_idx].start_bus_number) << 20) |
               (device << 15) | (f << 12));

          device_set[idx].ecam_addr = (uint8_t *)ecam_addr;
          break;
        }
      idx++;
    }
    device++;
  }
}