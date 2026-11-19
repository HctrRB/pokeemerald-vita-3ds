#!/usr/bin/env python3
"""Checks the thread boundary between the game (Core 1) and the other cores.

The game (everything linked into game_combined.o) runs on Core 1, in AgbMain. Core 0
(presentation) and Core 2 (draw, chunk and building I/O workers) may only see the game
through what Core 1 publishes for them (docs/THREADS.md). This script proves it on the
linked ELF:

  * it walks every function reachable from each thread's entry point, decoding the direct
    branches (BL, BLX, B.W, tail calls) itself, since the assembler resolves calls to static
    functions without leaving a relocation;
  * it follows address-taken functions (a function's address in a register or a literal pool
    may be called) and function pointers stored in the port's own tables (how Mirage Island's
    layout check reached the party from Core 0);
  * it reports each call into the game's code and each access to the game's writable state
    (the gamedata / gamebss sections), with the path that leads to it.

Read-only game data (.rodata, script_data) is allowed. Exceptions are listed, each with its
reason, in allow.txt next to this script.

Usage: check_core_boundary.py [--elf build-vita/pokeemerald] [--game build-vita/game_combined.o]
                              [--allow tools/core_boundary/allow.txt] [--report FILE] [--strict]
--strict exits with status 1 when a violation is found (the build uses it).
"""

import argparse
import bisect
import collections
import fnmatch
import os
import struct
import sys

# ── ELF reading ──────────────────────────────────────────────────────────────

SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_REL = 9
STT_OBJECT = 1
STT_FUNC = 2
STT_SECTION = 3
STT_FILE = 4
STB_LOCAL = 0

R_ARM_ABS32 = 2
R_ARM_TARGET1 = 38
R_ARM_MOVW_ABS_NC = 43
R_ARM_MOVT_ABS = 44
R_ARM_THM_MOVW_ABS_NC = 47
R_ARM_THM_MOVT_ABS = 48


class Section:
    __slots__ = ("index", "name", "type", "flags", "addr", "offset", "size", "link", "info")


class Symbol:
    __slots__ = ("name", "value", "size", "type", "bind", "shndx", "file")


class Elf:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 1:
            raise SystemExit(f"{path}: not a 32-bit little-endian ELF")
        shoff, = struct.unpack_from("<I", d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 0x2E)
        self.sections = []
        for i in range(shnum):
            s = Section()
            (name, s.type, s.flags, s.addr, s.offset, s.size, s.link, s.info, _, _) = \
                struct.unpack_from("<IIIIIIIIII", d, shoff + i * shentsize)
            s.index = i
            s.name = name
            self.sections.append(s)
        strtab = self.sections[shstrndx]
        for s in self.sections:
            s.name = self._str(strtab.offset, s.name)
        self.by_name = {s.name: s for s in self.sections}
        self.symbols = self._read_symbols()

    def _str(self, base, off):
        end = self.data.index(b"\0", base + off)
        return self.data[base + off:end].decode("latin-1")

    def _read_symbols(self):
        out = []
        for s in self.sections:
            if s.type != SHT_SYMTAB:
                continue
            strtab = self.sections[s.link]
            current_file = None
            for (name, value, size, info, _, shndx) in struct.iter_unpack(
                    "<IIIBBH", self.data[s.offset:s.offset + s.size]):
                sym = Symbol()
                sym.name = self._str(strtab.offset, name) if name else ""
                sym.value, sym.size, sym.shndx = value, size, shndx
                sym.type, sym.bind = info & 0xF, info >> 4
                if sym.type == STT_FILE:
                    current_file = sym.name
                sym.file = current_file if sym.bind == STB_LOCAL else None
                out.append(sym)
        return out

    def relocations(self, target_section):
        """(offset, type, symbol index) of the REL section that applies to target_section."""
        for s in self.sections:
            if s.type == SHT_REL and s.info == target_section.index:
                for (off, info) in struct.iter_unpack("<II", self.data[s.offset:s.offset + s.size]):
                    yield off, info & 0xFF, info >> 8

    def section_at(self, addr):
        for s in self.sections:
            if s.addr and s.addr <= addr < s.addr + s.size and s.flags & 0x2:  # SHF_ALLOC
                return s
        return None


