#!/usr/bin/env python3
"""NexOS : construction d'une ISO amorçable El Torito via pycdlib.

Repli local quand xorriso/genisoimage/mkisofs est absent :
    python3 tools/make_iso.py build/iso/nexos.img build/nexos.iso

Le fichier passé en argument doit être l'image disquette de 1,44 Mo
(boot.bin + chargeur + noyau) ; le BIOS la monte en émulation disquette
au moment du boot El Torito, exactement comme avec mkisofs -b.
"""
import sys

try:
    import pycdlib
except ImportError:
    sys.exit("pycdlib manquant : python3 -m pip install --target .venv_pip pycdlib")


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit("usage: make_iso.py <image-disquette> <sortie.iso>")
    floppy_path, out_path = sys.argv[1], sys.argv[2]

    with open(floppy_path, "rb") as f:
        floppy = f.read()
    if len(floppy) != 2880 * 512:
        sys.exit("l'image doit faire exactement 1,44 Mo (2880 secteurs)")

    iso = pycdlib.PyCdlib()
    iso.new(vol_ident="NEXOS", interchange_level=1, joliet=True)
    # Le fichier présent dans l'ISO doit avoir un nom ISO9660 valide (8.3).
    iso.add_file(floppy_path, iso_path="/NEXOS.IMG;1")
    iso.add_eltorito(
        bootfile_path="/NEXOS.IMG;1",
        media_name="floppy",
        bootable=True,
        # En emulation disquette le BIOS charge toute l'image : count = 2880 secteurs.
        boot_load_seg=0,
        boot_load_size=2880,
    )
    iso.write(out_path)
    print(f"ISO El Torito (emulation disquette) ecrite : {out_path}")


if __name__ == "__main__":
    main()
