// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_PRIV_TIMERS_H
#define GUBERNATRIX_PRIV_TIMERS_H

#include "stddef.h"

PRIVATE int hpet_getcount();

PRIVATE int hpet_init();
PRIVATE int pit_init();
PRIVATE int rtc_init();
PRIVATE int apic_timer_init();
PRIVATE int apic_timer_tsc_init();

PRIVATE bool use_tsc();
PRIVATE int tsc_init();
PRIVATE int tsc_mp_init();

#endif