# ── The program's map ────────────────────────────────────────────────────────

class Function:
    __slots__ = ("start", "end", "name", "file", "thumb")

    def label(self):
        return f"{self.name} ({self.file})" if self.file else self.name


class DataObject:
    __slots__ = ("start", "end", "name", "file")

    def label(self):
        return f"{self.name} ({self.file})" if self.file else self.name


def _place(elf, obj, section_name, sym_type):
    """Address range of `obj`'s section `section_name` inside the linked `elf`, found through a
    global symbol both define."""
    sec = obj.by_name.get(section_name)
    if sec is None:
        return None
    local = {s.name: s.value for s in obj.symbols
             if s.bind != STB_LOCAL and s.shndx == sec.index and s.name
             and (sym_type is None or s.type == sym_type)}
    for s in elf.symbols:
        if s.bind != STB_LOCAL and s.name in local and s.shndx != 0:
            base = s.value - local[s.name]
            return (base, base + sec.size)
    return None


class Program:
    def __init__(self, elf_path, game_path, game_data_path):
        self.elf = Elf(elf_path)
        e = self.elf
        text = e.by_name[".text"]
        self.text = text
        self.text_bytes = memoryview(e.data)[text.offset:text.offset + text.size]

        # The game's code is game_combined.o's .text, linked as one block.
        game = Elf(game_path)
        game_text = game.by_name[".text"]
        game_funcs = {s.name: s.value for s in game.symbols
                      if s.type == STT_FUNC and s.bind != STB_LOCAL and s.shndx == game_text.index}
        base = None
        for s in e.symbols:
            if s.type == STT_FUNC and s.bind != STB_LOCAL and s.name in game_funcs:
                base = (s.value & ~1) - (game_funcs[s.name] & ~1)
                break
        if base is None:
            raise SystemExit("cannot place game_combined.o's .text inside the ELF")
        self.game_text = (base, base + game_text.size)

        # The game's read-only data: game_combined.o's .rodata, the ROM data (game_data.o,
        # moved to .data so it can hold pointers) and the scripts. Allowed, but the pointers in
        # it (tileset animation callbacks, sprite templates...) are the game's, never followed.
        self.game_rodata = [r for r in (
            _place(e, game, ".rodata", STT_OBJECT),
            _place(e, Elf(game_data_path), ".data", None) if os.path.exists(game_data_path) else None,
        ) if r is not None]
        if "script_data" in e.by_name:
            s = e.by_name["script_data"]
            self.game_rodata.append((s.addr, s.addr + s.size))

        # The game's writable state.
        self.game_state = [(s.addr, s.addr + s.size) for name in ("gamedata", "gamebss")
                           if (s := e.by_name.get(name)) is not None]
        stubs = [s for s in e.sections if s.name.startswith(".vitalink.fstubs")]
        self.stub_range = (min(s.addr for s in stubs), max(s.addr + s.size for s in stubs)) if stubs else (0, 0)

        # Functions and data objects, sorted by address.
        funcs = {}
        objects = {}
        mapping = []
        for s in e.symbols:
            if s.name[:2] in ("$t", "$a", "$d") and (len(s.name) == 2 or s.name[2] == "."):
                if s.shndx == text.index:
                    mapping.append((s.value, s.name[1]))
                continue
            if s.type == STT_FUNC and s.shndx == text.index and s.value:
                start = s.value & ~1
                if start not in funcs or (funcs[start].file is None and s.bind != STB_LOCAL):
                    f = Function()
                    f.start, f.end = start, start + max(s.size, 2)
                    f.name, f.file, f.thumb = s.name, s.file, bool(s.value & 1)
                    funcs[start] = f
            elif s.type == STT_OBJECT and s.size and s.value:
                if s.value not in objects or objects[s.value].end - objects[s.value].start < s.size:
                    o = DataObject()
                    o.start, o.end, o.name, o.file = s.value, s.value + s.size, s.name, s.file
                    objects[s.value] = o
        self.funcs = [funcs[k] for k in sorted(funcs)]
        self.func_starts = [f.start for f in self.funcs]
        self.objects = [objects[k] for k in sorted(objects)]
        self.object_starts = [o.start for o in self.objects]
        mapping.sort()
        self.map_addr = [m[0] for m in mapping]
        self.map_kind = [m[1] for m in mapping]
        self.by_name = collections.defaultdict(list)
        for f in self.funcs:
            self.by_name[f.name].append(f)

        self._symbols = e.symbols
        self._text_refs = None
        self._data_ptrs = None

    # lookups
    def func_at(self, addr):
        i = bisect.bisect_right(self.func_starts, addr) - 1
        if i >= 0 and self.funcs[i].start <= addr < self.funcs[i].end:
            return self.funcs[i]
        return None

    def object_at(self, addr):
        i = bisect.bisect_right(self.object_starts, addr) - 1
        if i >= 0 and self.objects[i].start <= addr < self.objects[i].end:
            return self.objects[i]
        return None

    def is_game_code(self, addr):
        return self.game_text[0] <= addr < self.game_text[1]

    def is_game_state(self, addr):
        return any(lo <= addr < hi for lo, hi in self.game_state)

    def is_game_rodata(self, addr):
        return any(lo <= addr < hi for lo, hi in self.game_rodata)

    def is_text(self, addr):
        return self.text.addr <= addr < self.text.addr + self.text.size

    def u16(self, addr):
        o = addr - self.text.addr
        return self.text_bytes[o] | self.text_bytes[o + 1] << 8

    def u32(self, addr):
        o = addr - self.text.addr
        return struct.unpack_from("<I", self.text_bytes, o)[0]

    def word(self, addr):
        s = self.elf.section_at(addr)
        if s is None or s.type == SHT_NOBITS:
            return None
        return struct.unpack_from("<I", self.elf.data, s.offset + addr - s.addr)[0]

    def kind_at(self, addr, default):
        i = bisect.bisect_right(self.map_addr, addr) - 1
        return self.map_kind[i] if i >= 0 else default

    # ── addresses the code takes (relocations in .text) ──
    def text_refs(self):
        """{function start: [(site, target address)]} for every absolute address in code."""
        if self._text_refs is not None:
            return self._text_refs
        refs = collections.defaultdict(list)
        pending_movw = {}  # (function, sym, register) -> (site, low half)
        for (off, rtype, _sym) in self.elf.relocations(self.text):
            f = self.func_at(off)
            if f is None:
                continue
            if rtype in (R_ARM_ABS32, R_ARM_TARGET1):
                refs[f.start].append((off, self.u32(off)))
            elif rtype in (R_ARM_THM_MOVW_ABS_NC, R_ARM_THM_MOVT_ABS):
                hw1, hw2 = self.u16(off), self.u16(off + 2)
                imm = (hw1 & 0xF) << 12 | ((hw1 >> 10) & 1) << 11 | ((hw2 >> 12) & 7) << 8 | (hw2 & 0xFF)
                reg = (hw2 >> 8) & 0xF
                self._pair(refs, pending_movw, f, _sym, reg, off, imm, rtype == R_ARM_THM_MOVT_ABS)
            elif rtype in (R_ARM_MOVW_ABS_NC, R_ARM_MOVT_ABS):
                w = self.u32(off)
                imm = ((w >> 16) & 0xF) << 12 | (w & 0xFFF)
                reg = (w >> 12) & 0xF
                self._pair(refs, pending_movw, f, _sym, reg, off, imm, rtype == R_ARM_MOVT_ABS)
        # A MOVW without its MOVT: complete it with the symbol's high half.
        for (fstart, sym, _reg), (site, low) in pending_movw.items():
            base = self._symbols[sym].value
            refs[fstart].append((site, (base & 0xFFFF0000) | low))
        self._text_refs = refs
        return refs

    @staticmethod
    def _pair(refs, pending, f, sym, reg, site, imm, is_movt):
        key = (f.start, sym, reg)
        if not is_movt:
            pending[key] = (site, imm)
        elif key in pending:
            msite, low = pending.pop(key)
            refs[f.start].append((msite, imm << 16 | low))

    # ── pointers stored in data ──
    def data_ptrs(self):
        """{data address: pointer value} for every absolute address stored in data."""
        if self._data_ptrs is not None:
            return self._data_ptrs
        ptrs = {}
        for s in self.elf.sections:
            if s.type != SHT_REL:
                continue
            target = self.elf.sections[s.info]
            if target is self.text or not target.flags & 0x2 or target.flags & 0x4:  # alloc, not exec
                continue
            if target.type == SHT_NOBITS:
                continue
            for (off, rtype, _sym) in self.elf.relocations(target):
                if rtype in (R_ARM_ABS32, R_ARM_TARGET1):
                    ptrs[off] = struct.unpack_from("<I", self.elf.data, target.offset + off - target.addr)[0]
        self._data_ptrs = ptrs
        self._ptr_addrs = sorted(ptrs)
        return ptrs

    def pointers_in(self, lo, hi):
        ptrs = self.data_ptrs()
        i = bisect.bisect_left(self._ptr_addrs, lo)
        while i < len(self._ptr_addrs) and self._ptr_addrs[i] < hi:
            yield self._ptr_addrs[i], ptrs[self._ptr_addrs[i]]
            i += 1

    # ── direct branches ──
    def branches(self, f):
        """(site, target, is_call) for each direct branch out of f."""
        out = []
        pc = f.start
        default = "t" if f.thumb else "a"
        while pc < f.end:
            kind = self.kind_at(pc, default)
            nxt = self._next_mapping(pc, f.end)
            if kind == "d":
                pc = nxt
                continue
            if kind == "t":
                while pc < nxt and pc + 2 <= f.end:
                    hw1 = self.u16(pc)
                    top = hw1 >> 11
                    if top in (0b11101, 0b11110, 0b11111):
                        if pc + 4 > f.end:
                            break
                        hw2 = self.u16(pc + 2)
                        if top == 0b11110 and hw2 & 0x8000:
                            r = self._thumb32_branch(pc, hw1, hw2)
                            if r:
                                out.append((pc,) + r)
                        pc += 4
                    else:
                        if top == 0b11100:  # B T2 (16-bit): may be a tail call to a close function
                            imm = (hw1 & 0x7FF) << 1
                            if imm & 0x800:
                                imm -= 0x1000
                            out.append((pc, pc + 4 + imm, False))
                        pc += 2
            else:
                pc = (pc + 3) & ~3
                while pc < nxt and pc + 4 <= f.end:
                    w = self.u32(pc)
                    if (w >> 25) & 7 == 0b101:
                        imm = (w & 0xFFFFFF) << 2
                        if imm & 0x2000000:
                            imm -= 0x4000000
                        if w >> 28 == 0xF:  # BLX imm
                            out.append((pc, pc + 8 + imm + (((w >> 24) & 1) << 1), True))
                        elif w >> 28 != 0xF:
                            out.append((pc, pc + 8 + imm, bool((w >> 24) & 1)))
                    pc += 4
            pc = max(pc, nxt)
        return out

    def _next_mapping(self, pc, end):
        i = bisect.bisect_right(self.map_addr, pc)
        return min(self.map_addr[i], end) if i < len(self.map_addr) else end

    @staticmethod
    def _thumb32_branch(pc, hw1, hw2):
        s = (hw1 >> 10) & 1
        j1 = (hw2 >> 13) & 1
        j2 = (hw2 >> 11) & 1
        if hw2 & 0x1000:  # BL (bit 14 set) or B.W T4
            i1, i2 = 1 - (j1 ^ s), 1 - (j2 ^ s)
            imm = s << 24 | i1 << 23 | i2 << 22 | (hw1 & 0x3FF) << 12 | (hw2 & 0x7FF) << 1
            if s:
                imm -= 1 << 25
            return pc + 4 + imm, bool(hw2 & 0x4000)
        if hw2 & 0x4000:  # BLX imm (to ARM code)
            i1, i2 = 1 - (j1 ^ s), 1 - (j2 ^ s)
            imm = s << 24 | i1 << 23 | i2 << 22 | (hw1 & 0x3FF) << 12 | ((hw2 >> 1) & 0x3FF) << 2
            if s:
                imm -= 1 << 25
            return ((pc + 4) & ~3) + imm, True
        cond = (hw1 >> 6) & 0xF
        if cond >= 14:
            return None
        imm = s << 20 | j2 << 19 | j1 << 18 | (hw1 & 0x3F) << 12 | (hw2 & 0x7FF) << 1
        if s:
            imm -= 1 << 21
        return pc + 4 + imm, False


