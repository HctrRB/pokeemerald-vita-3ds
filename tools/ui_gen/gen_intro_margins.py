#!/usr/bin/env python3
"""Procedural panoramic 16:9 continuation margins for the intro leaves scene (EM3DLVS1).

Reads graphics/intro/scene_1 from the decomp workspace and generates continuous
widescreen margin palette-index strips for all four parallax layers.
"""

from __future__ import annotations

import argparse
import math
import random
import struct
from pathlib import Path
from typing import Callable

ROOT = Path(__file__).resolve().parents[2]
SCENE = ROOT / "graphics" / "intro" / "scene_1"
MAGIC = b"EM3DLVS1"
W = 56
H = 256
PATCH = 8

TILES: bytes | None = None
MAPS: list[list[int]] | None = None
PICS: dict[int, list[list[tuple[int, int] | None]]] = {}


def load_scene() -> tuple[bytes, list[list[int]]]:
    tiles = (SCENE / "bg.4bpp").read_bytes()
    maps: list[list[int]] = []
    for i in range(4):
        d = (SCENE / f"bg{i}_map.bin").read_bytes()
        e = list(struct.unpack(f"<{len(d) // 2}H", d))
        maps.append(e + [0] * (32 * 32 - len(e)))
    return tiles, maps


def pic(L: int) -> list[list[tuple[int, int] | None]]:
    global TILES, MAPS
    if TILES is None or MAPS is None:
        TILES, MAPS = load_scene()
    if L not in PICS:
        out: list[list[tuple[int, int] | None]] = [[None] * 240 for _ in range(H)]
        for r in range(32):
            for c in range(30):
                e = MAPS[L][r * 32 + c]
                base = (e & 1023) * 32
                bank = e >> 12
                for y in range(8):
                    for x in range(8):
                        b = TILES[base + y * 4 + x // 2] if base + 32 <= len(TILES) else 0
                        v = (b >> ((x & 1) * 4)) & 15
                        sx = 7 - x if e & 0x400 else x
                        sy = 7 - y if e & 0x800 else y
                        if v:
                            out[r * 8 + sy][c * 8 + sx] = (bank, v)
        PICS[L] = out
    return PICS[L]


class Canvas:
    def __init__(self) -> None:
        self.p: list[list[tuple[int, int] | None]] = [[None] * W for _ in range(H)]

    def set(self, x: int, y: int, c: tuple[int, int] | None) -> None:
        if 0 <= x < W and 0 <= y < H:
            self.p[y][x] = c

    def get(self, x: int, y: int) -> tuple[int, int] | None:
        if 0 <= x < W and 0 <= y < H:
            return self.p[y][x]
        return None


class Margins:
    def __init__(self, L: int) -> None:
        self.L = L
        self.c = {"left": Canvas(), "right": Canvas()}
        self.sky: tuple[int, int] | None = None
        self.patch: dict[tuple[int, int], tuple[int, int] | None] = {}
        self.patchable: Callable[[int, int], bool] = lambda x, y: False

    def put(self, x: int, y: int, color: tuple[int, int] | None) -> None:
        if not (0 <= y < H):
            return
        if 0 <= x < 240:
            if self.patchable(x, y):
                self.patch[(x, y)] = color
            return
        if x < 0:
            self.c["left"].set(-1 - x, y, color)
        else:
            self.c["right"].set(x - 240, y, color)

    def xs(self, side: str) -> range:
        return range(-W, 0) if side == "left" else range(240, 240 + W)


def row_period(row: list[tuple[int, int] | None], side: str, span: int = 32) -> int | None:
    xs = range(0, span) if side == "left" else range(240 - span, 240)
    vals = [row[x] for x in xs]
    for p in (1, 2, 4, 8, 16):
        if all(vals[i] == vals[i + p] for i in range(len(vals) - p)):
            return p
    return None


def continue_texture(m: Margins, y0: int, y1: int, sides: tuple[str, ...] = ("left", "right"), span: int = 32) -> None:
    P = pic(m.L)
    for side in sides:
        for y in range(y0, y1):
            p = row_period(P[y], side, span) or 2
            for x in m.xs(side):
                src = 240 - p + ((x - 240) % p) if side == "right" else (x % p + p) % p
                m.put(x, y, P[y][src])


def spans_at(
    P: list[list[tuple[int, int] | None]],
    x: int,
    y0: int,
    y1: int,
    gap: Callable[[int, tuple[int, int]], bool],
) -> list[tuple[int, int]]:
    out: list[tuple[int, int]] = []
    start: int | None = None
    for y in range(y0, y1 + 1):
        c = P[y][x] if y < y1 else None
        solid = c is not None and not gap(y, c)
        if solid and start is None:
            start = y
        elif not solid and start is not None:
            out.append((start, y - 1))
            start = None
    return out


def continue_edge(
    m: Margins,
    side: str,
    y0: int = 0,
    y1: int = 256,
    gap: Callable[[int, tuple[int, int]], bool] = lambda y, c: False,
    long_span: int = 48,
    look: int = 5,
    reach: float | None = None,
    edge: int | None = None,
) -> None:
    P = pic(m.L)
    ex = edge if edge is not None else (0 if side == "left" else 239)
    ix = ex + look if side == "left" else ex - look
    out = -1 if side == "left" else 1
    inner = spans_at(P, ix, y0, y1, gap)
    for top0, bot0 in spans_at(P, ex, y0, y1, gap):
        h = bot0 - top0 + 1
        mid = (top0 + bot0) / 2.0
        best: tuple[float, int, int] | None = None
        for a, b in inner:
            if a <= bot0 and b >= top0:
                d_val = abs((a + b) / 2.0 - mid)
                if best is None or d_val < best[0]:
                    best = (d_val, a, b)
        st = sb = 0.0
        if best:
            st = max(-1.2, min(1.2, (top0 - best[1]) / look))
            sb = max(-1.2, min(1.2, (bot0 - best[2]) / look))
        L = reach if reach is not None else max(3.0, min(40.0, h * 1.3))
        if h >= long_span:
            L = 2.0
        d_step = 0
        while d_step < L:
            t = (d_step / L) ** 2
            ct = top0 + st * d_step
            cb = bot0 + sb * d_step
            cm = (ct + cb) / 2.0
            top = ct + (cm - ct) * t
            bot = cb + (cm - cb) * t
            if bot < top:
                break
            yt, yb = int(round(top)), int(round(bot))
            for y in range(yt, yb + 1):
                src = top0 + (y - yt) * (bot0 - top0) / (yb - yt) if yb > yt else (top0 + bot0) / 2.0
                c = P[int(round(src))][ex]
                if c is not None and not gap(int(round(src)), c):
                    m.put(ex + out * (d_step + 1), y, c)
            d_step += 1


def mountain(
    m: Margins,
    profile: Callable[[float], float],
    x0: int,
    x1: int,
    base: int,
    body: int = 9,
    lit: int = 11,
    rim: int = 10,
    lit_side: str = "right",
    lit_depth: int = 9,
) -> None:
    for x in range(x0, x1):
        top = int(round(profile(x)))
        if top >= base:
            continue
        slope = profile(x + 1) - profile(x - 1)
        facing = (slope > 0) if lit_side == "right" else (slope < 0)
        for y in range(top, base):
            c = (5, body)
            d = y - top
            if facing and abs(slope) > 0.3:
                if d < 1:
                    c = (5, rim)
                elif d < lit_depth - 2:
                    c = (5, lit)
                elif d < lit_depth and (x + y) & 1:
                    c = (5, lit)
            m.put(x, y, c)


def cloud(m: Margins, cx: int, cy: int, w: int, h: int, color: int = 11, shade: int = 10) -> None:
    for y in range(cy - h, cy + h + 1):
        for x in range(cx - w, cx + w + 1):
            dx = (x - cx) / w
            dy = (y - cy) / h
            bump = 0.25 * math.sin(x * 0.9) if y < cy else 0.0
            if dx * dx + dy * dy <= 1.0 + bump:
                m.put(x, y, (5, shade if y > cy + h // 3 else color))


def layer3() -> Margins:
    m = Margins(3)
    continue_texture(m, 0, 256)

    def ridge(points: list[tuple[float, float]]) -> Callable[[float], float]:
        def f(x: float) -> float:
            for (xa, ya), (xb, yb) in zip(points, points[1:]):
                if xa <= x <= xb:
                    return ya + (yb - ya) * (x - xa) / (xb - xa)
            return 99.0

        return f

    mountain(m, ridge([(-W - 1, 49), (-44, 43), (-38, 44), (-30, 47), (-14, 53), (0, 57)]), -W, 0, 57)
    mountain(m, ridge([(239, 53), (252, 57), (262, 52), (276, 45), (281, 44), (292, 48), (240 + W, 51)]), 240, 240 + W, 57)
    m.sky = pic(3)[0][0]
    cloud(m, 262, 36, 7, 2)
    cloud(m, -30, 30, 6, 2)
    return m


def leaf(
    m: Margins,
    cx: float,
    cy: float,
    length: float,
    width: float,
    angle: float,
    light: int,
    dark: int,
    bank: int,
    shine: int | None = None,
    bend: float = 0.0,
    edge: int | None = None,
    fullness: float = 0.75,
) -> None:
    ca, sa = math.cos(angle), math.sin(angle)
    r = int(length) + 2
    for y in range(int(cy) - r, int(cy) + r + 1):
        for x in range(int(cx) - r, int(cx) + r + 1):
            dx, dy = x - cx, y - cy
            u = (dx * ca + dy * sa) / (length / 2.0)
            v = (-dx * sa + dy * ca) - bend * (u * u) * width
            if not -1.0 <= u <= 1.0:
                continue
            half = width / 2.0 * (1.0 - u * u) ** fullness
            if abs(v) > half:
                continue
            c = light if v < 0 else dark
            if edge is not None and abs(v) > half - 0.9 and v >= 0:
                c = edge
            m.put(x, y, (bank, c))
    if shine is not None:
        m.put(int(round(cx - ca * length * 0.2)), int(round(cy - sa * length * 0.2 - width * 0.15)), (bank, shine))


def rng_for(L: int, side: str) -> random.Random:
    return random.Random(1000 * L + (0 if side == "left" else 1))


def blades(
    m: Margins,
    xs: list[int],
    rnd: random.Random,
    bank: int,
    body: int,
    lit: int,
    base: int,
    tip_lo: int,
    tip_hi: int,
    gap: tuple[int, int] = (2, 4),
    base_at: Callable[[int], float] | None = None,
) -> None:
    x = xs[0] - 6
    while x <= xs[-1] + 6:
        h = rnd.randint(tip_lo, tip_hi)
        w = rnd.randint(3, 6)
        lean = rnd.uniform(-0.35, 0.35)
        if base_at is not None:
            base = int(round(base_at(x)))
        top = base - h
        for y in range(top, base + 1):
            t = (base - y) / max(1, h)
            half = w * (1.0 - t) / 2.0
            cx = x + lean * (base - y)
            x0, x1 = int(math.floor(cx - half)), int(math.ceil(cx + half))
            for xx in range(x0, max(x0 + 1, x1)):
                m.put(xx, y, (bank, body))
            if y < base - 1 and rnd.random() < 0.7:
                m.put(x0, y, (bank, lit))
        x += rnd.randint(*gap)


def layer2() -> Margins:
    m = Margins(2)
    for side in ("left", "right"):
        rnd = rng_for(2, side)
        xs = list(m.xs(side))
        near = side == "right"
        tip_lo, tip_hi = (6, 20) if near else (3, 11)
        crown: dict[int, float] = {}
        phase = rnd.uniform(0.0, 6.3)
        for x in xs:
            crown[x] = 71.0 + 3.5 * math.sin(x / 7.0 + phase) + 2.0 * math.sin(x / 3.1 + 2.0 * phase) + rnd.uniform(-1.0, 1.0)
            for y in range(48, 256):
                m.put(x, y, (2, 8 if y < 62 else 6 if y < crown[x] else 2 if y < 134 else 12))
        blades(m, xs, rnd, 2, 8, 13, 48, tip_lo, tip_hi)
        x = xs[0] - 2
        while x <= xs[-1] + 2:
            h = rnd.choice((0, 2, 4, 6, 8, 10))
            for d in range(h):
                for k in range(2 if d < h - 2 else 1):
                    m.put(x + k, 61 - d, (2, 6))
            x += rnd.randint(3, 7)
        for _ in range(len(xs) // 3):
            cx, cy = rnd.uniform(xs[0], xs[-1]), rnd.uniform(58, 70)
            leaf(m, cx, cy, rnd.uniform(3, 6), 2.2, rnd.uniform(-2.8, -0.3), 9, 9, 2)
        for _ in range(int(len(xs) * 1.7)):
            cx, cy = rnd.uniform(xs[0] - 3, xs[-1] + 3), rnd.uniform(68, 104)
            ang = rnd.choice((rnd.uniform(-2.7, -1.9), rnd.uniform(-1.2, -0.4), rnd.uniform(0.3, 0.9)))
            leaf(m, cx, cy, rnd.uniform(5, 8), rnd.uniform(2.5, 3.5), ang, rnd.choice((9, 7, 7)), 5, 2)
            if rnd.random() < 0.18:
                m.put(int(cx) + 1, int(cy), (2, 13))
        for _ in range(len(xs) // 8):
            cx, cy = rnd.uniform(xs[0], xs[-1]), rnd.uniform(96, 110)
            leaf(m, cx, cy, rnd.uniform(10, 16), rnd.uniform(4, 6), rnd.uniform(-0.3, 0.3), 1, 1, 2)
        x = xs[0] - 10 + rnd.randint(0, 12)
        while x < xs[-1] + 16:
            root = rnd.randint(124, 130)
            fan = [
                (-1.95, 7, 5, 3, 18, 0, -8),
                (-1.2, 7, 5, 3, 18, 0, -8),
                (-2.65, 9, 7, 3, 24, -4, -2),
                (-0.5, 9, 7, 3, 24, 4, -2),
                (2.2, 6, 4, 2, 18, -3, 4),
                (0.95, 6, 4, 2, 18, 3, 4),
                (2.85, 7, 5, 2, 24, -6, 6),
                (0.3, 7, 5, 2, 24, 6, 6),
            ]
            for ang, lt, dk, rim, ln, ox, oy in fan:
                ang += rnd.uniform(-0.15, 0.15)
                ln_s = ln * rnd.uniform(0.85, 1.1)
                cx = x + ox + math.cos(ang) * ln_s * 0.5
                cy = root + oy + math.sin(ang) * ln_s * 0.4
                leaf(
                    m,
                    cx,
                    cy,
                    ln_s,
                    ln_s * 0.52,
                    ang,
                    lt,
                    dk,
                    2,
                    edge=rim,
                    shine=14 if rnd.random() < 0.5 else (13 if rnd.random() < 0.3 else None),
                    bend=0.25,
                )
            x += rnd.randint(38, 46)
        continue_edge(m, side, 50, 150, gap=lambda y, c: c in ((2, 2), (2, 1), (2, 12), (2, 6), (2, 8)))
    return m


def tier(y: float) -> tuple[int, int, int, int]:
    if y < 34:
        return 0, 4, 3, 1
    if y < 56:
        return 2, 4, 3, 1
    return 3, 4, 3, 1


def shaped_leaf(
    m: Margins,
    cx: float,
    cy: float,
    length: float,
    width: float,
    angle: float,
    bank: int,
    fill: int,
    outline: int,
    shade: int,
    heart: bool = False,
    dew: int | None = None,
) -> None:
    ca, sa = math.cos(angle), math.sin(angle)
    r = int(length) + 2
    inside: dict[tuple[int, int], tuple[float, float]] = {}
    for y in range(int(cy) - r, int(cy) + r + 1):
        for x in range(int(cx) - r, int(cx) + r + 1):
            dx, dy = x - cx, y - cy
            u = (dx * ca + dy * sa) / (length / 2.0)
            v = -dx * sa + dy * ca
            if not -1.0 <= u <= 1.0:
                continue
            half = width * 0.58 * (1.0 + u) ** 0.35 * (1.0 - u) ** 0.7 if heart else width / 2.0 * (1.0 - u * u) ** 0.6
            if abs(v) <= half:
                inside[(x, y)] = (v, half)
    for (x, y), (v, half) in inside.items():
        border = any((x + dx, y + dy) not in inside for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        if border:
            c = outline if v < 0 else shade
        elif v > half - 2.2 or (v > 0 and abs(v) < 0.6):
            c = outline
        else:
            c = fill
        m.put(x, y, (bank, c))
    if dew is not None:
        px, py = int(round(cx)), int(round(cy))
        for dx, c in ((-2, 1), (-1, 14), (0, 4), (1, 14), (2, 1)):
            m.put(px + dx, py, (dew, c))
        m.put(px - 1, py + 1, (dew, 1))
        m.put(px, py + 1, (dew, 13))
        m.put(px + 1, py + 1, (dew, 1))


def plant(m: Margins, rnd: random.Random, x0: float, top: int, bottom: int = 80) -> None:
    phase = rnd.uniform(0.0, 6.3)
    stem = lambda y: x0 + 1.5 * math.sin(y / 10.0 + phase)
    for _ in range(9):
        yy = rnd.uniform(top + 6, bottom - 6)
        ang = rnd.choice((-0.4, 0.4, math.pi - 0.4, math.pi + 0.4)) + rnd.uniform(-0.3, 0.3)
        bank = 0 if yy < 34 else 2
        shaped_leaf(m, stem(yy) + math.cos(ang) * 8, yy + math.sin(ang) * 5, rnd.uniform(10, 14), 6, ang, bank, 1, 1, 1)
    for y in range(top + 4, bottom):
        m.put(int(round(stem(y))), y, (tier(y)[0], 1))
    y_cur = float(top)
    for s in (-1, 1):
        ang = -math.pi / 2.0 + s * rnd.uniform(0.55, 0.8)
        ln = rnd.uniform(14, 17)
        shaped_leaf(m, stem(y_cur + 8) + math.cos(ang) * ln * 0.45, y_cur + 8 + math.sin(ang) * ln * 0.45, ln, 7.5, ang, 0, 4, 3, 1)
    y_cur += 14
    for s in (-1, 1):
        ang = (-0.25 if s > 0 else math.pi + 0.25) + rnd.uniform(-0.15, 0.15)
        ln = rnd.uniform(14, 18)
        bank = tier(y_cur)[0]
        shaped_leaf(m, stem(y_cur) + math.cos(ang) * ln * 0.5, y_cur + math.sin(ang) * ln * 0.3, ln, 8, ang, bank, 4, 3, 1)
    y_cur += 14
    first = True
    while y_cur < bottom - 6:
        for s in (-1, 1):
            if rnd.random() < 0.25 and not first:
                continue
            ang = (0.55 if s > 0 else math.pi - 0.55) + rnd.uniform(-0.15, 0.15)
            ln = rnd.uniform(15, 18)
            bank = 2 if y_cur < 56 else 3
            shaped_leaf(
                m,
                stem(y_cur) + math.cos(ang) * ln * 0.45,
                y_cur + math.sin(ang) * ln * 0.4,
                ln,
                11,
                ang,
                bank,
                4,
                3,
                1,
                heart=True,
                dew=3 if first and s > 0 else None,
            )
        first = False
        y_cur += rnd.randint(13, 16)


def layer1() -> Margins:
    m = Margins(1)
    for side in ("left", "right"):
        rnd = rng_for(1, side)
        xs = list(m.xs(side))
        dist: Callable[[float], float] = (lambda x: -1.0 - x) if side == "left" else (lambda x: x - 240.0)
        pit = (3, 10)
        quiet = 0
        if side == "left":
            m.patchable = lambda x, y: x < 8 and y >= 72
            xs = list(range(-W, 8))
            dist = lambda x: 7.0 - x
            fringe = lambda x: 116.0 + 0.02 * dist(x)
            pit_top = lambda x: 133.0 + 0.3 * dist(x) + 0.004 * dist(x) ** 2
        else:
            fringe = lambda x: 112.0 - 0.05 * dist(x)
            pit_top = lambda x: 138.0 + 0.4 * dist(x)
        for x in xs:
            for y in range(73 if x >= 0 else 84, 256):
                c = (3, 12) if y < fringe(x) else (3, 11) if y < pit_top(x) else pit
                m.put(x, y, c)
            yb = int(pit_top(x))
            for y in range(yb - 3, yb):
                if rnd.random() < 0.3:
                    m.put(x, y, pit)
        blades(m, [x for x in xs if dist(x) >= quiet], rnd, 3, 11, 11, 0, 2, 7, gap=(2, 3), base_at=lambda x: fringe(x) + 3.0)
        for _ in range(len(xs) // 3):
            cx, cy = rnd.uniform(xs[0] - 4, xs[-1] + 4), rnd.uniform(58, 86)
            leaf(m, cx, cy, rnd.uniform(12, 18), rnd.uniform(7, 10), rnd.uniform(-0.5, 0.5) + rnd.choice((0.0, math.pi)), 12, 12, 3)
        for _ in range(len(xs) // 6):
            cx, cy = rnd.uniform(xs[0], xs[-1]), rnd.uniform(14, 50)
            leaf(m, cx, cy, rnd.uniform(10, 14), rnd.uniform(5, 7), rnd.uniform(-0.6, 0.6) + rnd.choice((0.0, math.pi)), 1, 1, 0 if cy < 34 else 2)
        centre = xs[0] + rnd.uniform(14, 22)
        while centre < xs[-1] + 12:
            plant(m, rnd, centre, rnd.randint(6, 18))
            centre += rnd.uniform(36, 42)
        for _ in range(int(len(xs) * 0.7)):
            cx, cy = rnd.uniform(xs[0] - 3, xs[-1] + 3), rnd.uniform(74, 106)
            if dist(cx) < quiet + 6:
                continue
            ang = rnd.choice((rnd.uniform(-2.8, -2.2), rnd.uniform(-0.9, -0.3)))
            leaf(m, cx, cy, rnd.uniform(9, 14), rnd.uniform(4, 6), ang, 4, 3, 3, edge=1)
        gap_fn = lambda y, c: y >= 74 and c in ((3, 12), (2, 12))
        dark = ((0, 1), (2, 1), (3, 1), (3, 12))
        if side == "left":
            continue_edge(m, side, 0, 72, gap=lambda y, c: c in dark)
            continue_edge(m, side, 72, 112, gap=gap_fn, edge=8)
        else:
            continue_edge(m, side, 0, 72, gap=lambda y, c: c in dark)
            continue_edge(m, side, 72, 110, gap=gap_fn)
    return m


def layer0() -> Margins:
    m = Margins(0)
    for side in ("left", "right"):
        rnd = rng_for(0, side)
        xs = list(m.xs(side))

        def ground(x: int, side: str = side) -> float:
            d = -1 - x if side == "left" else x - 240
            return 185.0 - 0.35 * d - 0.003 * d * d

        for x in xs:
            for y in range(int(round(ground(x))), 256):
                m.put(x, y, (3, 11))
        blades(m, xs, rnd, 3, 11, 11, 0, 3, 11, gap=(2, 4), base_at=ground)
        continue_edge(m, side, 0, 110)
    return m


def generate() -> dict[tuple[int, str], object]:
    out: dict[tuple[int, str], object] = {}
    for L, fn in ((3, layer3), (2, layer2), (1, layer1), (0, layer0)):
        m = fn()
        out[(L, "left")] = m.c["left"]
        out[(L, "right")] = m.c["right"]
        out[(L, "sky")] = m.sky
        out[(L, "patch")] = m.patch
    return out


def encode(parts: dict[tuple[int, str], object]) -> bytes:
    def byte(c: tuple[int, int] | None) -> int:
        return 0xFF if c is None else (c[0] << 4) | c[1]

    blob = bytearray(MAGIC)
    for L in range(4):
        sky = parts[(L, "sky")]
        blob.append(byte(sky))  # type: ignore[arg-type]
        left: Canvas = parts[(L, "left")]  # type: ignore[assignment]
        right: Canvas = parts[(L, "right")]  # type: ignore[assignment]
        patch: dict[tuple[int, int], tuple[int, int] | None] = parts[(L, "patch")]  # type: ignore[assignment]
        for y in range(H):
            for x in range(-W, PATCH):
                blob.append(byte(left.get(-1 - x, y) if x < 0 else patch.get((x, y))))
        for y in range(H):
            for x in range(W):
                blob.append(byte(right.get(x, y)))
    return bytes(blob)


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate procedural intro widescreen margins (intro_margins.bin).")
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    data = encode(generate())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"intro margins: {len(data)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
