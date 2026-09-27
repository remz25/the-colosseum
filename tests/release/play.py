"""Release test + screenshots on the PATCHED ROM (clean FE8 + UPS). Persistent driver pieces."""
import sys, os, shutil, struct
sys.path.insert(0, r"C:\Users\RdotS\Downloads\Colosseum\tests")
sys.path.insert(0, r"C:\Users\RdotS\Downloads\Colosseum\tests\emu")
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import run_tests as rt
from gdb import Gdb, start_mgba
import screen

COL, PLAYST = 0x0203F600, 0x0202BCF0
BLUE, RED = 0x0202BE4C, 0x0202CFBC
OUT = os.path.join(HERE, "shots")
os.makedirs(OUT, exist_ok=True)


def boot(rom="patched.gba", save=None):
    for f in os.listdir(HERE):
        if f.endswith(".sav") and f != "keep.sav":
            os.remove(os.path.join(HERE, f))
    work = os.path.join(HERE, "run.gba")
    shutil.copyfile(os.path.join(HERE, rom), work)
    proc = start_mgba(work, save=os.path.join(HERE, save) if save else None)
    return proc, Gdb()


def shot(g, name, scale=2):
    screen.render(screen.grab(g), os.path.join(OUT, name + ".png"))
    if scale > 1:
        upscale(os.path.join(OUT, name + ".png"), scale)


def upscale(path, k):
    import zlib
    data = open(path, "rb").read()
    w, h = struct.unpack(">II", data[16:24])
    idat = b""
    p = 8
    while p < len(data):
        n, t = struct.unpack(">I4s", data[p:p + 8])
        if t == b"IDAT":
            idat += data[p + 8:p + 8 + n]
        p += 12 + n
    raw = zlib.decompress(idat)
    rows = [raw[y * (w * 3 + 1) + 1:(y + 1) * (w * 3 + 1)] for y in range(h)]
    out = []
    for r in rows:
        rr = bytearray()
        for x in range(w):
            rr += r[x * 3:x * 3 + 3] * k
        out += [bytes(rr)] * k
    W, H = w * k, h * k
    raw2 = b"".join(b"\0" + r for r in out)

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw2, 9)) + chunk(b"IEND", b""))


def press(g, key, after=30, hold=4):
    g.frames(hold, key)
    g.frames(after)


def units(g):
    r = []
    for base in (BLUE, RED):
        for i in range(5):
            u = g.read(base + 0x48 * i, 0x14)
            if u[0:4] != bytes(4):
                r.append((hex(base + 0x48 * i), u[0x10], u[0x11], u[0x13], u[0x12], hex(int.from_bytes(u[0x0C:0x10], "little"))))
    return r