# ── Exceptions ───────────────────────────────────────────────────────────────

class Allow:
    """allow.txt: one rule per line, '#' starts a comment.

    call  <game function>             any Core 0 / Core 2 function may call it
    call  <caller>:<game function>    only that caller may
    data  <game variable>             any may read or write it
    data  <caller>:<game variable>    only that caller may
    table <port data object>          its function pointers are not called from these cores
    stop  <port function>             not run by these cores (or only while Core 1 is parked)
    Names accept shell wildcards (*, ?).
    """

    def __init__(self, path):
        self.rules = collections.defaultdict(list)
        self.used = set()
        if not path or not os.path.exists(path):
            return
        with open(path, encoding="utf-8") as f:
            for n, line in enumerate(f, 1):
                line = line.split("#", 1)[0].strip()
                if not line:
                    continue
                parts = line.split()
                if len(parts) != 2 or parts[0] not in ("call", "data", "table", "stop"):
                    raise SystemExit(f"{path}:{n}: cannot read '{line}'")
                kind, what = parts
                caller, _, target = what.rpartition(":")
                self.rules[kind].append((caller or "*", target, f"{kind} {what}"))

    def match(self, kind, caller, target):
        for (c, t, rule) in self.rules[kind]:
            if fnmatch.fnmatchcase(target, t) and fnmatch.fnmatchcase(caller, c):
                self.used.add(rule)
                return True
        return False

    def unused(self):
        return [r for rules in self.rules.values() for (_, _, r) in rules if r not in self.used]


