NASM ?= nasm
CC ?= gcc
LD ?= ld
OBJCOPY ?= objcopy
QEMU ?= qemu-system-i386
BUILD := build
QEMU_FLAGS ?=
SRC := kernel/kernel.c kernel/console.c kernel/serial.c kernel/memory.c kernel/interrupts.c kernel/scheduler.c kernel/string.c kernel/shell.c drivers/pit.c drivers/keyboard.c
OBJ := $(patsubst %.c,$(BUILD)/%.o,$(SRC)) $(BUILD)/kernel/entry.o
CFLAGS_COMMON := -m32 -std=gnu11 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -Wall -Wextra -Werror -Ikernel
LDFLAGS := -m elf_i386 -T kernel/linker.ld --build-id=none
QEMU_BASE := -machine pc -m 32M -drive file=$(BUILD)/nexos.img,format=raw,if=floppy -boot a -serial stdio -monitor none -no-reboot -no-shutdown

.PHONY: all debug release run test test-boot test-shell clean help
all: release

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(CFLAGS_COMMON) -O0 -g3 -DDEBUG=1' $(BUILD)/nexos.img
release:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(CFLAGS_COMMON) -O2 -DNDEBUG' $(BUILD)/nexos.img

$(BUILD)/kernel/%.o: kernel/%.c kernel/kernel.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/drivers/%.o: drivers/%.c kernel/kernel.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel/entry.o: kernel/entry.asm
	@mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/boot.bin: boot/boot.asm
	@mkdir -p $(@D)
	$(NASM) -f bin $< -o $@
	@test "$$(wc -c < $@)" -eq 512
$(BUILD)/kernel.elf: $(OBJ) kernel/linker.ld
	$(LD) $(LDFLAGS) -Map=$(BUILD)/kernel.map -o $@ $(OBJ)
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
$(BUILD)/nexos.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	@test "$$(wc -c < $(BUILD)/kernel.bin)" -le 51200 || (echo 'Noyau > 100 secteurs, limite BIOS dépassée' >&2; exit 1)
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BUILD)/boot.bin of=$@ conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=1 conv=notrunc status=none

run: release
	$(QEMU) $(QEMU_FLAGS) $(QEMU_BASE)
test: release
	python3 tests/test_image.py $(BUILD)/nexos.img $(BUILD)/kernel.elf
	python3 tests/test_boot.py $(BUILD)/nexos.img $(QEMU) $(QEMU_FLAGS)
test-boot: test
test-shell: release
	python3 tests/test_shell.py $(BUILD)/nexos.img $(QEMU) $(QEMU_FLAGS)
clean:
	rm -rf $(BUILD)
help:
	@echo 'make debug       : construire image avec symboles -O0'
	@echo 'make release     : construire image optimisée -O2'
	@echo 'make run         : démarrer NexOS dans QEMU'
	@echo 'make test        : vérifier ELF, boot et logs série QEMU'
	@echo 'make clean       : supprimer build/'
