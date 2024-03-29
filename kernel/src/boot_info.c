/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "boot_info.h"
#include "bootstrap_malloc.h"
#include "multiboot2.h"

#include "stdint.h"
#include "string.h"
#include "types.h"

#include "debug.h"

BootInfo bootInfo;

typedef struct multiboot_tag_framebuffer multiboot_tag_framebuffer;
typedef struct multiboot_tag_mmap multiboot_tag_mmap;
typedef struct multiboot_tag_new_acpi multiboot_tag_new_acpi;
typedef struct multiboot_tag_elf_sections multiboot_tag_elf_sections;
typedef struct multiboot_tag_module multiboot_tag_module;

extern uint64_t _region_kernel_start_, _region_kernel_end_,
    _bootstrap_region_start, _bootstrap_region_end, _trampoline_region_start,
    _trampoline_region_end;
extern uint64_t KERNEL_VADDR;

static uint64_t kernel_start_phys, kernel_end_phys, kernel_virt_addr;

void set_bootinfo(void *boot_info, uint32_t magic) {

  if (magic != MULTIBOOT_MAGIC)
    PANIC("Multiboot2 magic number check failed.");

  kernel_start_phys = (uint64_t)(&_region_kernel_start_);
  kernel_end_phys = (uint64_t)(&_region_kernel_end_);
  kernel_virt_addr = (uint64_t)(&KERNEL_VADDR);
  bootstrap_malloc_init(kernel_end_phys + kernel_virt_addr);

  uint8_t *hdr_8 = (uint8_t *)boot_info;
  uint32_t total_size = *(uint32_t *)boot_info;

  print_str("Multiboot2 info location: ");
  print_uint64((uint64_t)hdr_8, BASE_HEX);
  print_str(" Size: ");
  print_uint32(total_size, BASE_HEX);
  print_str("\r\n");

  memset(&bootInfo, 0, sizeof(BootInfo));
  bootInfo.kernel_end_phys = kernel_end_phys;
  bootInfo.kernel_virt_offset = kernel_virt_addr;

  uint32_t s = 0;

  for (uint32_t i = 8; i < total_size;) {
    uint32_t val = *(uint32_t *)&hdr_8[i];
    switch (val) {
    case MULTIBOOT_TAG_TYPE_MMAP: {
      print_str("Memory Map Found\r\n");
      multiboot_tag_mmap *mmap = (multiboot_tag_mmap *)&hdr_8[i];
      int entryCount = (mmap->size - 16) / mmap->entry_size;
      MemMap *map = bootstrap_malloc(sizeof(MemMap) * (entryCount));

      print_str("Entries: ");
      print_int32(entryCount, BASE_HEX);
      print_str("\r\n");

      uint32_t mmap_entry = 0;
      for (uint32_t j = 0; j < (mmap->size - 16); j += mmap->entry_size) {
        multiboot_memory_map_t *mmap_e =
            (multiboot_memory_map_t *)((uint8_t *)mmap->entries + j);

        if (mmap_e->type == MULTIBOOT_MEMORY_AVAILABLE)
          bootInfo.MemorySize += mmap_e->len;

        // Check the address range and insert a split if necessary
        // Reserve everything below 2M
        if (mmap_e->addr < MiB(2)) {
          uint64_t diff = MiB(2) - mmap_e->addr;
          if (diff >= mmap_e->len) {
            // skip this entry
            continue;
          }

          mmap_e->addr += diff;
          mmap_e->len -= diff;
        }

        // Reserve kernel memory
        if (mmap_e->addr >= kernel_start_phys) {
          uint64_t diff = kernel_end_phys - mmap_e->addr;
          if (mmap_e->addr + mmap_e->len < kernel_end_phys)
            continue; // Skip this entry

          if (mmap_e->addr < kernel_end_phys) {
            mmap_e->addr += diff;
            mmap_e->len -= diff;
          }
        }

        map[mmap_entry].addr = mmap_e->addr;
        map[mmap_entry].len = mmap_e->len;
        map[mmap_entry].type = (MemoryRegionType)mmap_e->type;
        mmap_entry++;
      }

      bootInfo.MemoryMap = map;
      bootInfo.MemoryMapCount = entryCount;
    } break;
    case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
      multiboot_tag_framebuffer *framebuffer =
          (multiboot_tag_framebuffer *)&hdr_8[i];
      bootInfo.FramebufferAddress = framebuffer->common.FramebufferAddress;
      bootInfo.FramebufferPitch = framebuffer->common.FramebufferPitch;
      bootInfo.FramebufferWidth = framebuffer->common.FramebufferWidth;
      bootInfo.FramebufferHeight = framebuffer->common.FramebufferHeight;
      bootInfo.FramebufferBPP = framebuffer->common.FramebufferBPP;

      bootInfo.FramebufferRedFieldPosition =
          framebuffer->FramebufferRedFieldPosition;
      bootInfo.FramebufferRedMaskSize = framebuffer->FramebufferRedMaskSize;
      bootInfo.FramebufferGreenFieldPosition =
          framebuffer->FramebufferGreenFieldPosition;
      bootInfo.FramebufferGreenMaskSize = framebuffer->FramebufferGreenMaskSize;
      bootInfo.FramebufferBlueFieldPosition =
          framebuffer->FramebufferBlueFieldPosition;
      bootInfo.FramebufferBlueMaskSize = framebuffer->FramebufferBlueMaskSize;
    } break;
    case MULTIBOOT_TAG_TYPE_ELF_SECTIONS: {
      multiboot_tag_elf_sections *elf = (multiboot_tag_elf_sections *)&hdr_8[i];
      bootInfo.elf_shdr_type = elf->type;
      bootInfo.elf_shdr_size = elf->size;
      bootInfo.elf_shdr_num = elf->num;
      bootInfo.elf_shdr_entsize = elf->entsize;
      bootInfo.elf_shdr_shndx = elf->shndx;
      bootInfo.elf_shdr_addr = (uint64_t)elf->sections;

    } break;
    case MULTIBOOT_TAG_TYPE_ACPI_OLD:
    case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
      multiboot_tag_new_acpi *acpi = (multiboot_tag_new_acpi *)&hdr_8[i];
      bootInfo.RSDPAddress = (uint64_t)acpi->rsdp;
    } break;
    case MULTIBOOT_TAG_TYPE_MODULE: {
      multiboot_tag_module *module = (multiboot_tag_module *)&hdr_8[i];
      bootInfo.InitrdStartAddress = (uint64_t)module->mod_start;
      bootInfo.InitrdPhysStartAddress = (uint64_t)module->mod_start;
      bootInfo.InitrdLength = (uint64_t)(module->mod_end - module->mod_start);
    } break;
    case MULTIBOOT_TAG_TYPE_END:
      // i += 8;   //We're done, exit the loop
      break;
    }
    s = *(uint32_t *)(&hdr_8[i + 4]);
    if (s % 8)
      s += 8 - (s % 8);
    i += s;
  }
}

BootInfo *get_bootinfo(void) { return &bootInfo; }