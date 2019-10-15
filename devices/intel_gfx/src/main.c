/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"

#include "memory.h"
#include "pci.h"

#include "devices.h"
#include "driver.h"

#include "display.h"
#include "gmbus.h"

#include "debug.h"

static igfx_driver_t driver_instance;

void igfx_matchdevice(void) {
  bool device_found = false;

  int dev_count = get_pcidevice_count();

  print_str("IGFX Searching Count: ");
  print_int32(dev_count, BASE_HEX);
  print_str("\r\n");

  for (int dev_idx = 0; dev_idx < dev_count; dev_idx++) {
    pci_device_t *device = get_pcidevice(dev_idx);
    int igfx_iter = 0;
    do {
      if (device->VendorID == 0x8086 &&
          device->DeviceID == igfx_devices[igfx_iter].device_id) {
        driver_instance.igfx_device_index = igfx_iter;
        driver_instance.bus = device->Bus;
        driver_instance.device = device->Device;
        driver_instance.architecture = igfx_devices[igfx_iter].architecture;
        driver_instance.config_space = (pci_config_t *)vmem_phystovirt(
            (intptr_t)device->ecam_addr, KiB(4),
            vmem_flags_uncached | vmem_flags_kernel | vmem_flags_rw);
        driver_instance.mmio_base = (uint8_t *)vmem_phystovirt(
            (intptr_t)pci_parsebar(driver_instance.config_space, 0),
            pci_getbarsize(driver_instance.config_space, 0),
            vmem_flags_uncached | vmem_flags_kernel | vmem_flags_rw);
        device_found = true;

        print_str("igfx driver loaded for: ");
        print_str(igfx_devices[igfx_iter].name);
        print_str("\r\n");
        break;
      }
    } while (igfx_devices[igfx_iter++].device_id != 0);
    if (device_found)
      break;
  }

  if (device_found) {
    // initialize device
    driver_instance.config_space->command.busmaster = 1;
    if (driver_instance.architecture == IGFX_CHERRYTRAIL){
      driver_instance.display_mmio_base = IGFX_CHERRYTRAIL_DISP_BASE;
      driver_instance.gtt_base = IGFX_CHERRYTRAIL_GTT_BASE;
    }
    igfx_gmbus_init(&driver_instance);
    igfx_display_init(&driver_instance);
    // TODO: setup msi interrupts
    // detect displays
    // read edid
    // setup display pipes
  }
}