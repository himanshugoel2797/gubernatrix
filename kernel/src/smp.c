/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "smp.h"
#include "apic_data.h"
#include "cpuid.h"
#include "interrupts.h"
#include "types.h"
#include "timer.h"
#include "stdlib.h"
#include "string.h"
#include "local_spinlock.h"

#define TLS_SIZE ((int)KiB(16))
#define GS_BASE_MSR (0xC0000101)
#define KERNEL_GS_BASE_MSR (0xC0000102)

static int smp_loc = 0;
static _Atomic volatile int pos = 0;
static TLS uint64_t* g_tls;
static _Atomic int coreCount = 1;
static _Atomic volatile int core_ready = 0;

void alloc_ap_stack(void);

void smp_init(void) {
    //Start initializing the APs
    uint64_t ap_cnt = get_lapic_count();

    for(uint32_t i = 0; i < ap_cnt; i++) {
        uint64_t apic_id = get_lapic_info(i)->apic_id;

        if((int)apic_id != interrupt_get_cpuidx()) {
            core_ready = 0;

            alloc_ap_stack();
            interrupt_sendipi(apic_id, 0x0, ipi_delivery_mode_init);

            //Use the timer api to wait for 10ms
            timer_wait(10 * 1000 * 1000);
            interrupt_sendipi(apic_id, 0x0f, ipi_delivery_mode_startup);
            while(!core_ready);
        }
    }
}

int smp_corecount(void) {
    return coreCount;
}

void smp_signalready(void) {

    coreCount++;
    print_str("Core Registered.\r\n");
    core_ready = 1;
    while(true)
        ;
}

int smp_platform_getstatesize(void) {
    return sizeof(interrupt_register_state_t);
}

void smp_platform_getstate(void* buf) {
    if(buf == NULL)
        PANIC("Parameter is null.");

    interrupt_getregisterstate( (interrupt_register_state_t*) buf);
}

void smp_platform_setstate(void* buf) {
    if(buf == NULL)
        PANIC("Parameter is null.");

    interrupt_setregisterstate( (interrupt_register_state_t*) buf);
}

void smp_platform_getdefaultstate(void *buf, void *stackpointer, void *instr_ptr, void *args) {
    memset(buf, 0, smp_platform_getstatesize());
    interrupt_register_state_t* regs = (interrupt_register_state_t*)buf;

    regs->cs = 0x8;
    regs->ss = 0x10;

    regs->rsp = (uint64_t)stackpointer;
    regs->rbp = (uint64_t)stackpointer;

    regs->rflags = 0x0200;

    regs->rip = (uint64_t)instr_ptr;

    regs->rdi = (uint64_t)args;
}