# ── The walk ─────────────────────────────────────────────────────────────────

THREADS = {
    # thread label: entry functions
    "Core 0 (MainThread)": ["MainThread_Run"],
    "Core 2 (DrawWorker)": ["DrawWorkerMain"],
    "Core 2 (VoxelChunkWorker)": ["VoxelChunkWorkerMain"],
    "Core 2 (VoxelIoWorker)": ["VoxelBuildingIoWorkerMain"],
}
# Entry points of other threads: taking their address does not run them on this thread.
THREAD_ENTRIES = {"main", "DoMain", "AgbMain", "DrawWorkerMain", "VoxelChunkWorkerMain", "VoxelBuildingIoWorkerMain"}


def walk(prog, allow, label, roots):
    violations = collections.OrderedDict()  # (kind, target label) -> list of (from function, site, path)
    parent = {}
    queue = collections.deque()
    for name in roots:
        for f in prog.by_name.get(name, []):
            parent[f.start] = None
            queue.append(f)
    if not queue:
        raise SystemExit(f"{label}: entry point {roots} not found")
    refs = prog.text_refs()
    seen_tables = set()

    def path_of(fstart):
        out = []
        while fstart is not None:
            f = prog.func_at(fstart)
            p = parent.get(fstart)
            out.append(f.name if f else hex(fstart))
            fstart = p[0] if p else None
        return " <- ".join(out)

    def report(kind, target_label, f, site, how=""):
        key = (kind, target_label)
        violations.setdefault(key, []).append((f.label(), site, how, path_of(f.start)))

    def visit(f, target, site, via):
        """f reaches the function at `target` (a call, a taken address, a table)."""
        if prog.stub_range[0] <= target < prog.stub_range[1]:
            return
        if prog.is_game_code(target):
            g = prog.func_at(target)
            gname = g.name if g else hex(target)
            if not allow.match("call", f.name, gname):
                report("call", g.label() if g else gname, f, site, via)
            return
        g = prog.func_at(target)
        if g is None or g.start == f.start and via == "":
            return
        if g.start in parent:
            return
        if g.name in THREAD_ENTRIES and via != "":
            return
        if allow.match("stop", f.name, g.name):
            return
        parent[g.start] = (f.start, via)
        queue.append(g)

    def visit_table(f, obj, site, depth=0):
        if allow.match("table", f.name, obj.name):
            return
        if (obj.start, f.start) in seen_tables or depth > 4:
            return
        seen_tables.add((obj.start, f.start))
        for (where, value) in prog.pointers_in(obj.start, obj.end):
            if prog.is_text(value & ~1):
                visit(f, value & ~1, site, f"via table {obj.name}")
            elif not prog.is_game_state(value) and not prog.is_game_code(value):
                inner = prog.object_at(value)
                if inner is not None and inner is not obj and not _is_game_data(prog, inner):
                    visit_table(f, inner, site, depth + 1)

    while queue:
        f = queue.popleft()
        for (site, target, _is_call) in prog.branches(f):
            if f.start <= target < f.end:
                continue
            visit(f, target, site, "")
        for (site, value) in refs.get(f.start, ()):
            if prog.is_game_state(value):
                obj = prog.object_at(value)
                name = obj.name if obj else hex(value)
                if not allow.match("data", f.name, name):
                    report("data", obj.label() if obj else name, f, site)
            elif prog.is_text(value & ~1):
                visit(f, value & ~1, site, "address taken")
            else:
                obj = prog.object_at(value)
                if obj is None:
                    continue
                if not _is_game_data(prog, obj):
                    visit_table(f, obj, site)
                elif prog.is_game_rodata(obj.start) and obj.end - obj.start <= 64:
                    # A constant pointer to the game's state (gWeatherPtr = &gWeather): reading
                    # through it is reading the state.
                    for (_where, ptr) in prog.pointers_in(obj.start, obj.end):
                        if prog.is_game_state(ptr):
                            inner = prog.object_at(ptr)
                            name = inner.name if inner else hex(ptr)
                            if not allow.match("data", f.name, name):
                                report("data", f"{inner.label() if inner else name} (through {obj.name})", f, site)
    return violations, len(parent)


