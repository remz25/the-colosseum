#!/usr/bin/env python3
"""Build the release package: player ROM -> UPS patch -> verified -> The_Colosseum_vX.Y.Z.zip.

    py -3 scripts/make_release.py [--skip-build]

Steps (each one is checked; any failure stops the script):
  1. Build the player ROM into build/release/ (scripts/build.py --output; Colosseum.gba may
     stay open in mGBA).
  2. Check FE8_clean.gba is the expected base ROM (size + SHA-1).
  3. Create the UPS patch; apply it to the clean ROM and compare with the built ROM.
  4. Check the docs in release/: all present, the version and the patched ROM's size and CRC32
     match, no placeholder left.
  5. Assemble the zip (docs, patch, release/screenshots/*.png) - never a ROM or a save.
  6. Extract the zip to a temporary folder, check its file list, and apply the extracted patch
     to the clean ROM again.
Output: build/The_Colosseum_vX.Y.Z.zip
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
import ups  # noqa: E402

VERSION = "v0.1.0"
NAME = f"The_Colosseum_{VERSION}"
CLEAN = ROOT / "FE8_clean.gba"
CLEAN_SIZE = 16_777_216
CLEAN_SHA1 = "c25b145e37456171ada4b0d440bf88a19f4d509f"
DOCS = ["README.txt", "PATCHING_GUIDE.txt", "RELEASE_NOTES.txt", "CHANGELOG.txt",
        "KNOWN_ISSUES.txt", "CREDITS.txt"]
NOT_SHIPPED = ["FEUNIVERSE_POST.txt"]           # kept in release/ for the forum post, not zipped
PLACEHOLDERS = ["[YOUR NAME]", "TODO", "TBD"]
FORBIDDEN = (".gba", ".sav", ".sgm", ".elf", ".sym")


def step(msg: str) -> None:
    print(f"[ .. ] {msg}", flush=True)


def ok(msg: str) -> None:
    print(f"[ ok ] {msg}", flush=True)


def fail(msg: str) -> int:
    print(f"[FAIL] {msg}", flush=True)
    return 1


def crc32(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--skip-build", action="store_true", help="reuse build/release/built.gba")
    args = ap.parse_args()

    out_dir = ROOT / "build" / "release"
    out_dir.mkdir(parents=True, exist_ok=True)
    built_path = out_dir / "built.gba"

    # 1. build
    if not args.skip_build:
        step("building the player ROM")
        r = subprocess.run([sys.executable, str(ROOT / "scripts" / "build.py"), "--output", str(built_path)],
                           cwd=ROOT)
        if r.returncode:
            return fail("build failed")
    if not built_path.exists():
        return fail(f"{built_path} missing (build first)")
    built = built_path.read_bytes()
    ok(f"built ROM: {len(built):,} bytes, CRC32 {crc32(built)}")

    # 2. base ROM
    clean = CLEAN.read_bytes()
    if len(clean) != CLEAN_SIZE or hashlib.sha1(clean).hexdigest() != CLEAN_SHA1:
        return fail("FE8_clean.gba is not the clean FE8 (USA) ROM")
    ok("base ROM is clean FE8 (USA), SHA-1 matches")

    # 3. patch
    patch = ups.create(clean, built)
    if ups.apply(clean, patch) != built:
        return fail("clean ROM + patch != built ROM")
    try:
        ups.apply(clean[:-1] + bytes([clean[-1] ^ 0xFF]), patch)
        return fail("the patch accepted a modified base ROM")
    except ups.UpsError:
        pass
    ok(f"patch: {len(patch):,} bytes; clean + patch = built ROM; wrong base rejected")

    # 4. docs
    rel = ROOT / "release"
    for d in DOCS + NOT_SHIPPED:
        if not (rel / d).exists():
            return fail(f"release/{d} missing")
        text = (rel / d).read_text(encoding="utf-8")
        for p in PLACEHOLDERS:
            if p in text:
                return fail(f"release/{d} still contains the placeholder {p!r}")
    guide = (rel / "PATCHING_GUIDE.txt").read_text(encoding="utf-8")
    if f"{len(built):,}" not in guide or crc32(built) not in guide:
        return fail(f"PATCHING_GUIDE.txt must state the patched ROM's size {len(built):,} and CRC32 {crc32(built)}")
    for d in ("README.txt", "RELEASE_NOTES.txt", "CHANGELOG.txt"):
        if VERSION not in (rel / d).read_text(encoding="utf-8"):
            return fail(f"release/{d} does not mention {VERSION}")
    shots = sorted((rel / "screenshots").glob("*.png"))
    if not shots:
        return fail("release/screenshots/ has no screenshots")
    ok(f"docs checked; {len(shots)} screenshots")

    # 5. zip
    zip_path = ROOT / "build" / f"{NAME}.zip"
    step(f"writing {zip_path.relative_to(ROOT)}")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr(f"{NAME}/{NAME}.ups", patch)
        for d in DOCS:
            z.write(rel / d, f"{NAME}/{d}")
        for s in shots:
            z.write(s, f"{NAME}/screenshots/{s.name}")

    # 6. verify the zip from scratch
    tmp = Path(tempfile.mkdtemp(prefix="colosseum_release_"))
    try:
        with zipfile.ZipFile(zip_path) as z:
            names = z.namelist()
            z.extractall(tmp)
        bad = [n for n in names if n.lower().endswith(FORBIDDEN)]
        if bad:
            return fail(f"forbidden files in the zip: {bad}")
        expected = {f"{NAME}/{NAME}.ups"} | {f"{NAME}/{d}" for d in DOCS} \
            | {f"{NAME}/screenshots/{s.name}" for s in shots}
        if set(names) != expected:
            return fail(f"zip contents differ: extra {set(names) - expected}, missing {expected - set(names)}")
        extracted_patch = (tmp / NAME / f"{NAME}.ups").read_bytes()
        if ups.apply(clean, extracted_patch) != built:
            return fail("the extracted patch does not reproduce the built ROM")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    size = zip_path.stat().st_size
    ok(f"{zip_path.name}: {len(names)} files, {size:,} bytes; extracted and re-verified; no ROMs or saves")
    return 0


if __name__ == "__main__":
    sys.exit(main())
