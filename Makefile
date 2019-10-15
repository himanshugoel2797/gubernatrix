CC= clang
ASM= clang
LD= clang

BUILD_MODE=DEBUG
DEFINES= -DMULTIBOOT2 -D$(BUILD_MODE) -D_KERNEL_ -DCURRENT_YEAR="$(shell date +"%Y")" -DTYPES_H=common/inc/types.h

CFLAGS= -fPIC -target x86_64-none-elf -nostdinc -std=c11 -ffreestanding -Wall -Wextra -Wno-unused-variable -Wno-trigraphs -Werror -mno-red-zone -mcmodel=kernel -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3
ASMFLAGS= -fPIC
LDFLAGS= -fuse-ld=lld -ffreestanding -O2 -mno-red-zone -nostdlib -z max-page-size=0x1000 -mcmodel=kernel
KERN_LDFLAGS= -Wl,--whole-archive -Wl,--script=linker.ld -Wl,--omagic $(LDFLAGS)

COMMON_C_SRCS= $(wildcard common/src/*.c) $(wildcard common/src/edid/*.c)

KERN_C_SRCS= $(wildcard kernel/src/*.c) $(wildcard kernel/src/memory/*.c) $(wildcard kernel/src/interrupts/*.c) $(wildcard kernel/src/timers/*.c)
KERN_ASM_SRCS= $(wildcard kernel/src/*.S)

KERN_INCS= -I "kernel/inc/" -I "common/inc/"

KERN_OBJS= $(KERN_ASM_SRCS:.S=.o) $(COMMON_C_SRCS:.c=.o) $(KERN_C_SRCS:.c=.o)
KERN_DRIVERS= $(wildcard devices/*.a)

all: build

kernel/src/%.o: kernel/src/%.S
	$(ASM) $(ASMFLAGS) $(DEFINES) -c $< -o $@

kernel/src/%.o: kernel/src/%.c
	$(CC) $(CFLAGS) $(KERN_INCS) $(DEFINES) -S $< -o $(<:.c=.S)
	$(ASM) $(ASMFLAGS) $(DEFINES) -c $(<:.c=.S) -o $@
	rm $(<:.c=.S)
	
common/src/%.o: common/src/%.c
	$(CC) $(CFLAGS) $(KERN_INCS) $(DEFINES) -S $< -o $(<:.c=.S)
	$(ASM) $(ASMFLAGS) $(DEFINES) -c $(<:.c=.S) -o $@
	rm $(<:.c=.S)

drivers:
	$(MAKE) -C devices

kernel.bin: drivers $(KERN_OBJS)
	$(LD) -o $@ $(KERN_LDFLAGS) $(KERN_DRIVERS) $(KERN_OBJS) 

image: kernel.bin
	grub-mkstandalone -O x86_64-efi -o "BOOTX64.EFI" "boot/grub/grub.cfg=grub.cfg" "boot/kernel.bin=kernel.bin"

build: image

install_win: build
	powershell.exe -File "I:\Code\gubernatrix\install.ps1"

clean:
	@- rm $(KERN_OBJS)
	@- rm kernel.bin