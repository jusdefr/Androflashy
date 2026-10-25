#!/usr/bin/env python3
"""Tests statiques de l'image disquette NexOS et de son ELF freestanding."""
import struct
import sys
from pathlib import Path


def check(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"ÉCHEC: {message}")
    print(f"OK: {message}")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("Usage: test_image.py build/nexos.img build/kernel.elf")
    image = Path(sys.argv[1]).read_bytes()
    elf = Path(sys.argv[2]).read_bytes()
    check(len(image) == 1474560, "image disquette 1,44 Mo")
    check(image[510:512] == b"\x55\xaa", "signature BIOS 0xAA55")
    check(image[:2] == b"\xfa\x31" or image[:1] == b"\xe9" or image[:1] == b"\xeb", "code de demarrage BIOS x86")
    check(b"NexOS BIOS boot" in image[:510], "message boot présent")
    check(len(elf) >= 52 and elf[:4] == b"\x7fELF", "format ELF")
    check(elf[4] == 1 and elf[5] == 1, "ELF 32 bits little endian")
    machine = struct.unpack_from("<H", elf, 18)[0]
    check(machine == 3, "ELF cible Intel 80386")
    entry = struct.unpack_from("<I", elf, 24)[0]
    check(entry == 0x10000, f"adresse d'entrée 0x{entry:08x}")
    phoff = struct.unpack_from("<I", elf, 28)[0]
    phentsize, phnum = struct.unpack_from("<HH", elf, 42)
    loaded = []
    for i in range(phnum):
        offset = phoff + i * phentsize
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz = struct.unpack_from("<IIIIII", elf, offset)
        if p_type == 1:
            loaded.append((p_offset, p_vaddr, p_paddr, p_filesz, p_memsz))
            check(p_paddr >= 0x10000 and p_paddr + p_memsz < 0xA0000,
                  f"segment chargeable a {p_paddr:#x} sous la memoire VGA")
    check(bool(loaded), "au moins un segment ELF chargeable")
    for _, vaddr, _, filesz, memsz in loaded:
        check(memsz >= filesz, "BSS compris dans la memoire allouee")
        check(vaddr + filesz <= 0x10000 + 153600, "noyau charge sur les 300 secteurs du chargeur")
    print("Image NexOS validee.")


if __name__ == "__main__":
    main()
