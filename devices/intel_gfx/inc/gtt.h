// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBER_IGFX_GTT_H
#define GUBER_IGFX_GTT_H

#include "driver.h"

void igfx_gtt_init(igfx_driver_t *driver);

uint64_t igfx_gtt_alloc(size_t sz);

void igfx_gtt_free(uint64_t addr, size_t sz);

#endif