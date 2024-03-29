/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "interrupts.h"
#include "local_spinlock.h"
#include "memory.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"
#include "debug.h"

#define IDT_ENTRY_COUNT (256)
#define IDT_HANDLER_CNT (16)
#define IDT_ENTRY_HANDLER_SIZE (64)
#define IDT_TYPE_INTR (0xE)

typedef struct {
  uint32_t offset0 : 16;
  uint32_t seg_select : 16;
  uint32_t ist : 3;
  uint32_t zr0 : 5;
  uint32_t type : 4;
  uint32_t zr1 : 1;
  uint32_t dpl : 2;
  uint32_t p : 1;
  uint32_t offset1 : 16;
  uint32_t offset2 : 32;
  uint32_t zr2 : 32;
} idt_t;

typedef struct PACKED {
  uint16_t limit;
  idt_t *base;
} idtr_t;

typedef struct {
  uint64_t rsp;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rbp;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;
  uint64_t int_no;
  uint64_t err_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t useresp;
  uint64_t ss;
} regs_t;

typedef struct {
  idt_t *idt;
  regs_t *reg_state;
  regs_t *reg_ref;
  int proc_idx;
} tls_idt_t;

static TLS tls_idt_t *idt = NULL;
static char idt_handlers[IDT_ENTRY_COUNT][IDT_ENTRY_HANDLER_SIZE];
static InterruptHandler interrupt_funcs[IDT_ENTRY_COUNT][IDT_HANDLER_CNT];
static bool interrupt_blocked[IDT_ENTRY_COUNT];
static int interrupt_alloc_lock = 0;
static _Atomic int proc_idx_cntr = 0;
static bool int_arr_inited = false;

void interrupt_registerhandler(int irq, InterruptHandler handler) {
  int state = cli();
  local_spinlock_lock(&interrupt_alloc_lock);
  for (int i = 0; i < IDT_HANDLER_CNT; i++)
    if (interrupt_funcs[irq][i] == NULL) {
      interrupt_funcs[irq][i] = handler;
      local_spinlock_unlock(&interrupt_alloc_lock);
      sti(state);
      return;
    }
  local_spinlock_unlock(&interrupt_alloc_lock);
  sti(state);

  PANIC("Interrupt oversubscribed!");
}

void interrupt_unregisterhandler(int irq, InterruptHandler handler) {
  int state = cli();
  local_spinlock_lock(&interrupt_alloc_lock);
  for (int i = 0; i < IDT_HANDLER_CNT; i++)
    if (interrupt_funcs[irq][i] == handler) {
      interrupt_funcs[irq][i] = NULL;
    }

  local_spinlock_unlock(&interrupt_alloc_lock);
  sti(state);
}

int interrupt_allocate(int cnt, interrupt_flags_t flags, int *base) {
  if (flags & interrupt_flags_fixed) {
    int state = cli();
    local_spinlock_lock(&interrupt_alloc_lock);

    for (int c = 0; c < cnt; c++) {
      if (interrupt_blocked[*base + c]) {
        local_spinlock_unlock(&interrupt_alloc_lock);
        sti(state);
        return -1;
      }
    }

    if (flags & interrupt_flags_exclusive)
      for (int c = 0; c < cnt; c++)
        interrupt_blocked[*base + c] = true;

    local_spinlock_unlock(&interrupt_alloc_lock);
    sti(state);
    return 0;
  } else {
    // if fixed allocation works, use it
    if (*base != 0) {
      int err = interrupt_allocate(cnt, flags | interrupt_flags_fixed, base);
      if (err == 0)
        return 0;
    }

    // find a block that does work
    int state = cli();
    local_spinlock_lock(&interrupt_alloc_lock);

    int run_off = 32; // IRQs start at 32 for x86
    int run_len = 0;
    for (int i = 32; i < IDT_ENTRY_COUNT; i++) {
      if (run_len >= cnt) {
        if (flags & interrupt_flags_exclusive)
          for (int c = 0; c < cnt; c++)
            interrupt_blocked[run_off + c] = true;

        *base = run_off;
        local_spinlock_unlock(&interrupt_alloc_lock);
        sti(state);
        return 0;
      }

      if (interrupt_blocked[i]) {
        run_len = 0;
        run_off = i + 1;
      } else
        run_len++;
    }

    local_spinlock_unlock(&interrupt_alloc_lock);
    sti(state);
    return -1;
  }
}

