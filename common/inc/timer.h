// Copyright (c) 2019 Himanshu Goel
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef GUBERNATRIX_TIMERS_H
#define GUBERNATRIX_TIMERS_H

#include "stdint.h"

typedef enum {
  timer_features_none = 0,
  timer_features_oneshot = (1 << 0),
  timer_features_periodic = (1 << 1),
  timer_features_read = (1 << 2),
  timer_features_persistent = (1 << 3),
  timer_features_absolute = (1 << 4),
  timer_features_64bit = (1 << 5),
  timer_features_write = (1 << 6),
  timer_features_local = (1 << 7),
  timer_features_pcie_msg_intr = (1 << 8),
  timer_features_fixed_intr = (1 << 9),
  timer_features_counter = (1 << 10),
} timer_features_t;

typedef struct timer_handlers timer_handlers_t;
struct timer_handlers {
  char name[16];
  uint64_t (*read)(timer_handlers_t *);
  void (*write)(timer_handlers_t *, uint64_t);
  uint64_t (*set_mode)(timer_handlers_t *, timer_features_t);
  void (*set_enable)(timer_handlers_t *, bool);
  void (*set_handler)(timer_handlers_t *, void (*)(int));
  uint64_t rate;
  uint64_t state;
};

void timer_wait(uint64_t ns);

int timer_request(timer_features_t features, uint64_t ns, void (*handler)(int));
int timer_register(timer_features_t features, timer_handlers_t *handlers);

int timer_platform_gettimercount();
int timer_platform_init();

void timer_init(void);
void timer_mp_init(void);

#endif