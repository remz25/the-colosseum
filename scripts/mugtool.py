#!/usr/bin/env python3
"""Portrait (mug) sheet helper: inspect a 128x112 portrait PNG.

    py -3 scripts/mugtool.py src/graphics/portraits/Mira.png

Reports the colour count (GBA portraits need <= 16 colours including the
background) and suggests mouth/eye tile coordinates for the portrait table
by finding where the sheet's mouth and eye frames best match the main face.
The standard sheet layout (FE8 / PortraitFormatter):
    main portrait  (0,0)   96x80
    mini portrait  (96,16) 32x32
    eye frames     (96,48), (96,64)            32x16 each
    mouth frames   (0,80) (32,80) (64,80) (0,96) (32,96) (64,96)  32x16 each
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

MOUTH_FRAMES = [(0, 80), (32, 80), (64, 80), (0, 96), (32, 96), (64, 96)]
EYE_FRAMES = [(96, 48), (96, 64)]


def read_png(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    """Decode a PNG into rows of RGB tuples (indexed 1-8 bit, RGB, RGBA)."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos, idat, plte, trns = 8, b"", None, None
    ihdr = None
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        typ, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + ln]
        if typ == b"IHDR":
            ihdr = body
        elif typ == b"PLTE":
            plte = body
        elif typ == b"IDAT":
            idat += body
        pos += 12 + ln
    w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", ihdr)
    if interlace:
        raise ValueError(f"{path}: interlaced PNGs are not supported")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    bits = depth * channels
    stride = (w * bits + 7) // 8
    bpp = max(1, bits // 8)
    raw = zlib.decompress(idat)
    rows, prev, p = [], bytearray(stride), 0
    for _ in range(h):
        f, line = raw[p], bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(bytes(line))
        prev = line
    pix = []
    for row in rows:
        out = []
        for x in range(w):
            if ctype == 3:
                per = 8 // depth
                v = (row[x // per] >> (8 - depth * (x % per + 1))) & ((1 << depth) - 1) if depth < 8 else row[x]
                out.append(tuple(plte[3 * v:3 * v + 3]))
            elif ctype in (2, 6):
                o = x * channels
                out.append(tuple(row[o:o + 3]))
            else:
                g = row[x * channels]
                out.append((g, g, g))
        pix.append(out)
    return w, h, pix


def region(pix, x0, y0, w, h):
    return [pix[y][x0:x0 + w] for y in range(y0, y0 + h)]


def diff(a, b):
    return sum(1 for ra, rb in zip(a, b) for pa, pb in zip(ra, rb) if pa != pb)


def best_tile_position(pix, frame):
    """Best 8px-aligned 32x16 position of `frame` inside the 96x80 main portrait."""
    best = None
    for ty in range(0, 80 // 8 - 1):
        for tx in range(0, 96 // 8 - 3):
            d = diff(region(pix, tx * 8, ty * 8, 32, 16), frame)
            if best is None or d < best[0]:
                best = (d, tx, ty)
    return best


def analyse(path: Path) -> dict:
    w, h, pix = read_png(path)
    if (w, h) != (128, 112):
        raise ValueError(f"{path}: sheet is {w}x{h}; portraits must be 128x112")
    colours = {c for row in pix for c in row}
    mouth = [best_tile_position(pix, region(pix, x, y, 32, 16)) for x, y in MOUTH_FRAMES]
    eyes = [best_tile_position(pix, region(pix, x, y, 32, 16)) for x, y in EYE_FRAMES]
    return {"colours": len(colours), "mouth": mouth, "eyes": eyes}


def main() -> int:
    for arg in sys.argv[1:]:
        r = analyse(Path(arg))
        print(f"{arg}: {r['colours']} colours{'  (TOO MANY: max 16)' if r['colours'] > 16 else ''}")
        for label, key in (("mouth", "mouth"), ("eyes ", "eyes")):
            for d, tx, ty in r[key]:
                print(f"  {label} frame best at tile ({tx},{ty})  differing pixels {d}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
