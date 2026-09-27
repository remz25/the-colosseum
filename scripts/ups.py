#!/usr/bin/env python3
"""UPS patches (byuu's format, as used by Flips / NUPS / Rom Patcher JS).

    py -3 scripts/ups.py create SOURCE TARGET PATCH
    py -3 scripts/ups.py apply  SOURCE PATCH OUTPUT

Format: "UPS1", varint source size, varint target size, then hunks (varint count of unchanged
bytes to skip, XOR bytes up to a 0 terminator; the terminator's byte is unchanged too), then
CRC32 of source, target and of the patch itself (little-endian). A target larger than the source
reads the missing source bytes as 0.

`apply` is written separately from `create` (it follows the format, not the creator's code), and
checks all three CRCs, so a patch that applies here is a correct UPS file.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path


class UpsError(Exception):
    pass


def _varint(n: int) -> bytes:
    out = bytearray()
    while True:
        x = n & 0x7F
        n >>= 7
        if n == 0:
            out.append(0x80 | x)
            return bytes(out)
        out.append(x)
        n -= 1


def create(source: bytes, target: bytes) -> bytes:
    out = bytearray(b"UPS1")
    out += _varint(len(source)) + _varint(len(target))
    size = len(target)
    pos = last = 0
    while pos < size:
        a = source[pos] if pos < len(source) else 0
        if a == target[pos]:
            pos += 1
            continue
        out += _varint(pos - last)
        while pos < size:
            a = source[pos] if pos < len(source) else 0
            x = a ^ target[pos]
            if x == 0:
                break
            out.append(x)
            pos += 1
        out.append(0)
        pos += 1                                # the terminator stands for one unchanged byte
        last = pos
    out += struct.pack("<II", zlib.crc32(source) & 0xFFFFFFFF, zlib.crc32(target) & 0xFFFFFFFF)
    out += struct.pack("<I", zlib.crc32(out) & 0xFFFFFFFF)
    return bytes(out)


def apply(source: bytes, patch: bytes) -> bytes:
    if patch[:4] != b"UPS1" or len(patch) < 16:
        raise UpsError("not a UPS patch")
    if zlib.crc32(patch[:-4]) & 0xFFFFFFFF != struct.unpack_from("<I", patch, len(patch) - 4)[0]:
        raise UpsError("the patch file is damaged (patch CRC mismatch)")
    src_crc, dst_crc = struct.unpack_from("<II", patch, len(patch) - 12)
    if zlib.crc32(source) & 0xFFFFFFFF != src_crc:
        raise UpsError("wrong base ROM (source CRC mismatch)")
    i = 4

    def read_varint() -> int:
        nonlocal i
        value, shift = 0, 1
        while True:
            b = patch[i]
            i += 1
            value += (b & 0x7F) * shift
            if b & 0x80:
                return value
            shift <<= 7
            value += shift

    src_size, dst_size = read_varint(), read_varint()
    if src_size != len(source):
        raise UpsError("wrong base ROM size")
    out = bytearray(source[:dst_size]) + bytearray(max(0, dst_size - len(source)))
    p = 0
    end = len(patch) - 12
    while i < end:
        p += read_varint()
        while True:
            x = patch[i]
            i += 1
            if x == 0:
                break
            if p < dst_size:
                out[p] ^= x
            p += 1
        p += 1
    if zlib.crc32(out) & 0xFFFFFFFF != dst_crc:
        raise UpsError("result does not match (target CRC mismatch)")
    return bytes(out)


def main() -> int:
    if len(sys.argv) != 5 or sys.argv[1] not in ("create", "apply"):
        print(__doc__)
        return 2
    a, b, c = (Path(x) for x in sys.argv[2:])
    try:
        if sys.argv[1] == "create":
            c.write_bytes(create(a.read_bytes(), b.read_bytes()))
        else:
            c.write_bytes(apply(a.read_bytes(), b.read_bytes()))
    except UpsError as e:
        print(f"error: {e}")
        return 1
    print(f"wrote {c}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
