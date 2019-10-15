// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBER_INTEL_HD_DEVICES_H
#define GUBER_INTEL_HD_DEVICES_H

#include "stddef.h"
#include "stdint.h"

#include "architectures.h"

typedef struct {
  char name[256];
  uint16_t device_id;
  uint8_t architecture;
} intel_gfx_device_t;

static intel_gfx_device_t igfx_devices[] = {{"Atom z8350", 0x22b0, IGFX_CHERRYTRAIL}, {"", 0, 0}};

void igfx_matchdevice(void);

#endif