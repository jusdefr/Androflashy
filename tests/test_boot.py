#!/usr/bin/env python3
"""Démarre le binaire réel dans QEMU et vérifie ses diagnostics COM1."""
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> None:
    if len(sys.argv) < 3:
        raise SystemExit("Usage: test_boot.py build/nexos.img /chemin/vers/qemu-system-i386 [options QEMU...]")
    image, qemu, *extra = Path(sys.argv[1]), sys.argv[2], *sys.argv[3:]
    with tempfile.TemporaryDirectory(prefix="nexos-qemu-") as tmp:
        serial = Path(tmp) / "serial.log"
        command = [qemu, *extra, "-machine", "pc", "-m", "32M", "-display", "none", "-monitor", "none",
                   "-no-reboot", "-no-shutdown", "-serial", f"file:{serial}", "-drive",
                   f"file={image},format=raw,if=floppy", "-boot", "a"]
        try:
            subprocess.run(command, timeout=5, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except subprocess.TimeoutExpired:
            pass  # le shell est interactif ; QEMU doit continuer à tourner
        text = serial.read_text(errors="replace") if serial.exists() else ""
        print(text, end="")
        required = ["NexOS boot", "[NexOS/INFO] initialisation du noyau",
                    "[NexOS/INFO] allocateur memoire initialise", "[NexOS/INFO] IDT chargee",
                    "[NexOS/INFO] PIT configure", "[NexOS/INFO] tache shell prete"]
        missing = [entry for entry in required if entry not in text]
        if missing:
            raise SystemExit("ÉCHEC boot : diagnostics série absents: " + ", ".join(missing))
        if "KERNEL PANIC" in text or "erreur de lecture disque" in text:
            raise SystemExit("ÉCHEC boot : panique noyau ou erreur BIOS")
        print("OK: QEMU a booté le véritable noyau et initialisé ses services.")


if __name__ == "__main__":
    main()
