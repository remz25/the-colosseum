#!/usr/bin/env python3
"""Static arena checks on a built ROM (docs/ARENAS.md): reads gColArenas (src/arenas/arenas.c)
and each arena's map and tile config, and checks for every arena:

  - the map is at least 15x10 and every spawn / hazard / sacred tile is inside it;
  - the 6 spawn tiles are distinct, not on hazard tiles, and a unit of every tested class can
    stand on them (movement cost > 0) in every weather the arena can roll (FE8's rain and snow
    movement tables);
  - hazard and sacred tiles can be stood on (on foot) - otherwise they could never matter;
  - every player spawn can reach every enemy spawn on foot and in armour.

    py -3 tests/check_arenas.py [ROM]       (default: Colosseum_test.gba; run_tests.py calls it)
"""
from __future__ import annotations

import struct
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
from arenas import lz77_decode, CHAPTER_TABLE, CHAPTER_SIZE, ASSET_TABLE   # noqa: E402

CLASS_TABLE = 0x807110
CLASS_SIZE = 0x54
CLASSES = {"Mercenary": 0x0F, "Knight": 0x09, "Cavalier": 0x05, "Archer": 0x19, "Mage": 0x25,
           "Fighter": 0x3F}
PATH_CLASSES = ("Mercenary", "Knight")
WEATHER_TABLE = {0: 0, 1: 1, 2: 2, 3: 0, 4: 0, 5: 0}   # COLOSSEUM weather -> movement table (0 normal, 1 rain, 2 snow)
TILE_NAMES = {1: "burning", 2: "poison", 3: "void", 4: "sacred"}
ARENA_SIZE = 36


def read_sym(path: Path) -> dict:
    syms = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split()
        if len(parts) == 2:
            try:
                syms[parts[1]] = int(parts[0], 16)
            except ValueError:
                pass
    return syms


def cstr(rom: bytes, ptr: int) -> str:
    if not ptr:
        return ""
    o = ptr - 0x08000000
    return rom[o:rom.index(b"\0", o)].decode("latin-1")


def check(rom_path: Path) -> list[str]:
    rom = rom_path.read_bytes()
    syms = read_sym(rom_path.with_suffix(".sym"))
    if "gColArenas" not in syms or "Col_ArenaCount" not in syms:
        return [f"gColArenas not in {rom_path.with_suffix('.sym').name}"]
    base = syms["gColArenas"] - 0x08000000
    # the table ends where the next symbol begins; count = entries with a valid name pointer
    problems, arenas = [], []
    for i in range(32):
        e = rom[base + i * ARENA_SIZE: base + (i + 1) * ARENA_SIZE]
        name_p, hint_p, tiles_p = struct.unpack_from("<III", e, 0)
        if not 0x08000000 <= name_p < 0x0A000000:
            break
        arenas.append((i, e, cstr(rom, name_p), tiles_p))
    if len(arenas) < 8:
        problems.append(f"only {len(arenas)} arenas found")
    for i, e, name, tiles_p in arenas:
        chapter, asset = e[12], e[13]
        weathers = [w for w in range(6) if e[15 + w]]
        spawns = [(e[21 + 2 * k], e[22 + 2 * k]) for k in range(6)]
        mp = lz77_decode(rom, struct.unpack_from("<I", rom, ASSET_TABLE + 4 * asset)[0] - 0x08000000)
        w, h = mp[0], mp[1]
        meta = [[struct.unpack_from("<H", mp, 2 + 2 * (y * w + x))[0] >> 2 for x in range(w)] for y in range(h)]
        cfg_id = rom[CHAPTER_TABLE + chapter * CHAPTER_SIZE + 0x07]
        cfg = lz77_decode(rom, struct.unpack_from("<I", rom, ASSET_TABLE + 4 * cfg_id)[0] - 0x08000000)
        terrain = [[cfg[0x2000 + m] for m in row] for row in meta]
        tiles = {}
        o = tiles_p - 0x08000000 if tiles_p else None
        while o is not None and rom[o + 2]:
            tiles[(rom[o], rom[o + 1])] = rom[o + 2]
            o += 4
        tag = f"arena {i} ({name})"
        if w < 15 or h < 10:
            problems.append(f"{tag}: map {w}x{h} is smaller than 15x10")
        if not weathers:
            problems.append(f"{tag}: no weather can be rolled")
        if len(set(spawns)) != 6:
            problems.append(f"{tag}: spawn tiles are not distinct {spawns}")
        for (x, y), kind in tiles.items():
            if not (x < w and y < h):
                problems.append(f"{tag}: {TILE_NAMES.get(kind, kind)} tile ({x},{y}) outside the map")
        for p in spawns:
            if not (p[0] < w and p[1] < h):
                problems.append(f"{tag}: spawn {p} outside the map")
                continue
            if tiles.get(p, 0) in (1, 2, 3):
                problems.append(f"{tag}: spawn {p} is on a {TILE_NAMES[tiles[p]]} tile")

        def cost_table(cls: int, table: int) -> bytes:
            ptr = struct.unpack_from("<I", rom, CLASS_TABLE + cls * CLASS_SIZE + 0x38 + 4 * table)[0]
            return rom[ptr - 0x08000000: ptr - 0x08000000 + 0x41]

        for cname, cls in CLASSES.items():
            for wx in weathers:
                costs = cost_table(cls, WEATHER_TABLE[wx])

                def ok(x, y):
                    c = costs[terrain[y][x]]
                    return 0 < c < 0x80

                for p in spawns:
                    if p[0] < w and p[1] < h and not ok(*p):
                        problems.append(f"{tag}: a {cname} cannot stand on spawn {p} "
                                        f"(terrain {terrain[p[1]][p[0]]:#x}, weather {wx})")
                if cname == "Mercenary":
                    for (x, y), kind in tiles.items():
                        if x < w and y < h and not ok(x, y):
                            problems.append(f"{tag}: {TILE_NAMES.get(kind, kind)} tile ({x},{y}) cannot be stood on")
                if cname in PATH_CLASSES:
                    start = spawns[0]
                    seen = {start}
                    q = deque([start])
                    while q:
                        x, y = q.popleft()
                        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                            if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen and ok(nx, ny):
                                seen.add((nx, ny))
                                q.append((nx, ny))
                    for p in spawns[1:]:
                        if p not in seen:
                            problems.append(f"{tag}: a {cname} at {start} cannot reach {p} (weather {wx})")
    return problems


def main() -> int:
    rom = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "Colosseum_test.gba"
    problems = check(rom)
    for p in problems:
        print(f"  [FAIL] {p}")
    if not problems:
        print("  [PASS] arenas: maps, spawn tiles, hazard/sacred tiles and paths (every tested class and weather)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
