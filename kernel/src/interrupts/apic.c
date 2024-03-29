/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "stdint.h"
#include "types.h"

#include "cpuid.h"
#include "interrupts.h"
#include "memory.h"

#define APIC_ID (0x20)
#define APIC_TPR (0x80)
#define APIC_PPR (0xA0)
#define APIC_EOI (0xB0)
#define APIC_LDR (0xD0)
#define APIC_DFR (0xE0)
#define APIC_SVR (0xF0)
#define APIC_ISR (0x100)
#define APIC_ICR_xAPIC_LO (0x300)
#define APIC_ICR_xAPIC_HI (0x310)
#define APIC_ICR_x2APIC (0x300)

#define APIC_TIMER (0x320)
#define APIC_ICoR (0x380)
#define APIC_CCoR (0x390)
#define APIC_DCR (0x3E0)

#define MSI_ADDR (0xFEEFF00C)
#define MSI_VEC                                                                \
  (lvl, active_low, vector)(((lvl & 1) << 15) | ((~active_low & 1) << 14) |    \
                            0x100 /*lowest priority*/ | (vector & 0xff))

typedef struct {
  uint32_t *base_addr;
  uint32_t id;
  bool x2apic_mode;
} tls_apic_t;

static TLS tls_apic_t *apic = NULL;

uint64_t apic_read(uint32_t off) {
  if (apic->x2apic_mode) {
    uint64_t val = rdmsr(0x800 + off / 16);
    return val;
  }
  return apic->base_addr[off / sizeof(uint32_t)];
}

void apic_write(uint32_t off, uint64_t val) {
  if (apic->x2apic_mode) {

    if (off == APIC_EOI) {
      wrmsr(0x800 + off / 16, val);
      return;
    }
    uint64_t tmp_Val = rdmsr(0x800 + off / 16);
    wrmsr(0x800 + off / 16, (tmp_Val & ~0xffffffff) | val);
  } else
    apic->base_addr[off / sizeof(uint32_t)] = (uint32_t)val;
}

void apic_init(void) {

  if (apic == NULL) {
    apic = (TLS tls_apic_t *)tls_alloc(sizeof(tls_apic_t));
  }

  uint64_t apic_base_reg = rdmsr(IA32_APIC_BASE);
  apic->base_addr = (uint32_t *)vmem_phystovirt(apic_base_reg & ~0xfff, KiB(8),
                                                vmem_flags_uncached);
  apic_base_reg |= (1 << 11); // Enable the apic

  bool x2apic_sup = get_cpuid()->x2apic;
  apic->x2apic_mode = x2apic_sup;

  if (x2apic_sup)
    apic_base_reg |= (1 << 10); // Enable x2apic mode

  if (!apic->x2apic_mode)
    apic_write(APIC_DFR, 0xf0000000); // Setup cluster destination mode
  wrmsr(IA32_APIC_BASE, apic_base_reg);

  // Read the id after enabling the apic so x2apic handling works correctly
  apic->id = apic_read(APIC_ID);
  if (!apic->x2apic_mode)
    apic->id = apic->id >> 24;

  apic_write(APIC_TPR, 0);
  apic_write(APIC_SVR, (1 << 8) | 0xFF);

  // TODO: Implement IPI function
}

int interrupt_get_cpuidx(void) { return apic->id; }

void interrupt_sendeoi(int irq) {
  int byte_off = (irq / 32) * 16;
  int bit_off = irq % 32;

  if (apic_read(APIC_ISR + byte_off) & (1 << bit_off)) {
    apic_write(APIC_EOI, 0);
  }
}

void interrupt_sendipi(int cpu, int vector, ipi_delivery_mode_t delivery_mode) {
  uint64_t ipi_msg = 0;
  ipi_msg |= (vector & 0xff);
  ipi_msg |= (delivery_mode & 0x7) << 8;
  ipi_msg |= (1 << 14);

  if (apic->x2apic_mode) {
    ipi_msg |= ((uint64_t)cpu << 32);

    apic_write(APIC_ICR_x2APIC, ipi_msg);
  } else {
    ipi_msg |= ((uint64_t)cpu << 56);

    apic_write(APIC_ICR_xAPIC_HI, (uint32_t)(ipi_msg >> 32));
    apic_write(APIC_ICR_xAPIC_LO, (uint32_t)ipi_msg);
  }
}

static int intrpt_num = 0;

// configure timer for tsc deadline
int local_apic_timer_init(bool tsc_mode, void (*handler)(int), bool ap) {

  if (!ap) {
    intrpt_num = 50; // Allocate a low priority interrupt
    interrupt_allocate(1, interrupt_flags_exclusive, &intrpt_num);
    interrupt_registerhandler(intrpt_num, handler);
  }

  uint64_t v = apic_read(APIC_TIMER);
  v = v & ~0x007100ff;
  v |= (intrpt_num & 0xff);

  if (tsc_mode) {
    v |= (2 << 17); // Set TSC-Deadline mode
  } else {
    v |= (1 << 17); // Set Periodic mode

    // tick every 0.05ms
    uint64_t apic_freq = get_cpuid()->apic_freq;
    apic_write(APIC_ICoR, apic_freq / 20000);
  }

  apic_write(APIC_TIMER, v);
  return intrpt_num;
}

// task switches don't require callibration
// sleep operations do require callibration - given frequency timers