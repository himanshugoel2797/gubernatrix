# Copyright (c) 2019 Himanshu Goel
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT


CC:= clang
ASM:= clang
AR = llvm-ar

BUILD_MODE:=DEBUG
DEFINES:= -DMULTIBOOT2 -D$(BUILD_MODE) -D_KERNEL_ -DCURRENT_YEAR="$(shell date +"%Y")" -DTYPES_H=common/inc/types.h

CFLAGS:= -fPIC -target x86_64-none-elf -nostdinc -std=c11 -ffreestanding -Wall -Wextra -Wno-unused-variable -Wno-trigraphs -Werror -mno-red-zone -mcmodel=kernel -mno-aes -mno-mmx -mno-pclmul -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4a -mno-fma4 -mno-ssse3
ASMFLAGS:= -fPIC

SRCS:= $(wildcard src/*.c)
INCS:= -I "inc/" -I "../../common/inc/"

OBJS:= $(SRCS:.c=.o)

all: build

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCS) $(DEFINES) -S $< -o $(<:.c=.S)
	$(ASM) $(ASMFLAGS) $(DEFINES) -c $(<:.c=.S) -o $@
	rm $(<:.c=.S)
	
igfx.a: $(OBJS)
	$(AR) rcs igfx.a $(OBJS)

build: igfx.a
	cp igfx.a ../igfx.a
	rm igfx.a

clean:
	@- rm $(OBJS)
	@- rm igfx.a