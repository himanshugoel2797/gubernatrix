/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "string.h"

#include "cpuid.h"
#include "priv_timers.h"
#include "timer.h"

PRIVATE bool use_tsc() {
  bool tsc_valid = get_cpuid()->tsc_valid;
  bool tsc_deadline = get_cpuid()->tsc_deadline;
  bool tsc_invar = get_cpuid()->tsc_invar;
  uint64_t tsc_freq = get_cpuid()->tsc_freq;
  uint64_t apic_freq = get_cpuid()->apic_freq;

  // return true;
  return (tsc_valid && tsc_deadline && tsc_invar && tsc_freq != 0 &&
          apic_freq != 0);
}

PRIVATE uint64_t tsc_read(timer_handlers_t *handlers) {
  handlers = NULL;

  uint64_t edx = 0, eax = 0;
  __asm__ volatile("rdtsc" : "=d"(edx), "=a"(eax));
  return (edx << 32) | (eax & 0xffffffff);
}

PRIVATE int tsc_init() {
  // Setup the tsc
  uint64_t cr4 = 0;
  __asm__ volatile("mov %%cr4, %0" : "=r"(cr4)::);
  cr4 |= (1 << 2);
  __asm__ volatile("mov %0, %%cr4" ::"r"(cr4));

  // Add the tsc as a counter
  {
    timer_handlers_t main_counter = {.name = "tsc"};
    timer_features_t main_features = timer_features_persistent |
                                     timer_features_counter |
                                     timer_features_read;

    // strncpy(main_counter.name, "tsc", 16);
    main_counter.rate = get_cpuid()->tsc_freq;
    main_counter.read = tsc_read;
    main_counter.write = NULL;
    main_counter.set_mode = NULL;
    main_counter.set_enable = NULL;
    main_counter.set_handler = NULL;

    timer_register(main_features, &main_counter);
  }

  // Initialize the apic timer
  return apic_timer_init(); // TODO: May want to use the TSC deadline mode
}

PRIVATE int tsc_mp_init() {
  // Setup the tsc
  uint64_t cr4 = 0;
  __asm__ volatile("mov %%cr4, %0" : "=r"(cr4)::);
  cr4 |= (1 << 2);
  __asm__ volatile("mov %0, %%cr4" ::"r"(cr4));

  // Initialize the apic timer
  return apic_timer_init(); // TODO: May want to use the TSC deadline mode
}