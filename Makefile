NASM ?= $(shell command -v nasm 2>/dev/null || echo tools/usr/bin/nasm)
CC ?= gcc
LD ?= ld
OBJCOPY ?= objcopy
QEMU ?= qemu-system-i386
MKISOFS ?= $(or $(shell command -v xorriso 2>/dev/null),$(shell command -v genisoimage 2>/dev/null),$(shell command -v mkisofs 2>/dev/null))
MKISOFS_FLAGS := $(if $(findstring xorriso,$(MKISOFS)),-as mkisofs,)
BUILD := build
QEMU_FLAGS ?=
SRC := kernel/kernel.c kernel/console.c kernel/serial.c kernel/memory.c kernel/interrupts.c kernel/scheduler.c kernel/string.c kernel/shell.c \
	kernel/gfx.c kernel/theme.c kernel/desktop.c \
	kernel/app_browser.c kernel/app_settings.c kernel/app_security.c kernel/app_terminal.c \
	kernel/font8x16.c \
	drivers/pit.c drivers/keyboard.c drivers/mouse.c
OBJ := $(patsubst %.c,$(BUILD)/%.o,$(SRC)) $(BUILD)/kernel/entry.o
CFLAGS_COMMON := -m32 -std=gnu11 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -finput-charset=UTF-8 -fexec-charset=ISO-8859-1 -Wall -Wextra -Werror -Ikernel
LDFLAGS := -m elf_i386 -T kernel/linker.ld --build-id=none
QEMU_BASE := -machine pc -m 32M -drive file=$(BUILD)/nexos.img,format=raw,if=floppy -boot a -serial stdio -monitor none -no-reboot -no-shutdown

.PHONY: all debug release run test test-boot test-shell iso clean help
all: release

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(CFLAGS_COMMON) -O0 -g3 -DDEBUG=1' $(BUILD)/nexos.img
release:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(CFLAGS_COMMON) -O2 -DNDEBUG' $(BUILD)/nexos.img

$(BUILD)/kernel/%.o: kernel/%.c kernel/kernel.h $(wildcard kernel/*.h)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/drivers/%.o: drivers/%.c $(wildcard drivers/*.h) kernel/kernel.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel/entry.o: kernel/entry.asm
	@mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/boot.bin: boot/boot.asm
	@mkdir -p $(@D)
	$(NASM) -f bin $< -o $@
	@test "$$(wc -c < $@)" -eq 512
$(BUILD)/loader.bin: boot/load.asm
	@mkdir -p $(@D)
	$(NASM) -f bin $< -o $@
$(BUILD)/kernel.elf: $(OBJ) kernel/linker.ld
	$(LD) $(LDFLAGS) -Map=$(BUILD)/kernel.map -o $@ $(OBJ)
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
$(BUILD)/nexos.img: $(BUILD)/boot.bin $(BUILD)/loader.bin $(BUILD)/kernel.bin
	@test "$$(wc -c < $(BUILD)/loader.bin)" -le 3072 || (echo 'Chargeur etage 2 > 6 secteurs' >&2; exit 1)
	@test "$$(wc -c < $(BUILD)/kernel.bin)" -le 153600 || (echo 'Noyau > 300 secteurs, limite BIOS depassee' >&2; exit 1)
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BUILD)/boot.bin of=$@ conv=notrunc status=none
	dd if=$(BUILD)/loader.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=7 conv=notrunc status=none

$(BUILD)/iso/nexos.img: $(BUILD)/nexos.img
	@mkdir -p $(BUILD)/iso
	cp $< $@

$(BUILD)/nexos.iso: $(BUILD)/iso/nexos.img
ifeq ($(MKISOFS),)
	@test -d .venv_pip/pycdlib || python3 -m pip install --target .venv_pip pycdlib
	PYTHONPATH=.venv_pip python3 tools/make_iso.py $(BUILD)/iso/nexos.img $@
else
	$(MKISOFS) $(MKISOFS_FLAGS) -o $@ -b nexos.img -c boot.cat -J -R -V NEXOS $(BUILD)/iso
endif

iso: release
	$(MAKE) $(BUILD)/nexos.iso

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
	@echo 'make release     : image optimale -O2 (reaction par defaut)'
	@echo 'make run         : demarrer NexOS dans QEMU (bureau VBE + souris)'
	@echo 'make test        : verifier ELF, boot QEMU et logs serie'
	@echo 'make test-shell  : injecter about au clavier QEMU'
	@echo 'make iso         : construire build/nexos.iso (El Torito, emulation disquette 1,44 Mo)'
	@echo 'make clean       : supprimer build/'
