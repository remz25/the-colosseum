#!/usr/bin/env python3
"""COLISEUM build: the Skill System buildfile steps (MAKE_HACK_full.cmd), run from the
right folders with every tool named explicitly and every step checked.

    py -3 scripts/build.py           full build (tables, text, maps, assemble)
    py -3 scripts/build.py --quick   assemble only (no tables/text/maps)
    py -3 scripts/build.py --test    test build (links src/tests/: on-target unit tests, run by
                                     tests/run_tests.py) -> Coliseum_test.gba
    py -3 scripts/build.py --debug   debug build (defines __DEBUG__: Skill System debug menu and
                                     COLISEUM debug commands) -> Coliseum_debug.gba. The player ROM
                                     Coliseum.gba never contains debug tools (spec 81).

Why not MAKE_HACK_full.cmd: the repo ships extensionless Linux builds next to the Windows
.exe files (EventAssembler/ColorzCore, ParseFile, Png2Dmp), and cmd.exe can pick the
extensionless file, so the batch file's "ColorzCore A FE8 ..." fails silently and leaves an
unmodified ROM. This script also refuses to report success unless the assembler says
"No errors" and the output ROM is valid.

Output: Coliseum.gba (+ Coliseum.sym). The clean base ROM is FE8_clean.gba (git-ignored).
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CLEAN = ROOT / "FE8_clean.gba"
CLEAN_CRC = 0xA47246AE                      # FE8U (USA), 16 MiB
OUT = ROOT / "Coliseum.gba"
TMP = ROOT / "Coliseum.tmp.gba"
SYM = ROOT / "Coliseum.sym"
DEBUG_OUT = ROOT / "Coliseum_debug.gba"
TEST_OUT = ROOT / "Coliseum_test.gba"
EA = ROOT / "EventAssembler"
TOOLS = ROOT / "Tools"
SRC = ROOT / "src"
GEN = ROOT / "build"
CLIB = TOOLS / "FE-Clib"
LYN = EA / "Tools" / "lyn.exe"
ARM_TC = Path(os.environ.get("ARM_TOOLCHAIN",
              r"C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin"))
CFLAGS = ["-mcpu=arm7tdmi", "-mthumb", "-mthumb-interwork", "-mtune=arm7tdmi", "-mlong-calls", "-O2",
          "-ffreestanding", "-fno-builtin", "-fno-tree-loop-distribute-patterns", "-fno-common",
          "-Wall", "-Wextra", "-Wno-unused-parameter", "-Werror", "-std=gnu11",
          # The Skill System's battle calc loop (BattleGenerate and everything calling it) returns
          # with r11 zeroed instead of preserved (seen in tests/run_tests.py's combat tests), so
          # our code must never keep a value in r11 across an engine call.
          "-ffixed-r11"]


class BuildError(Exception):
    pass


def info(msg: str) -> None:
    print(f"[build] {msg}", flush=True)


def ok(msg: str) -> None:
    print(f"[ ok  ] {msg}", flush=True)


def run(cmd: list, cwd: Path, what: str, stdin: str = "\n") -> str:
    exe = Path(cmd[0])
    if not exe.is_file():
        raise BuildError(f"{what}: tool not found: {exe}")
    r = subprocess.run([str(c) for c in cmd], cwd=cwd, input=stdin, capture_output=True,
                       text=True, errors="replace")
    out = (r.stdout or "") + (r.stderr or "")
    if r.returncode != 0:
        raise BuildError(f"{what} failed (exit {r.returncode}):\n{out.strip()[-3000:]}")
    return out


def check_clean_rom() -> bytes:
    if not CLEAN.is_file():
        raise BuildError(f"{CLEAN.name} not found. Put a clean FE8U ROM there (CRC32 {CLEAN_CRC:08X}).")
    data = CLEAN.read_bytes()
    crc = zlib.crc32(data) & 0xFFFFFFFF
    if crc != CLEAN_CRC:
        raise BuildError(f"{CLEAN.name} is not a clean FE8U ROM (CRC32 {crc:08X}, expected {CLEAN_CRC:08X}).")
    return data


def build_tables() -> None:
    info("Processing tables (c2ea)")
    run([TOOLS / "C2EA" / "c2ea.exe", CLEAN, "-installer", ROOT / "Tables" / "TableInstaller.event"],
        ROOT / "Tables", "c2ea")
    ok("Tables")


def build_text() -> None:
    info("Processing text (text-process-classic)")
    run([TOOLS / "TextProcess" / "text-process-classic.exe", "text_buildfile.txt",
         "--parser-exe", EA / "Tools" / "ParseFileUTF8.exe",
         "--installer", "InstallTextData.event", "--definitions", "TextDefinitions.event"],
        ROOT / "Text", "text-process")
    ok("Text")


def build_maps() -> None:
    info("Processing maps (tmx2ea)")
    run([TOOLS / "tmx2ea" / "tmx2ea.exe", "-s", "-O", "MasterMapInstaller.event"], ROOT / "Maps", "tmx2ea")
    ok("Maps")


def build_c(test: bool) -> int:
    """src/**/*.c -> objects -> lyn (linked against the FE-CLib reference) -> build/Coliseum.lyn.event.
    Code must not need RAM variables (.data/.bss): state lives in the COLISEUM RAM block."""
    info("Compiling src/ (arm-none-eabi-gcc + lyn)" + (" [with tests]" if test else ""))
    gcc, objdump = ARM_TC / "arm-none-eabi-gcc.exe", ARM_TC / "arm-none-eabi-objdump.exe"
    ref = CLIB / "reference" / "fireemblem8.ref.o"
    if not (CLIB / "include" / "global.h").is_file():
        raise BuildError("FE-CLib missing: run `git submodule update --init Tools/FE-Clib`")
    obj_dir = GEN / "obj"
    obj_dir.mkdir(parents=True, exist_ok=True)
    sources = sorted(f for f in SRC.rglob("*.c") if test or "tests" not in f.relative_to(SRC).parts)
    parts = ["// GENERATED by scripts/build.py from src/ - do not edit.\n"]
    for c in sources:
        rel = c.relative_to(SRC)
        obj = obj_dir / (str(rel.with_suffix("")).replace("\\", "_").replace("/", "_") + ".o")
        run([gcc, *CFLAGS, *(["-DCOLISEUM_TESTS"] if test else []),
             "-I", SRC / "include", "-I", CLIB / "include", "-c", c, "-o", obj], ROOT, f"compiling {rel}")
        sections = run([objdump, "-h", obj], ROOT, "objdump")
        for line in sections.splitlines():
            f = line.split()
            if len(f) > 2 and f[1].startswith((".data", ".bss")) and int(f[2], 16):
                raise BuildError(f"{rel}: has a {f[1]} section ({int(f[2], 16)} bytes); "
                                 "keep state in the COLISEUM RAM block (coliseum.h)")
        lyn = run([LYN, obj, ref], ROOT, f"lyn {rel}", stdin="")
        parts.append(f"\n// ---- {rel} ----\n{lyn}")
    (GEN / "Coliseum.lyn.event").write_text("".join(parts), encoding="utf-8")
    ok(f"Compiled {len(sources)} C file(s)")
    return len(sources)


def assemble(clean: bytes, debug: bool = False) -> bytes:
    info("Assembling ROMBuildfile.event (ColorzCore)" + (" [DEBUG build]" if debug else ""))
    TMP.unlink(missing_ok=True)
    TMP.write_bytes(clean)
    out = run([EA / "ColorzCore.exe", "A", "FE8", f"-output:{TMP}", f"-input:{ROOT / 'ROMBuildfile.event'}",
               "--nocash-sym", *(["-D:__DEBUG__=1"] if debug else [])], EA, "ColorzCore")
    for line in out.splitlines():
        if line.startswith("message:"):
            print("        " + line.split(": ", 2)[-1], flush=True)
    if "No errors" not in out or any(l.lower().startswith("error") for l in out.splitlines()):
        TMP.unlink(missing_ok=True)
        raise BuildError("ColorzCore reported errors:\n" + out.strip()[-3000:])
    warnings = [l for l in out.splitlines() if l.lower().startswith("warning")]
    if warnings:
        print(f"[warn ] {len(warnings)} assembler warning(s):", *warnings[:10], sep="\n        ")
    data = TMP.read_bytes()
    if data == clean:
        TMP.unlink(missing_ok=True)
        raise BuildError("Output ROM is identical to the clean ROM: nothing was assembled.")
    if data[0xAC:0xB0] != b"BE8E":
        TMP.unlink(missing_ok=True)
        raise BuildError("Output ROM header game code was overwritten.")
    if not 0x1000000 <= len(data) <= 0x2000000:
        TMP.unlink(missing_ok=True)
        raise BuildError(f"Output ROM size {len(data):#x} is outside 16-32 MiB.")
    return data


def finish(data: bytes, out: Path) -> None:
    try:
        os.replace(TMP, out)
    except PermissionError:
        raise BuildError(f"Could not replace {out.name}; it is probably open in mGBA. Close it and build again.")
    sym_src = TMP.with_suffix(".sym")
    if sym_src.is_file():
        shutil.move(sym_src, out.with_suffix(".sym"))
    ok(f"Built {out.name} ({len(data):,} bytes, CRC32 {zlib.crc32(data) & 0xFFFFFFFF:08X})")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--quick", action="store_true", help="assemble only (skip tables, text, maps)")
    ap.add_argument("--debug", action="store_true", help="debug build -> Coliseum_debug.gba")
    ap.add_argument("--test", action="store_true", help="test build with on-target unit tests -> Coliseum_test.gba")
    args = ap.parse_args()
    try:
        clean = check_clean_rom()
        if not args.quick:
            build_tables()
            build_text()
            build_maps()
        build_c(args.test)
        out = TEST_OUT if args.test else DEBUG_OUT if args.debug else OUT
        finish(assemble(clean, args.debug or args.test), out)
    except BuildError as e:
        print(f"[FAIL ] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
