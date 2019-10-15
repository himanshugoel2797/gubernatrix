// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBER_INTEL_HD_DRIVER_H
#define GUBER_INTEL_HD_DRIVER_H

#include "stddef.h"
#include "stdint.h"

#include "pci.h"
#include "edid.h"

#define IGFX_CHERRYTRAIL_DISP_BASE 0x180000
#define IGFX_HASWELL_DISP_BASE 0xC0000
#define IGFX_IRONLAKE_DISP_BASE 0xC0000

#define IGFX_CHERRYTRAIL_GTT_BASE 0x800000

typedef struct {
  bool hotplug;
  bool connected;
  edid_t edid;
  uint32_t standard_mode;   //bit map that matches the edid standard timings
  int32_t custom_mode_idx;  //index into the edid detailed modes, -1 for standard mode
} igfx_display_info_t;

typedef struct {
  int igfx_device_index;
  uint16_t bus;
  uint16_t device;
  uint8_t architecture;
  pci_config_t *config_space;
  uint8_t *mmio_base;
  uint32_t display_mmio_base;

  // GTT
  uint32_t gtt_base;

  // Display
  uint32_t display_count;
  igfx_display_info_t *displays;
} igfx_driver_t;

static inline void igfx_write32(igfx_driver_t *driver, uint32_t offset,
                                uint32_t value) {
  *(uint32_t *)&driver->mmio_base[offset] = value;
}

static inline uint32_t igfx_read32(igfx_driver_t *driver, uint32_t offset) {
  return *(uint32_t *)&driver->mmio_base[offset];
}

#endif