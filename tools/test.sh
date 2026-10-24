#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
make release
python3 tests/test_image.py build/nexos.img build/kernel.elf
QEMU="${QEMU:-qemu-system-i386}"
if ! command -v "$QEMU" >/dev/null 2>&1; then
  echo "QEMU non installé : compilez et testez l'image avec make test; test de boot ignoré." >&2
  exit 0
fi
python3 tests/test_boot.py build/nexos.img "$(command -v "$QEMU")"