# The game thread's side of the contract: it captures the game frame, it never reads it (the
# slot GameFrame_Get returns is the one Core 0 pinned). Its own code and the game's are walked
# alike, function pointer tables included.
GAME_THREAD = ("Core 1 (AgbMain)", ["DoMain"])
CORE0_ONLY = {"GameFrame_Pin", "GameFrame_Unpin", "GameFrame_MapSettled"}
# GameFrame_Get is inline: what it reads is Core 0's pin.
CORE0_ONLY_DATA = {"gGameFrameCore0"}


def walk_game_thread(prog, allow, roots):
    violations = collections.OrderedDict()
    parent = {}
    queue = collections.deque()
    for name in roots:
        for f in prog.by_name.get(name, []):
            parent[f.start] = None
            queue.append(f)
    refs = prog.text_refs()
    seen_objects = set()

    def path_of(fstart):
        out = []
        while fstart is not None and len(out) < 12:
            f = prog.func_at(fstart)
            out.append(f.name if f else hex(fstart))
            p = parent.get(fstart)
            fstart = p if p else None
        return " <- ".join(out)

    def visit(f, target, via):
        g = prog.func_at(target)
        if g is None or g.start in parent or g.start == f.start:
            return
        if g.name in THREAD_ENTRIES - {"DoMain", "AgbMain"}:
            return
        if g.name in CORE0_ONLY:
            if not allow.match("call", f.name, g.name):
                violations.setdefault(("call", g.name), []).append((f.label(), via, path_of(f.start)))
            return
        if allow.match("stop", f.name, g.name):
            return
        parent[g.start] = f.start
        queue.append(g)

    def visit_object(f, obj, depth=0):
        if obj.start in seen_objects or depth > 4 or prog.is_game_state(obj.start):
            return
        if allow.match("table", f.name, obj.name):
            return
        seen_objects.add(obj.start)
        for (_where, value) in prog.pointers_in(obj.start, obj.end):
            if prog.is_text(value & ~1):
                visit(f, value & ~1, f"via table {obj.name}")
            else:
                inner = prog.object_at(value)
                if inner is not None and inner is not obj:
                    visit_object(f, inner, depth + 1)

    while queue:
        f = queue.popleft()
        for (_site, target, _is_call) in prog.branches(f):
            if not (f.start <= target < f.end) and not (prog.stub_range[0] <= target < prog.stub_range[1]):
                visit(f, target, "")
        for (_site, value) in refs.get(f.start, ()):
            if prog.is_text(value & ~1):
                visit(f, value & ~1, "address taken")
            else:
                obj = prog.object_at(value)
                if obj is None:
                    continue
                if obj.name in CORE0_ONLY_DATA:
                    if not allow.match("data", f.name, obj.name):
                        violations.setdefault(("data", obj.name), []).append((f.label(), "", path_of(f.start)))
                    continue
                visit_object(f, obj)
    return violations, len(parent)


