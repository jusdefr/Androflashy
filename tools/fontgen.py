#!/usr/bin/env python3
"""Convertit une fonte console PSF 8x16 en tableau C freestanding, indexé Latin-1.

Gère PSF1 (entête 4 octets, table Unicode optionnelle) et PSF2. La table Unicode
de la fonte est utilisée pour remapper les glyphes de 0xA0..0xFF à leurs vrais
codes Latin-1 (é, è, à, ç…). Usage : python3 tools/fontgen.py <fonte.psf> <sortie.c>
"""
import gzip
import struct
import sys
from pathlib import Path


def decode_ucs2_table(table: bytes, glyph_count: int):
    """Table console-data : mots UCS-2 petit-boutisme, entrées séparées par 0xFFFF.
    Renvoie {codepoint: position de glyphe}."""
    codepoint_to_glyph = {}
    cur = []
    pos = 0
    i = 0
    while i + 1 < len(table):
        word = table[i] | (table[i + 1] << 8)
        i += 2
        if word == 0xFFFF:
            if cur:
                for code in cur:
                    if code not in codepoint_to_glyph:
                        codepoint_to_glyph[code] = pos
                pos += 1
            cur = []
        elif word == 0xFFFE:  # séparateur interne (séquences combinées)
            continue
        else:
            cur.append(word)
    if cur:
        for code in cur:
            if code not in codepoint_to_glyph:
                codepoint_to_glyph[code] = pos
    # Comble l'ASCII non listé : position == code de code.
    for code in range(0x20, 0x7F):
        codepoint_to_glyph.setdefault(code, code)
    return codepoint_to_glyph


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("Usage: fontgen.py <fonte.psf> <sortie.c>")
    raw = gzip.decompress(Path(sys.argv[1]).read_bytes())
    unicode_table = None
    if raw[:2] == b"\x36\x04":  # PSF1 : largeur fixe 8, octet 3 = hauteur
        height = raw[3]
        mode = raw[2]
        # Le bit « 512 glyphes » du mode est parfois trompeur : on exige la place.
        glyph_count = 512 if mode & 0x02 and len(raw) >= 4 + 512 * height else 256
        data, header = raw[4:4 + glyph_count * height], 4
        width, char_size = 8, height
        # La table Unicode est présente dès que des octets restent après les glyphes.
        if len(raw) > 4 + glyph_count * height:
            unicode_table = raw[4 + glyph_count * height:]
    elif raw[:4] == b"\x72\xb5\x4a\x86":  # PSF2
        header_size, glyph_count, char_size, height, width = struct.unpack_from("<IIIII", raw, 4)
        data = raw[header_size:header_size + glyph_count * char_size]
        if len(raw) > header_size + glyph_count * char_size:
            unicode_table = raw[header_size + glyph_count * char_size:]
    else:
        raise SystemExit("Magie PSF inconnue")
    if width != 8 or height != 16 or char_size != 16:
        raise SystemExit(f"Fonte inattendue : {width}x{height} (char={char_size})")

    # Remap Unicode -> positions de glyphes (table console-data UCS-2 LE).
    codepoint_to_glyph = {} if unicode_table is None else decode_ucs2_table(unicode_table, glyph_count)

    glyphs = [data[i * char_size:(i + 1) * char_size] for i in range(glyph_count)]

    def glyph_at(code: int) -> bytes:
        position = code
        if codepoint_to_glyph and code >= 0x80:
            mapped = codepoint_to_glyph.get(code)
            if mapped is not None:
                position = mapped
        if position >= glyph_count:
            position = ord('?')
        return glyphs[position] if position < glyph_count else glyphs[ord('?')]

    out = [
        "/* Généré par tools/fontgen.py depuis une fonte console PSF (8x16).",
        " * Indexé Latin-1 : les codes 0xA0..0xFF sont remappés via la table",
        " * Unicode de la fonte. Format : 16 octets par glyphe, bits 7..0",
        " * = colonnes gauche à droite.",
        " */",
        "#include \"kernel.h\"",
        "",
        "const uint8_t font8x16[256][16] = {",
    ]
    for index in range(256):
        rows = glyph_at(index)
        rowstr = ", ".join(f"0x{b:02X}" for b in rows)
        label = f"/* {index:03X} */" if index < 0x20 or index >= 0x7F else f"/* {chr(index)} */"
        out.append(f"    {{{rowstr}}},{label}")
    out.append("};")
    Path(sys.argv[2]).write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"font8x16.c généré : {glyph_count} glyphes natifs, table Unicode utilisée.")


if __name__ == "__main__":
    main()