void idt_mainhandler(regs_t *regs) {
  // Store the registers in the processor interrupt state
  regs->int_no = (uint8_t)regs->int_no;

  memcpy(idt->reg_state, regs, sizeof(regs_t));
  idt->reg_ref = regs;

  bool handled = false;

  int state = cli();

  local_spinlock_lock(&interrupt_alloc_lock);
  for (int i = 0; i < IDT_HANDLER_CNT; i++) {
    if (interrupt_funcs[regs->int_no][i] != NULL) {
      interrupt_funcs[regs->int_no][i](regs->int_no);
      handled = true;
    }
  }
  local_spinlock_unlock(&interrupt_alloc_lock);
  sti(state);

  if (!handled) {
    print_str("[Core: ");
    print_int32(idt->proc_idx, BASE_HEX);
    print_str("] Unhandled Interrupt: ");
    print_uint8((uint8_t)regs->int_no, BASE_HEX);
    print_str("\r\n");
    PANIC("Failure!");
  }

  idt->reg_ref = NULL;

  if (regs->int_no >= 32)
    interrupt_sendeoi(regs->int_no);
}

void interrupt_setregisterstate(interrupt_register_state_t *state) {
  if (state != NULL) {

    idt->reg_ref->r15 = state->r15;
    idt->reg_ref->r14 = state->r14;
    idt->reg_ref->r13 = state->r13;
    idt->reg_ref->r12 = state->r12;
    idt->reg_ref->r11 = state->r11;
    idt->reg_ref->r10 = state->r10;
    idt->reg_ref->r9 = state->r9;
    idt->reg_ref->r8 = state->r8;
    idt->reg_ref->rdi = state->rdi;
    idt->reg_ref->rsi = state->rsi;
    idt->reg_ref->rdx = state->rdx;
    idt->reg_ref->rcx = state->rcx;
    idt->reg_ref->rbx = state->rbx;
    idt->reg_ref->rax = state->rax;

    idt->reg_ref->rflags = state->rflags;
    idt->reg_ref->rip = state->rip;

    idt->reg_ref->cs = state->cs;
    idt->reg_ref->ss = state->ss;

    idt->reg_ref->rbp = state->rbp;
    idt->reg_ref->useresp = state->rsp;
  }
}

void interrupt_getregisterstate(interrupt_register_state_t *state) {
  if (state != NULL) {

    state->r15 = idt->reg_ref->r15;
    state->r14 = idt->reg_ref->r14;
    state->r13 = idt->reg_ref->r13;
    state->r12 = idt->reg_ref->r12;
    state->r11 = idt->reg_ref->r11;
    state->r10 = idt->reg_ref->r10;
    state->r9 = idt->reg_ref->r9;
    state->r8 = idt->reg_ref->r8;
    state->rdi = idt->reg_ref->rdi;
    state->rsi = idt->reg_ref->rsi;
    state->rdx = idt->reg_ref->rdx;
    state->rcx = idt->reg_ref->rcx;
    state->rbx = idt->reg_ref->rbx;
    state->rax = idt->reg_ref->rax;

    state->rflags = idt->reg_ref->rflags;
    state->rip = idt->reg_ref->rip;

    state->cs = idt->reg_ref->cs;
    state->ss = idt->reg_ref->ss;

    state->rbp = idt->reg_ref->rbp;
    state->rsp = idt->reg_ref->useresp;
  }
}

