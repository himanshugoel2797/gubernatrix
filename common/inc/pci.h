// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_PCI_H
#define GUBERNATRIX_PCI_H

#include "memory.h"
#include "stddef.h"
#include "stdint.h"

typedef struct {
  uint32_t ClassCode;
  uint32_t SubClassCode;
  uint32_t ProgIF;

  uint32_t HdrType;

  uint32_t DeviceID;
  uint32_t VendorID;

  uint32_t BarCount;

  uint32_t Bus;
  uint32_t Device;
  uint32_t Function;

  uint8_t *ecam_addr;
  uint8_t **bars;
} pci_device_t;

typedef struct {
  uint16_t unkn0 : 1;
  uint16_t mem_space : 1;
  uint16_t busmaster : 1;
  uint16_t unkn1 : 7;
  uint16_t int_disable : 1;
} pci_command_reg_t;

typedef struct PACKED {
  uint16_t vendorID;
  uint16_t deviceID;
  pci_command_reg_t command;
  uint16_t status;
  uint32_t revisionID : 8;
  uint32_t classCode : 24;
  uint8_t cacheLine;
  uint8_t masterLatency;
  uint8_t headerType;
  uint8_t bist;
  uint32_t bar[6];
  uint32_t cardbus;
  uint16_t subsystem_vendor;
  uint16_t subsystem;
  uint32_t expansion_rom;
  uint32_t capabilitiesPtr : 8;
  uint32_t rsv0 : 24;
} pci_config_t;

typedef enum {
  pci_cap_pwm = 0x01,
  pci_cap_msi = 0x05,
  pci_cap_vendor = 0x09,
  pci_cap_msix = 0x11,
} pci_cap_ids_t;

typedef struct PACKED {
  uint8_t capID;
  uint8_t nextPtr;
  uint8_t data[0];
} pci_cap_header_t;

typedef struct {
  uint16_t enable : 1;
  uint16_t requested_vector_num : 3;
  uint16_t avail_vector_num : 3;
  uint16_t support_64bit : 1;
  uint16_t support_vectormask : 1;
} pci_msi_control_t;

typedef struct {
  pci_cap_header_t hdr;
  pci_msi_control_t ctrl;
  uint32_t msg_addr;
  uint16_t msg_data;
  uint16_t rsv;
} pci_msi_32_t;

typedef struct {
  pci_cap_header_t hdr;
  pci_msi_control_t ctrl;
  uint32_t msg_addr;
  uint32_t msg_addr_hi;
  uint16_t msg_data;
  uint16_t rsv;
} pci_msi_64_t;

typedef struct {
  uint16_t table_sz : 11;
  uint16_t rsv0 : 3;
  uint16_t func_mask : 1;
  uint16_t enable : 1;
} pci_msix_control_t;

typedef struct {
  uint32_t bir : 3;
  uint32_t offset : 29;
} pci_msix_entry_t;

typedef struct {
  pci_cap_header_t hdr;
  pci_msix_control_t ctrl;
  pci_msix_entry_t table_off;
  pci_msix_entry_t pba_off;
} pci_msix_t;

int get_pcidevice_count(void);
pci_device_t *get_pcidevice(int idx);
void pci_reg_init(void);
uint64_t pci_parsebar(pci_config_t *device, int idx);
bool pci_isbario(pci_config_t *device, int idx);
uint64_t pci_getbarsize(pci_config_t *device, int idx);
int pci_getmsiinfo(pci_config_t *device, int *cnt);
int pci_setmsiinfo(pci_config_t *device, int msix, uintptr_t *msi_addr,
                   uint32_t *msi_msg, int cnt);

#endif