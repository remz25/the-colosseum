#!/usr/bin/env python3
"""Per-character battle palettes: src/graphics/battle_palettes.txt -> build/CharPalettes.event.

FE8 picks a character's battle palette with two parallel tables of 7 bytes per character
(indexed by character ID - 1): 0x95E0A4 lists classes, 0x95EEA4 the palette for each class
(1-based index into the palette list at 0xEF8008: 16-byte entries, name[12] + LZ77 pointer).
When the unit's class is in its row, that palette replaces the class animation's own
(GetBanimPalette / PrepareBattleGraphics, 0x080573FC).

For each character in the manifest, every listed class gets a new palette: the base palette
(the slot's old FE8 character's palette for that class, or the class animation's default) with
colour families recoloured - each source colour takes the target's hue and saturation and keeps
its own lightness, shifted so the family's average lightness matches the target. The character's
class and palette rows are rewritten; the palettes are stored in palette-list entries of FE8
characters who never appear in COLOSSEUM (their pointers are repointed to the new data).
"""
from __future__ import annotations

import colorsys
import re
import struct
from dataclasses import dataclass, field
from pathlib import Path

CLASS_TABLE = 0x807110          # FE8U class data (0x54 per class); anim list pointer at +0x34
CLASS_ROWS = 0x95E0A4
PAL_ROWS = 0x95EEA4
PAL_LIST = 0xEF8008
PAL_LIST_COUNT = 108
ANIM_TABLE = 0xC00008           # battle animations (0x20 each, IDs from 1); palette pointer at +0x1C
ROW = 7
PAL_COLOURS = 80                # 5 palettes of 16 colours, as FE8's character palettes

# FE8 characters that never appear in COLOSSEUM (not in the pool, not bosses of our battles):
# their palette-list entries may be reused. Slot characters are added from the manifest.
UNUSED_CHARACTERS = [0x01, 0x02, 0x0B, 0x0F, 0x17, 0x1A, 0x1C, 0x1D]


class CharPalError(Exception):
    pass


def lz77(d: bytes, off: int) -> bytes:
    if d[off] != 0x10:
        raise CharPalError(f"no LZ77 data at {off:#x}")
    size = d[off + 1] | d[off + 2] << 8 | d[off + 3] << 16
    out = bytearray()
    i = off + 4
    while len(out) < size:
        flags = d[i]
        i += 1
        for bit in range(7, -1, -1):
            if len(out) >= size:
                break
            if flags >> bit & 1:
                b0, b1 = d[i], d[i + 1]
                i += 2
                disp = ((b0 & 0xF) << 8 | b1) + 1
                for _ in range((b0 >> 4) + 3):
                    out.append(out[-disp])
            else:
                out.append(d[i])
                i += 1
    return bytes(out)


def lz77_literal(data: bytes) -> bytes:
    out = bytearray([0x10, len(data) & 0xFF, len(data) >> 8 & 0xFF, len(data) >> 16 & 0xFF])
    for i in range(0, len(data), 8):
        out.append(0)
        out += data[i:i + 8]
    while len(out) % 4:
        out.append(0)
    return bytes(out)


def to_rgb(c: int) -> tuple[int, int, int]:
    return (c & 31) << 3, (c >> 5 & 31) << 3, (c >> 10 & 31) << 3


def to_gba(r: int, g: int, b: int) -> int:
    return (r >> 3) | (g >> 3) << 5 | (b >> 3) << 10


def hexrgb(h: str) -> tuple[int, int, int]:
    return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)


@dataclass
class ColourMap:
    sources: list[tuple[int, int, int]]
    target: tuple[int, int, int]


@dataclass
class CharPal:
    char_id: int
    classes: list[int]
    base: int | None                # character whose palettes are the base; None = class default
    maps: list[ColourMap] = field(default_factory=list)
    line: int = 0
    palettes: list[bytes] = field(default_factory=list)   # per class, 160 bytes
    indices: list[int] = field(default_factory=list)      # palette-list index per class (1-based)


