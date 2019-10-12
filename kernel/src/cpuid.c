/**
 * Copyright (c) 2019 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
#include "cpuid.h"

#include "debug.h"
#include "string.h"

typedef enum {
  CPUID_FEAT_ECX_SSE3 = 1 << 0,
  CPUID_FEAT_ECX_PCLMUL = 1 << 1,
  CPUID_FEAT_ECX_DTES64 = 1 << 2,
  CPUID_FEAT_ECX_MONITOR = 1 << 3,
  CPUID_FEAT_ECX_DS_CPL = 1 << 4,
  CPUID_FEAT_ECX_VMX = 1 << 5,
  CPUID_FEAT_ECX_SMX = 1 << 6,
  CPUID_FEAT_ECX_EST = 1 << 7,
  CPUID_FEAT_ECX_TM2 = 1 << 8,
  CPUID_FEAT_ECX_SSSE3 = 1 << 9,
  CPUID_FEAT_ECX_CID = 1 << 10,
  CPUID_FEAT_ECX_FMA = 1 << 12,
  CPUID_FEAT_ECX_CX16 = 1 << 13,
  CPUID_FEAT_ECX_ETPRD = 1 << 14,
  CPUID_FEAT_ECX_PDCM = 1 << 15,
  CPUID_FEAT_ECX_DCA = 1 << 18,
  CPUID_FEAT_ECX_SSE4_1 = 1 << 19,
  CPUID_FEAT_ECX_SSE4_2 = 1 << 20,
  CPUID_FEAT_ECX_x2APIC = 1 << 21,
  CPUID_FEAT_ECX_MOVBE = 1 << 22,
  CPUID_FEAT_ECX_POPCNT = 1 << 23,
  CPUID_FEAT_ECX_AES = 1 << 25,
  CPUID_FEAT_ECX_XSAVE = 1 << 26,
  CPUID_FEAT_ECX_OSXSAVE = 1 << 27,
  CPUID_FEAT_ECX_AVX = 1 << 28,

  CPUID_FEAT_EDX_FPU = 1 << 0,
  CPUID_FEAT_EDX_VME = 1 << 1,
  CPUID_FEAT_EDX_DE = 1 << 2,
  CPUID_FEAT_EDX_PSE = 1 << 3,
  CPUID_FEAT_EDX_TSC = 1 << 4,
  CPUID_FEAT_EDX_MSR = 1 << 5,
  CPUID_FEAT_EDX_PAE = 1 << 6,
  CPUID_FEAT_EDX_MCE = 1 << 7,
  CPUID_FEAT_EDX_CX8 = 1 << 8,
  CPUID_FEAT_EDX_APIC = 1 << 9,
  CPUID_FEAT_EDX_SEP = 1 << 11,
  CPUID_FEAT_EDX_MTRR = 1 << 12,
  CPUID_FEAT_EDX_PGE = 1 << 13,
  CPUID_FEAT_EDX_MCA = 1 << 14,
  CPUID_FEAT_EDX_CMOV = 1 << 15,
  CPUID_FEAT_EDX_PAT = 1 << 16,
  CPUID_FEAT_EDX_PSE36 = 1 << 17,
  CPUID_FEAT_EDX_PSN = 1 << 18,
  CPUID_FEAT_EDX_CLF = 1 << 19,
  CPUID_FEAT_EDX_DTES = 1 << 21,
  CPUID_FEAT_EDX_ACPI = 1 << 22,
  CPUID_FEAT_EDX_MMX = 1 << 23,
  CPUID_FEAT_EDX_FXSR = 1 << 24,
  CPUID_FEAT_EDX_SSE = 1 << 25,
  CPUID_FEAT_EDX_SSE2 = 1 << 26,
  CPUID_FEAT_EDX_SS = 1 << 27,
  CPUID_FEAT_EDX_HTT = 1 << 28,
  CPUID_FEAT_EDX_TM1 = 1 << 29,
  CPUID_FEAT_EDX_IA64 = 1 << 30,
  CPUID_FEAT_EDX_PBE = 1 << 31
} CPUID_FEAT;

typedef enum { CPUID_EAX = 0, CPUID_EBX, CPUID_ECX, CPUID_EDX } CPUID_REG;

typedef enum { CPUID_ECX_IGNORE = 0, CPUID_EAX_FIRST_PAGE = 1 } CPUID_REQUESTS;

#define MANUFACT_AMD 0
#define MANUFACT_INTEL 1

static uint32_t cache_line_size = 0;
static cpuinfo_t cpuinfo;

static void CPUID_RequestInfo(uint32_t page, uint32_t idx, uint32_t *eax,
                              uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {

  uint32_t ax = page, bx = *ebx, cx = idx, dx = *edx;

  __asm__ volatile("cpuid\n\t"
                   : "=a"(ax), "=b"(bx), "=c"(cx), "=d"(dx)
                   : "a"(ax), "c"(cx));

  *eax = ax;
  *ebx = bx;
  *ecx = cx;
  *edx = dx;
}

void cpuid_init(void) {
  uint32_t eax, ebx, ecx, edx;
  uint8_t *eax_str = (uint8_t *)&eax;
  uint8_t *ebx_str = (uint8_t *)&ebx;
  uint8_t *edx_str = (uint8_t *)&edx;
  uint8_t *ecx_str = (uint8_t *)&ecx;

  int cpu_manufacturer = 0;
  {
    CPUID_RequestInfo(0, 0, &eax, &ebx, &ecx, &edx);

    cpuinfo.processor_name[0] = ebx_str[0];
    cpuinfo.processor_name[1] = ebx_str[1];
    cpuinfo.processor_name[2] = ebx_str[2];
    cpuinfo.processor_name[3] = ebx_str[3];
    cpuinfo.processor_name[4] = edx_str[0];
    cpuinfo.processor_name[5] = edx_str[1];
    cpuinfo.processor_name[6] = edx_str[2];
    cpuinfo.processor_name[7] = edx_str[3];
    cpuinfo.processor_name[8] = ecx_str[0];
    cpuinfo.processor_name[9] = ecx_str[1];
    cpuinfo.processor_name[10] = ecx_str[2];
    cpuinfo.processor_name[11] = ecx_str[3];

    if (strncmp(cpuinfo.processor_name, "GenuineIntel", 12) == 0) {
      cpu_manufacturer = MANUFACT_INTEL;
      print_str("GenuineIntel\r\n");
    } else if (strncmp(cpuinfo.processor_name, "AuthenticAMD", 12) == 0) {
      cpu_manufacturer = MANUFACT_AMD;
      print_str("AuthenticAMD\r\n");
    }
  }

  {
    CPUID_RequestInfo(7, 0, &eax, &ebx, &ecx, &edx);
    cpuinfo.smep = (ebx >> 7) & 1;
    cpuinfo.smap = (ebx >> 20) & 1;
  }

  {
    CPUID_RequestInfo(0x80000001, 0, &eax, &ebx, &ecx, &edx);
    cpuinfo.hugepage = (edx >> 26) & 1;
  }

  {
    CPUID_RequestInfo(0x1, 0, &eax, &ebx, &ecx, &edx);
    cpuinfo.x2apic = (ecx >> 21) & 1;
    cpuinfo.xsave = (ecx >> 26) & 1;
  }

  {
    CPUID_RequestInfo(0x0d, 0, &eax, &ebx, &ecx, &edx);
    cpuinfo.xsave_sz = ecx;
    cpuinfo.xsave_bits = (uint64_t)edx << 32 | eax;
  }

  {
    CPUID_RequestInfo(0x80000007, 0, &eax, &ebx, &ecx, &edx);
    cpuinfo.tsc_invar = (edx >> 8) & 1;
  }

  {
    CPUID_RequestInfo(1, 0, &eax, &ebx, &ecx, &edx);
    uint32_t stepping = eax & 0x0F;
    uint32_t model = (eax & 0xF0) >> 4;
    uint32_t family = (eax & 0xF00) >> 8;
    uint32_t processor_type = (eax & 0xF000) >> 12;

    if (family == 15) {
      family += (eax & 0xFF00000) >> 20;
      model = model | ((eax & 0xF0000) >> 12);
    }

    cpuinfo.tsc_valid = (edx >> 4) & 1;
    cpuinfo.tsc_deadline = (ecx >> 24) & 1;

    // APIC frequency is the Bus frequency by default
    CPUID_RequestInfo(0x16, 0, &eax, &ebx, &ecx, &edx);
    uint32_t apic_rate = ecx & 0xFFFF;

    CPUID_RequestInfo(0x15, 0, &eax, &ebx, &ecx, &edx);
    uint32_t ratio_denom = eax;
    uint32_t ratio_numer = ebx;
    uint32_t clock_freq = ecx;
    uint32_t tsc_rate = 0;
    if (ratio_denom != 0)
      tsc_rate = (clock_freq * ratio_numer) / ratio_denom;

    // Use the processor identification to configure special information, like
    // APIC clock rates
    switch (cpu_manufacturer) {
    case MANUFACT_AMD: {
      // This method works for Zen only
      if (tsc_rate == 0)
        tsc_rate = (rdmsr(0xc0010064) & 0xff) * 25 * (1000 * 1000);

      cpuinfo.tsc_freq = tsc_rate;

      // Default to 100MHz
      if (apic_rate == 0)
        apic_rate = 100;

      cpuinfo.apic_freq = apic_rate * 1000 * 1000;
    } break;
    case MANUFACT_INTEL: {
      cpuinfo.tsc_freq = tsc_rate;

      if (apic_rate == 0)
        switch (model) {
        // Nehalem
        case 0x1a:
        case 0x1e:
        case 0x1f:
        case 0x2e:

        // Westmere
        case 0x25:
        case 0x2c:
        case 0x2f: // CPUID holds, else must callibrate
          break;

        case 0x2a: // Sandy Bridge
        case 0x2d: // Sandy Bridge EP
        case 0x3a: // Ivy Bridge
        case 0x3e: // Ivy Bridge EP
        case 0x3c: // Haswell DT
        case 0x3f: // Haswell MB
        case 0x45: // Haswell ULT
        case 0x46: // Haswell ULX
        case 0x3d: // Broadwell
        case 0x47: // Broadwell H
        case 0x56: // Broadwell EP
        case 0x4f: // Broadwell EX
          // MSR_PLATFORM_INFO gives the apic rate
          apic_rate = ((rdmsr(0xce) >> 8) & 0xFF) * 100;
          break;

        case 0x4e: // Skylake Y/U
        case 0x5e: // Skylake H/S
        case 0x55: // Skylake E

        case 0x8e: // Kabylake Y/U
        case 0x9e: // Kabylake H/S
          apic_rate = 24;
          break;
        }
      cpuinfo.apic_freq = apic_rate * 1000 * 1000;
    } break;
    default:
      cpuinfo.tsc_freq = tsc_rate;
      cpuinfo.apic_freq = apic_rate;
      break;
    }
  }
}

cpuinfo_t *get_cpuid(void) { return &cpuinfo; }