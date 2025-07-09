SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ASFLAGS = -l $(basename $@).lst
CFLAGS = -std=c89 -m32 -march=pentium \
         -Wall \
         -I./include \
         -nostdlib -fno-builtin \
         -Xassembler -al=$(basename $@).lst
LDFLAGS = -melf_i386 -Ttext=0x8000 -defsym _start=kmain --oformat=binary

CC = /usr/local/bin/x86_64-elf-gcc
LD = /usr/local/bin/x86_64-elf-ld
AS = nasm

all:	boot.flp

boot.flp:	boot kernel.bin
	dd if=/dev/zero of=boot.flp bs=512 count=2880
	dd if=boot of=boot.flp bs=512 conv=notrunc
	dd if=kernel.bin of=boot.flp bs=512 conv=notrunc seek=1

kernel.bin:	$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

boot:	boot.asm
	$(AS) -o $@ $<

clean:
	rm -f kernel.bin
	rm -f $(OBJS)
	rm -f boot boot.flp
	rm -f *.lst