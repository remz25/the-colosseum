"""Render the GBA screen (tile-mode BG layers + regular sprites) from memory read over the GDB stub."""
import struct, zlib


def grab(r):
    rd = lambda a, n: b"".join(r.read(a + k, min(0x80, n - k)) for k in range(0, n, 0x80))
    return {"io": rd(0x04000000, 0x60), "pal": rd(0x05000000, 0x400), "vram": rd(0x06000000, 0x18000),
            "oam": rd(0x07000000, 0x400)}


def rgb(c):
    return ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)


def render(m, path, scroll=None):
    io, pal, vram, oam = m["io"], m["pal"], m["vram"], m["oam"]
    dispcnt = struct.unpack_from("<H", io, 0)[0]
    W, H = 240, 160
    back = rgb(struct.unpack_from("<H", pal, 0)[0])
    img = [[back] * W for _ in range(H)]
    prio = [[9] * W for _ in range(H)]
    mode = dispcnt & 7
    layers = []
    for bg in range(4):
        if not (dispcnt >> (8 + bg)) & 1 or mode != 0:
            continue
        cnt = struct.unpack_from("<H", io, 8 + 2 * bg)[0]
        layers.append((cnt & 3, bg, cnt))
    for p, bg, cnt in sorted(layers, reverse=True):
        cbase = ((cnt >> 2) & 3) * 0x4000
        sbase = ((cnt >> 8) & 31) * 0x800
        c256 = (cnt >> 7) & 1
        size = cnt >> 14
        sw, sh = (256, 256) if size == 0 else (512, 256) if size == 1 else (256, 512) if size == 2 else (512, 512)
        hofs, vofs = (scroll or {}).get(bg, (0, 0))
        for y in range(H):
            for x in range(W):
                X, Y = (x + hofs) % sw, (y + vofs) % sh
                blk = (X // 256) + (Y // 256) * (sw // 256)
                e = struct.unpack_from("<H", vram, sbase + blk * 0x800 + ((Y % 256) // 8) * 64 + ((X % 256) // 8) * 2)[0]
                t, hf, vf, pb = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1, e >> 12
                px, py = X % 8, Y % 8
                if hf: px = 7 - px
                if vf: py = 7 - py
                if c256:
                    ci = vram[cbase + t * 64 + py * 8 + px] if cbase + t * 64 + 64 <= 0x10000 else 0
                    col = ci
                else:
                    off = cbase + t * 32 + py * 4 + px // 2
                    if off >= 0x10000:
                        continue
                    b = vram[off]
                    ci = (b >> 4) if px & 1 else (b & 15)
                    col = pb * 16 + ci
                if ci and p <= prio[y][x]:
                    img[y][x] = rgb(struct.unpack_from("<H", pal, col * 2)[0])
                    prio[y][x] = p
    if dispcnt & 0x1000:
        shapes = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64), (1, 0): (16, 8), (1, 1): (32, 8),
                  (1, 2): (32, 16), (1, 3): (64, 32), (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}
        oned = dispcnt & 0x40
        for i in range(127, -1, -1):
            a0, a1, a2 = struct.unpack_from("<HHH", oam, i * 8)
            if (a0 >> 8) & 3 == 2 or (a0 >> 8) & 1:
                continue
            shape, sz = a0 >> 14, a1 >> 14
            if (shape, sz) not in shapes:
                continue
            w, h = shapes[(shape, sz)]
            y0, x0 = a0 & 255, a1 & 511
            if y0 >= 160: y0 -= 256
            if x0 >= 240: x0 -= 512
            tile, p, pb = a2 & 1023, (a2 >> 10) & 3, a2 >> 12
            hf, vf = (a1 >> 12) & 1, (a1 >> 13) & 1
            for yy in range(h):
                for xx in range(w):
                    sx, sy = x0 + xx, y0 + yy
                    if not (0 <= sx < W and 0 <= sy < H):
                        continue
                    tx, ty = (w - 1 - xx if hf else xx), (h - 1 - yy if vf else yy)
                    tn = tile + (ty // 8) * (w // 8 if oned else 32) + tx // 8
                    off = 0x10000 + (tn & 1023) * 32 + (ty % 8) * 4 + (tx % 8) // 2
                    if off >= 0x18000:
                        continue
                    b = vram[off]
                    ci = (b >> 4) if tx & 1 else (b & 15)
                    if ci and p <= prio[sy][sx]:
                        img[sy][sx] = rgb(struct.unpack_from("<H", pal, 0x200 + (pb * 16 + ci) * 2)[0])
    if dispcnt & 0x80:
        img = [[(255, 255, 255)] * W for _ in range(H)]
    S = 2
    raw = b"".join(b"\0" + b"".join(bytes(img[y // S][x // S]) for x in range(W * S)) for y in range(H * S))
    ch = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + ch(b"IHDR", struct.pack(">IIBBBBB", W * S, H * S, 8, 2, 0, 0, 0))
                           + ch(b"IDAT", zlib.compress(raw)) + ch(b"IEND", b""))
    return dispcnt
