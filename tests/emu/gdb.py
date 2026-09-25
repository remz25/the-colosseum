"""Minimal GDB remote-serial-protocol client for mGBA's debugger (mGBA.exe --gdb <rom>).

mGBA exits when the debugger disconnects, so keep one connection for a whole session.
Used by tests/run_tests.py and other emulator tests. Test copies of ROMs/saves only."""
import os
import socket
import struct
import subprocess
import time

MGBA = os.environ.get("MGBA", r"C:\Program Files\mGBA\mGBA.exe")
KEY_READ_BP = 0x080013FC      # FE8U key read routine; r1 = pressed keys (main loop, once a frame)
KEYS = {"A": 1, "B": 2, "SELECT": 4, "START": 8, "RIGHT": 16, "LEFT": 32, "UP": 64, "DOWN": 128,
        "R": 256, "L": 512}
REG_PC, REG_LR, REG_SP, REG_CPSR = 15, 14, 13, 25


class Gdb:
    def __init__(self, port=2345, attempts=60):
        last = None
        for _ in range(attempts):
            try:
                self.s = socket.create_connection(("127.0.0.1", port), timeout=5)
                break
            except OSError as e:
                last = e
                time.sleep(0.25)
        else:
            raise RuntimeError(f"could not connect to mGBA's GDB server: {last}")
        self.s.settimeout(10)
        self.buf = b""
        self.cmd("?")

    def _packet(self):
        while True:
            while b"#" not in self.buf or len(self.buf) < self.buf.index(b"#") + 3:
                self.buf += self.s.recv(4096)
            start = self.buf.find(b"$")
            end = self.buf.index(b"#", start)
            data = self.buf[start + 1:end]
            self.buf = self.buf[end + 3:]
            self.s.sendall(b"+")
            out, i = bytearray(), 0
            while i < len(data):                 # run-length encoding: X*n repeats X (n - 29) more times
                if data[i:i + 1] == b"*" and out:
                    out += out[-1:] * (data[i + 1] - 29)
                    i += 2
                else:
                    out.append(data[i])
                    i += 1
            return out.decode("latin1")

    def cmd(self, data):
        pkt = data.encode("latin1")
        self.s.sendall(b"$" + pkt + b"#%02x" % (sum(pkt) & 0xFF))
        while not self.buf.startswith(b"+"):
            self.buf += self.s.recv(4096)
            if self.buf.startswith(b"-"):
                self.buf = self.buf[1:]
        self.buf = self.buf[1:]
        return self._packet()

    def read(self, addr, n):
        out = b""
        while n > 0:                              # small chunks: large replies are unreliable
            k = min(n, 0x40)
            out += bytes.fromhex(self.cmd(f"m{addr:x},{k:x}"))
            addr, n = addr + k, n - k
        return out

    def write(self, addr, data: bytes):
        return self.cmd(f"M{addr:x},{len(data):x}:{data.hex()}")

    def reg(self, i):
        return struct.unpack("<I", bytes.fromhex(self.cmd(f"p{i:x}")))[0]

    def setreg(self, i, v):
        return self.cmd(f"P{i:x}={struct.pack('<I', v & 0xFFFFFFFF).hex()}")

    def breakpoint(self, addr, on=True):
        return self.cmd(f"{'Z' if on else 'z'}0,{addr:x},2")

    def cont(self, timeout=10):
        self.s.settimeout(timeout)
        try:
            return self.cmd("c")
        finally:
            self.s.settimeout(10)

    def frames(self, n, key=None):
        """Run n frames (stops at the key-read breakpoint each frame), holding `key` if given."""
        self.breakpoint(KEY_READ_BP)
        for _ in range(n):
            self.cont()
            if self.reg(REG_PC) != KEY_READ_BP:
                raise RuntimeError(f"stopped at {self.reg(REG_PC):#010x}, not the key read")
            if key:
                self.setreg(1, KEYS[key] if isinstance(key, str) else key)

    def call(self, func, *args, trap=0x080000C0, timeout=30):
        """Call a Thumb function from the current stop (which must be in Thumb code, e.g. the key
        read) and return r0. The return lands on `trap` (ROM header, never executed) where a
        breakpoint catches it; all registers are then restored."""
        saved = {i: self.reg(i) for i in list(range(16)) + [REG_CPSR]}
        for i, a in enumerate(args):
            self.setreg(i, a)
        self.setreg(REG_LR, trap | 1)
        self.setreg(REG_PC, func & ~1)
        self.breakpoint(trap)
        try:
            self.cont(timeout)
            pc = self.reg(REG_PC)
            if pc != trap:
                raise RuntimeError(f"call to {func:#x} stopped at {pc:#010x}")
            result = self.reg(0)
        finally:
            self.breakpoint(trap, False)
            for i, v in saved.items():
                self.setreg(i, v)
        return result


def start_mgba(rom, save=None, port=2345):
    """Start a separate mGBA on `rom` (a scratch copy) with its GDB server; returns the process."""
    if save:
        import shutil
        shutil.copyfile(save, os.path.splitext(rom)[0] + ".sav")
    return subprocess.Popen([MGBA, "--gdb", rom])


def read_sym(path):
    """ColorzCore --nocash-sym output: 'AAAAAAAA Label' lines -> {label: address}."""
    syms = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                try:
                    syms[parts[1]] = int(parts[0], 16)
                except ValueError:
                    pass
    return syms