def _is_game_data(prog, obj):
    """Objects that belong to the game: its state is reported, its read-only data allowed, and
    the pointers in either are never followed."""
    return prog.is_game_state(obj.start) or prog.is_game_rodata(obj.start)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.normpath(os.path.join(here, "..", ".."))
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=os.path.join(repo, "build-vita", "pokeemerald"))
    ap.add_argument("--game", default=os.path.join(repo, "build-vita", "game_combined.o"))
    ap.add_argument("--game-data", default=os.path.join(repo, "build-vita", "generated", "game_data.o"))
    ap.add_argument("--allow", default=os.path.join(here, "allow.txt"))
    ap.add_argument("--report", default=os.path.join(repo, "build-vita", "core_boundary_report.txt"))
    ap.add_argument("--strict", action="store_true", help="exit 1 when a violation is found")
    args = ap.parse_args()

    prog = Program(args.elf, args.game, args.game_data)
    allow = Allow(args.allow)
    lines = []
    total = 0
    for label, roots in THREADS.items():
        violations, reached = walk(prog, allow, label, roots)
        total += len(violations)
        lines.append(f"== {label}: {reached} functions reached, {len(violations)} violations")
        for (kind, target), hits in violations.items():
            verb = "calls game code" if kind == "call" else "touches game state"
            lines.append(f"  {verb}: {target}")
            for (frm, site, how, path) in hits[:3]:
                extra = f" [{how}]" if how else ""
                lines.append(f"      from {frm} @ {site:#x}{extra}")
                lines.append(f"        path: {path}")
            if len(hits) > 3:
                lines.append(f"      ... and {len(hits) - 3} more sites")
        lines.append("")
    label, roots = GAME_THREAD
    violations, reached = walk_game_thread(prog, allow, roots)
    total += len(violations)
    lines.append(f"== {label}: {reached} functions reached, {len(violations)} violations")
    for (_kind, target), hits in violations.items():
        lines.append(f"  reads the game frame (Core 0's): {target}")
        for (frm, how, path) in hits[:3]:
            extra = f" [{how}]" if how else ""
            lines.append(f"      from {frm}{extra}")
            lines.append(f"        path: {path}")
        if len(hits) > 3:
            lines.append(f"      ... and {len(hits) - 3} more sites")
    lines.append("")
    unused = allow.unused()
    if unused:
        lines.append("Unused allow.txt rules (remove them):")
        lines.extend(f"  {r}" for r in unused)
    text = "\n".join(lines) + "\n"
    with open(args.report, "w", encoding="utf-8") as f:
        f.write(text)
    summary = [l for l in lines if l.startswith("==")]
    print("\n".join(summary))
    print(f"Core boundary: {total} violations (report: {args.report})")
    if args.strict and total:
        sys.exit(1)


if __name__ == "__main__":
    main()
