// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBER_IGFX_GMBUS_H
#define GUBER_IGFX_GMBUS_H

#include "driver.h"

#define IGFX_GMBUS0 0x5100
#define IGFX_GMBUS1 0x5104
#define IGFX_GMBUS2 0x5108
#define IGFX_GMBUS3 0x510C
#define IGFX_GMBUS4 0x5110

void igfx_gmbus_init(igfx_driver_t* driver_state);

void igfx_gmbus_read(igfx_driver_t* driver, uint32_t disp_idx, uint8_t offset, uint16_t sz, uint8_t *buf);

#endif