LINE_CHAR = re.compile(r"^char\s+(0x[0-9A-Fa-f]+)\s+classes\s+([0-9A-Fa-fx, ]+?)\s+base\s+(default|0x[0-9A-Fa-f]+)$")
LINE_MAP = re.compile(r"^map\s+((?:[0-9A-Fa-f]{6}\s+)+)->\s+([0-9A-Fa-f]{6})$")


def read_manifest(path: Path) -> list[CharPal]:
    items: list[CharPal] = []
    for n, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        m = LINE_CHAR.match(line)
        if m:
            classes = [int(c, 0) for c in m.group(2).replace(",", " ").split()]
            if not 1 <= len(classes) <= ROW:
                raise CharPalError(f"{path}:{n}: 1 to {ROW} classes")
            items.append(CharPal(int(m.group(1), 0), classes,
                                 None if m.group(3) == "default" else int(m.group(3), 0), line=n))
            continue
        m = LINE_MAP.match(line)
        if m and items:
            items[-1].maps.append(ColourMap([hexrgb(h) for h in m.group(1).split()], hexrgb(m.group(2))))
            continue
        raise CharPalError(f"{path}:{n}: expected 'char ID classes C,... base default|ID' or 'map RRGGBB ... -> RRGGBB'")
    return items


def class_default(rom: bytes, cls: int) -> list[int]:
    lst = struct.unpack_from("<I", rom, CLASS_TABLE + cls * 0x54 + 0x34)[0] - 0x08000000
    anim = struct.unpack_from("<H", rom, lst + 2)[0]
    pal = lz77(rom, struct.unpack_from("<I", rom, ANIM_TABLE + (anim - 1) * 0x20 + 0x1C)[0] - 0x08000000)
    return [struct.unpack_from("<H", pal, 2 * k)[0] for k in range(len(pal) // 2)]


def char_palette(rom: bytes, char_id: int, cls: int) -> list[int] | None:
    o = (char_id - 1) * ROW
    for s in range(ROW):
        if rom[CLASS_ROWS + o + s] == cls and rom[PAL_ROWS + o + s]:
            e = PAL_LIST + (rom[PAL_ROWS + o + s] - 1) * 16
            pal = lz77(rom, struct.unpack_from("<I", rom, e + 12)[0] - 0x08000000)
            return [struct.unpack_from("<H", pal, 2 * k)[0] for k in range(len(pal) // 2)]
    return None


def recolour(colours: list[int], maps: list[ColourMap]) -> list[int]:
    out = list(colours)
    for cm in maps:
        src_hls = [colorsys.rgb_to_hls(*(v / 255 for v in s)) for s in cm.sources]
        mean_l = sum(h[1] for h in src_hls) / len(src_hls)
        th, tl, ts = colorsys.rgb_to_hls(*(v / 255 for v in cm.target))
        wanted = {to_gba(*s) for s in cm.sources}
        for k, c in enumerate(colours):
            if c not in wanted:
                continue
            _, l, _ = colorsys.rgb_to_hls(*(v / 255 for v in to_rgb(c)))
            nl = min(0.97, max(0.05, l + (tl - mean_l)))
            r, g, b = colorsys.hls_to_rgb(th, nl, ts)
            out[k] = to_gba(round(r * 255), round(g * 255), round(b * 255))
    return out


def compile_all(manifest: Path, rom: bytes, out_dir: Path) -> list[CharPal]:
    items = read_manifest(manifest)
    rewritten = {cp.char_id for cp in items}
    # palette-list indices still used by characters we keep
    keep = set()
    for cid in range(1, 0x100):
        if cid in rewritten or cid in UNUSED_CHARACTERS:
            continue
        o = (cid - 1) * ROW
        if PAL_ROWS + o + ROW > len(rom):
            break
        keep.update(p for p in rom[PAL_ROWS + o:PAL_ROWS + o + ROW] if p)
    free: list[int] = []
    for cid in sorted(rewritten) + UNUSED_CHARACTERS:
        o = (cid - 1) * ROW
        for p in rom[PAL_ROWS + o:PAL_ROWS + o + ROW]:
            if p and p not in keep and p not in free and p <= PAL_LIST_COUNT:
                free.append(p)
    for cp in items:
        for cls in cp.classes:
            base = char_palette(rom, cp.base, cls) if cp.base is not None else None
            if base is None:
                base = class_default(rom, cls)
            while len(base) < PAL_COLOURS:                    # class defaults hold fewer palettes
                base = base + base[:16]
            new = recolour(base[:PAL_COLOURS], cp.maps)
            if not free:
                raise CharPalError("no free palette-list entries left")
            cp.palettes.append(b"".join(struct.pack("<H", c) for c in new))
            cp.indices.append(free.pop(0))

    lines = ["// GENERATED by scripts/charpal.py from src/graphics/battle_palettes.txt - do not edit.\n"]
    for cp in items:
        o = (cp.char_id - 1) * ROW
        for k, (cls, idx, data) in enumerate(zip(cp.classes, cp.indices, cp.palettes)):
            label = f"Col_CharPal_{cp.char_id:02X}_{cls:02X}"
            lines.append(f"ALIGN 4\n{label}:\nBYTE " + " ".join(str(b) for b in lz77_literal(data)) + "\n")
            lines.append(f"PUSH\nORG {PAL_LIST + (idx - 1) * 16 + 12:#x}\nPOIN {label}\nPOP\n")
        cls_row = cp.classes + [0] * (ROW - len(cp.classes))
        pal_row = cp.indices + [0] * (ROW - len(cp.indices))
        lines.append(f"PUSH\nORG {CLASS_ROWS + o:#x}\nBYTE {' '.join(str(c) for c in cls_row)}\n"
                     f"ORG {PAL_ROWS + o:#x}\nBYTE {' '.join(str(p) for p in pal_row)}\nPOP\n")
    lines.append("ALIGN 4\n")
    (out_dir / "CharPalettes.event").write_text("".join(lines), encoding="utf-8")
    return items


def verify_rom(items: list[CharPal], rom: bytes) -> list[str]:
    problems = []
    for cp in items:
        o = (cp.char_id - 1) * ROW
        if list(rom[CLASS_ROWS + o:CLASS_ROWS + o + len(cp.classes)]) != cp.classes:
            problems.append(f"character {cp.char_id:#x}: class row not written")
            continue
        for k, (idx, data) in enumerate(zip(cp.indices, cp.palettes)):
            if rom[PAL_ROWS + o + k] != idx:
                problems.append(f"character {cp.char_id:#x}: palette row not written")
                continue
            ptr = struct.unpack_from("<I", rom, PAL_LIST + (idx - 1) * 16 + 12)[0] - 0x08000000
            try:
                if lz77(rom, ptr) != data:
                    problems.append(f"character {cp.char_id:#x} class {cp.classes[k]:#x}: palette data differs")
            except (CharPalError, IndexError):
                problems.append(f"character {cp.char_id:#x} class {cp.classes[k]:#x}: bad palette pointer")
    return problems


def main() -> int:
    """py -3 scripts/charpal.py CHAR_ID CLASS_ID ... : print the base palettes (FE8_clean.gba) the
    manifest recolours - the character's own palette for each class, else the class default."""
    import sys
    rom = (Path(__file__).resolve().parent.parent / "FE8_clean.gba").read_bytes()
    char_id = int(sys.argv[1], 0)
    for cls in (int(a, 0) for a in sys.argv[2:]):
        pal = char_palette(rom, char_id, cls)
        src = "character" if pal else "class default"
        pal = pal or class_default(rom, cls)
        cols = " ".join(f"{k}:{''.join(f'{v:02x}' for v in to_rgb(pal[k]))}" for k in range(16))
        print(f"class {cls:#04x} ({src}): {cols}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
