
CC   = cross-compiler/bin/x86_64-elf-gcc
LD   = cross-compiler/bin/x86_64-elf-ld
NASM = nasm

CFLAGS  = -Wall -Wextra -std=c23 -nostdlib -ffreestanding \
          -fno-stack-protector -fno-stack-check -fno-PIC \
          -ffunction-sections -fdata-sections \
          -mcmodel=kernel -mno-red-zone \
          -mno-mmx -mno-sse -mno-sse2 \
          -Ilib -Isrc

NASMFLAGS = -f elf64

BUILD = build

C_SRCS   := $(shell find src -name '*.c')
ASM_SRCS := $(shell find src -name '*.asm')

C_OBJS   := $(patsubst src/%.c,   $(BUILD)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst src/%.asm, $(BUILD)/%.asm.o, $(ASM_SRCS))

OBJS := $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean run build-iso

all: build-iso

$(BUILD)/%.o: src/%.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.asm.o: src/%.asm Makefile
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

kernel: $(OBJS)
	$(LD) -o $@ -T linker.ld $(OBJS)

build-iso: kernel
	rm -rf iso_root
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	cp -v kernel iso_root/boot/
	cp -v limine.conf iso_root/boot/limine/
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
	    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
	    -efi-boot-part --efi-boot-image --protective-msdos-label \
	    iso_root -o kernel.iso

run:
	qemu-system-x86_64 --cdrom kernel.iso

clean:
	rm -rf $(BUILD) kernel iso_root kernel.iso
