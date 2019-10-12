/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "types.h"

#define ICW4_8086 0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW1_ICW4 0x01 /* ICW4 (not) needed */
#define ICW1_INIT 0x10 /* Initialization - required! */

void pic_fini(void) {
  // Disable the PIC
  outb(0x20,
       ICW1_INIT +
           ICW1_ICW4); // starts the initialization sequence (in cascade mode)
  outb(0xA0, ICW1_INIT + ICW1_ICW4);
  outb(0x21, 32); // ICW2: Master PIC vector offset
  outb(0xA1, 40); // ICW2: Slave PIC vector offset
  outb(
      0x21,
      4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  outb(0xA1, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)

  outb(0x21, ICW4_8086);
  outb(0xA1, ICW4_8086);

  outb(0x21, 0xFF);
  outb(0xA1, 0xFF); // disable all interrupts from the PIC
}