NAKED NORETURN static void idt_defaulthandler() {
  __asm__ volatile("pushq %rbx\n\t"
                   "pushq %rcx\n\t"
                   "pushq %rdx\n\t"
                   "pushq %rbp\n\t"
                   "pushq %rsi\n\t"
                   "pushq %rdi\n\t"
                   "pushq %r8\n\t"
                   "pushq %r9\n\t"
                   "pushq %r10\n\t"
                   "pushq %r11\n\t"
                   "pushq %r12\n\t"
                   "pushq %r13\n\t"
                   "pushq %r14\n\t"
                   "pushq %r15\n\t"
                   "movq %rsp, %rdi\n\t"
                   "pushq %rdi\n\t"
                   "movq %rsp, %rdi\n\t"
                   "callq idt_mainhandler\n\t"
                   "popq %rdi\n\t"
                   "popq %r15\n\t"
                   "popq %r14\n\t"
                   "popq %r13\n\t"
                   "popq %r12\n\t"
                   "popq %r11\n\t"
                   "popq %r10\n\t"
                   "popq %r9\n\t"
                   "popq %r8\n\t"
                   "popq %rdi\n\t"
                   "popq %rsi\n\t"
                   "popq %rbp\n\t"
                   "popq %rdx\n\t"
                   "popq %rcx\n\t"
                   "popq %rbx\n\t"
                   "popq %rax\n\t"
                   "add $16, %rsp\n\t"
                   "iretq\n\t");
}

static void idt_fillswinterrupthandler(char *idt_handler, uint8_t intNum,
                                       uint8_t pushToStack) {
  int index = 0;

  // Push dummy error code if the interrupt doesn't do so
  if (pushToStack) {
    idt_handler[index++] = 0x6a; // Push
    idt_handler[index++] = pushToStack;
  }

  idt_handler[index++] = 0x6a;   // Push
  idt_handler[index++] = intNum; // Push the interrupt number to stack

  // push jump address and ret
  idt_handler[index++] = 0x50; // push %%rax
  idt_handler[index++] = 0x48; // movq idt_defaulthandler, %%rax
  idt_handler[index++] = 0xb8;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (8)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (16)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (24)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> 32) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (32 + 8)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (32 + 16)) & 0xff;
  idt_handler[index++] = ((uint64_t)(idt_defaulthandler) >> (32 + 24)) & 0xff;
  //*(uint32_t*)&idt_handler[index] = 0x11223344;
  // index += 3;
  idt_handler[index++] = 0x50; // push %%rax
  idt_handler[index++] = 0xC3; // retq
}

void idt_init(void) {
  if (idt == NULL) {
    idt = (TLS tls_idt_t *)tls_alloc(sizeof(tls_idt_t));
  }
  idt->proc_idx = proc_idx_cntr++;
  idt->idt = malloc(IDT_ENTRY_COUNT * sizeof(idt_t));
  idt->reg_state = malloc(sizeof(regs_t));

  // Fill the IDT
  idt_t *idt_lcl = idt->idt;

  int pushesToStack = 1;
  if (!int_arr_inited) {

    for (int i = 0; i < IDT_ENTRY_COUNT; i++) {
      if (i == 8 || (i >= 10 && i <= 14))
        pushesToStack = 0;
      idt_fillswinterrupthandler(
          idt_handlers[i], i,
          pushesToStack); // If pushesToStack is non-zero, the value will be
                          // pushed to stack

      interrupt_blocked[i] = false;
      for (int j = 0; j < IDT_HANDLER_CNT; j++)
        interrupt_funcs[i][j] = NULL;

      pushesToStack = 1;
    }

    int_arr_inited = true;
  }

  for (int i = 0; i < IDT_ENTRY_COUNT; i++) {
    // Setup interrupts
    idt_lcl[i].offset0 = (uint64_t)idt_handlers[i] & 0xFFFF;
    idt_lcl[i].offset1 = ((uint64_t)idt_handlers[i] >> 16) & 0xFFFF;
    idt_lcl[i].offset2 = ((uint64_t)idt_handlers[i] >> 32) & 0xFFFFFFFF;
    idt_lcl[i].seg_select = 0x08;
    idt_lcl[i].type = IDT_TYPE_INTR;
    idt_lcl[i].p = 1;
    idt_lcl[i].dpl = 0;
    idt_lcl[i].ist = 0;
    idt_lcl[i].zr0 = 0;
    idt_lcl[i].zr1 = 0;
    idt_lcl[i].zr2 = 0;
  }

  idtr_t idtr;
  idtr.limit = sizeof(idt_t) * IDT_ENTRY_COUNT - 1;
  idtr.base = idt->idt;

  __asm__ volatile("lidt (%0)" ::"r"(&idtr));
}