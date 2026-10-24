#!/usr/bin/env python3
"""Vérifie qu'une vraie commande shell traverse le clavier PS/2 émulé."""
import json
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def main() -> None:
    if len(sys.argv) < 3:
        raise SystemExit("Usage: test_shell.py build/nexos.img /chemin/vers/qemu-system-i386 [options QEMU...]")
    image, qemu, *extra = Path(sys.argv[1]), sys.argv[2], *sys.argv[3:]
    with tempfile.TemporaryDirectory(prefix="nexos-shell-") as tmp:
        serial, qmp_path = Path(tmp) / "serial.log", Path(tmp) / "qmp.sock"
        cmd = [qemu, *extra, "-machine", "pc", "-m", "32M", "-display", "none", "-monitor", "none",
               "-no-reboot", "-no-shutdown", "-serial", f"file:{serial}", "-qmp",
               f"unix:{qmp_path},server=on,wait=off", "-drive", f"file={image},format=raw,if=floppy", "-boot", "a"]
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            client.settimeout(2)
            deadline = time.monotonic() + 5
            while True:
                try:
                    client.connect(str(qmp_path))
                    break
                except (FileNotFoundError, ConnectionRefusedError):
                    if proc.poll() is not None or time.monotonic() >= deadline:
                        raise SystemExit("QEMU/QMP n'a pas démarré")
                    time.sleep(0.05)
            stream = client.makefile("rwb", buffering=0)
            if not json.loads(stream.readline()).get("QMP"):
                raise SystemExit("Bannière QMP invalide")

            def qmp(command: str, args: dict | None = None) -> dict:
                request = {"execute": command}
                if args is not None: request["arguments"] = args
                stream.write(json.dumps(request).encode() + b"\r\n")
                while True:
                    response = json.loads(stream.readline())
                    if "return" in response: return response
                    if "error" in response: raise SystemExit(f"QMP: {response['error']}")

            qmp("qmp_capabilities")
            deadline = time.monotonic() + 5
            while "[NexOS/INFO] noyau pret" not in (serial.read_text(errors="replace") if serial.exists() else ""):
                if time.monotonic() >= deadline: raise SystemExit("Noyau non démarré")
                time.sleep(0.05)
            # QMP envoie make+break scancode en une requête. Délai volontaire
            # entre les touches pour laisser l'IRQ PS/2 consommer chaque octet.
            for key in "about":
                qmp("send-key", {"keys": [{"type": "qcode", "data": key}]})
                time.sleep(0.12)
            qmp("send-key", {"keys": [{"type": "qcode", "data": "ret"}]})
            deadline = time.monotonic() + 2
            while time.monotonic() < deadline:
                log = serial.read_text(errors="replace") if serial.exists() else ""
                if "SHELL_RESULT NexOS 0.1.0-dev" in log:
                    print("OK: QEMU → clavier PS/2 → shell → commande about.")
                    return
                time.sleep(0.05)
            raise SystemExit("Commande shell non exécutée; trace série:\n" + log)
        finally:
            client.close()
            if proc.poll() is None:
                proc.terminate()
                try: proc.wait(timeout=2)
                except subprocess.TimeoutExpired: proc.kill()


if __name__ == "__main__":
    main()
