#!/usr/bin/env python3
"""Procedural Emerald 3DS HD battle HUD & floating textbox generator (EM3DHUD1)."""

from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path
from typing import Callable

from PIL import Image, ImageChops, ImageDraw, ImageFilter

MAGIC = b"EM3DHUD1"
SS = 5
ZOOM = 1.4
K = SS * ZOOM

FRAMES = {
    "single_opponent": 0,
    "single_player": 1,
    "double_opponent": 2,
    "double_player": 3,
    "safari": 4,
}
ID_HP_TRACK = 8
ID_HP_FILL = {"green": 9, "yellow": 10, "red": 11}
ID_EXP_FILL = 12
ID_PILL = {"psn": 16, "par": 17, "slp": 18, "frz": 19, "brn": 20}
ID_BALL = 21
ID_BOX_CAP = 24
ID_BOX_MIDDLE = 25

SHAPES = {
    "single_opponent": ((2, 3, 92, 26), [[(92, 21), (101, 30), (12, 30), (9, 27), (2, 27), (2, 25)]]),
    "single_player": ((10, 3, 100, 34), [[(100, 20), (104, 20), (104, 35), (102, 37), (100, 38), (1, 38), (9, 30)]]),
    "double_opponent": ((2, 3, 92, 26), [[(92, 16), (101, 25), (92, 25)]]),
    "double_player": ((10, 3, 100, 26), [[(10, 16), (1, 25), (10, 25)]]),
    "safari": ((10, 3, 100, 34), [[(100, 20), (104, 20), (104, 35), (102, 37), (100, 38), (1, 38), (9, 30)]]),
}
EXP_RECT = (32.0, 34.6, 96.0, 37.0)
HP_BAR = (0.0, 0.0, 66.0, 6.0)
HP_TRACK = (15.0, 0.8, 65.2, 5.2)
HP_INSIDE = (15.8, 1.5, 64.4, 4.5)
PILL = (0.0, 0.0, 20.0, 8.0)

STATUS_COLOURS = {
    "psn": (192, 96, 192),
    "par": (184, 184, 24),
    "slp": (160, 160, 136),
    "frz": (136, 176, 224),
    "brn": (224, 112, 80),
}
FILL_GRADIENTS = {
    "green": [(0.0, (176, 255, 204)), (0.45, (98, 226, 142)), (1.0, (44, 168, 96))],
    "yellow": [(0.0, (255, 246, 150)), (0.45, (246, 208, 44)), (1.0, (196, 146, 8))],
    "red": [(0.0, (255, 176, 156)), (0.45, (240, 92, 64)), (1.0, (172, 46, 50))],
}

BOX_BAND_TOP = 192
BOX_TOP = 195
BOX_MARGIN = 8
BOX_BOTTOM_MARGIN = 3
BOX_CAP = 20


def lerp(a: tuple[float, ...], b: tuple[float, ...], t: float) -> tuple[float, ...]:
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(len(a)))


def gradient_rows(stops: list[tuple[float, tuple[float, ...]]], n: int) -> list[tuple[float, ...]]:
    out: list[tuple[float, ...]] = []
    for i in range(n):
        t = i / max(1, n - 1)
        for k in range(len(stops) - 1):
            t0, c0 = stops[k]
            t1, c1 = stops[k + 1]
            if t <= t1 or k == len(stops) - 2:
                u = 0.0 if t1 == t0 else min(1.0, max(0.0, (t - t0) / (t1 - t0)))
                out.append(lerp(c0, c1, u))
                break
    return out


class Canvas:
    def __init__(self, w: int, h: int, origin: tuple[float, float] = (0.0, 0.0)) -> None:
        self.w, self.h = w, h
        self.ox, self.oy = origin
        self.img = Image.new("RGBA", (w, h), (0, 0, 0, 0))

    def gx(self, x: float) -> float:
        return self.ox + x * K

    def gy(self, y: float) -> float:
        return self.oy + y * K

    def mask(self) -> Image.Image:
        return Image.new("L", (self.w, self.h), 0)

    def paint(self, mask: Image.Image, stops: list[tuple[float, tuple[float, ...]]], alpha: float = 1.0) -> None:
        box = mask.getbbox()
        if not box:
            return
        y0, y1 = box[1], box[3]
        rows = gradient_rows(stops, y1 - y0)
        column = Image.new("RGBA", (1, self.h))
        column.putdata(
            [tuple(int(round(v)) for v in rows[min(max(y - y0, 0), len(rows) - 1)]) + (255,) for y in range(self.h)]
        )
        grad = column.resize((self.w, self.h), Image.Resampling.NEAREST)
        if alpha < 1.0:
            mask = mask.point(lambda v: int(v * alpha))
        self.img.paste(grad, (0, 0), mask)

    def tint(self, mask: Image.Image, colour: tuple[int, int, int], alpha_rows: Callable[[int], float]) -> None:
        column = Image.new("L", (1, self.h))
        column.putdata([int(255 * min(1.0, max(0.0, alpha_rows(y)))) for y in range(self.h)])
        strength = ImageChops.multiply(column.resize((self.w, self.h), Image.Resampling.NEAREST), mask)
        self.img.paste(Image.new("RGBA", (self.w, self.h), colour + (255,)), (0, 0), strength)

    def rounded(self, x0: float, y0: float, x1: float, y1: float, r: float) -> Image.Image:
        m = self.mask()
        ImageDraw.Draw(m).rounded_rectangle((self.gx(x0), self.gy(y0), self.gx(x1) - 1, self.gy(y1) - 1), radius=r * K, fill=255)
        return m

    def capsule(self, x0: float, y0: float, x1: float, y1: float) -> Image.Image:
        return self.rounded(x0, y0, x1, y1, (y1 - y0) / 2.0)

    def corners(self, x0: float, y0: float, x1: float, y1: float, radii: tuple[float, float, float, float]) -> Image.Image:
        X0, Y0, X1, Y1 = self.gx(x0), self.gy(y0), self.gx(x1), self.gy(y1)
        m = self.mask()
        d = ImageDraw.Draw(m)
        d.rectangle((X0, Y0, X1 - 1, Y1 - 1), fill=255)
        for (cx, cy, sx, sy), r in zip(((X0, Y0, 1, 1), (X1, Y0, -1, 1), (X1, Y1, -1, -1), (X0, Y1, 1, -1)), radii):
            R = r * K
            if R <= 0:
                continue
            square = (min(cx, cx + sx * R), min(cy, cy + sy * R), max(cx, cx + sx * R) - 1, max(cy, cy + sy * R) - 1)
            d.rectangle(square, fill=0)
            q = self.mask()
            ImageDraw.Draw(q).ellipse((cx + sx * R - R, cy + sy * R - R, cx + sx * R + R - 1, cy + sy * R + R - 1), fill=255)
            clip = self.mask()
            ImageDraw.Draw(clip).rectangle(square, fill=255)
            m.paste(255, (0, 0), ImageChops.darker(q, clip))
        return m

    def bar(self, x0: float, y0: float, x1: float, y1: float) -> Image.Image:
        r = (y1 - y0) / 2.0
        return self.corners(x0, y0, x1, y1, (min(1.2, r), r, r, min(1.2, r)))

    def polygon(self, points: list[tuple[float, float]]) -> Image.Image:
        m = self.mask()
        ImageDraw.Draw(m).polygon([(self.gx(x), self.gy(y)) for x, y in points], fill=255)
        return m

    def out(self) -> Image.Image:
        return self.img.resize((self.w // SS, self.h // SS), Image.Resampling.BOX)


def union(*masks: Image.Image) -> Image.Image:
    out = masks[0].copy()
    for m in masks[1:]:
        out.paste(255, (0, 0), m)
    return out


def draw_frame(name: str) -> Image.Image:
    (x0, y0, x1, y1), polygons = SHAPES[name]
    cv = Canvas(int(106 * K) // SS * SS + SS * 2, int(40 * K) // SS * SS + SS * 2)
    big, small = 3.4, 1.6

    def radii(inset: float) -> tuple[float, float, float, float]:
        return (max(big - inset, 0.6), max(small - inset, 0.4), max(big - inset, 0.6), max(small - inset, 0.4))

    shadow = union(cv.corners(x0 - 1, y0 - 1, x1 + 1, y1 + 1, radii(-1)), *[cv.polygon(p) for p in polygons])
    cv.paint(shadow, [(0.0, (92, 116, 106)), (1.0, (70, 92, 84))])
    outline = cv.corners(x0, y0, x1, y1, radii(0))
    cv.paint(outline, [(0.0, (40, 66, 12)), (1.0, (26, 48, 0))])
    bevel = cv.corners(x0 + 1, y0 + 1, x1 - 1, y1 - 1, radii(1))
    cv.paint(bevel, [(0.0, (230, 222, 190)), (1.0, (206, 196, 150))])
    inside = cv.corners(x0 + 2, y0 + 2, x1 - 2, y1 - 2, radii(2))
    cv.paint(inside, [(0.0, (255, 255, 240)), (0.5, (255, 250, 222)), (1.0, (240, 230, 190))])
    edge = Image.eval(inside, lambda v: 255 - v).filter(ImageFilter.GaussianBlur(5))
    edge = Image.composite(edge, Image.new("L", edge.size, 0), inside)
    cv.img.paste(Image.new("RGBA", edge.size, (170, 150, 100, 255)), (0, 0), edge.point(lambda v: int(v * 0.55)))
    box = inside.getbbox()
    assert box is not None
    top, height = box[1], box[3] - box[1]
    cv.tint(inside, (255, 255, 255), lambda y: (max(0.0, 1.0 - (y - top) / (0.42 * height)) ** 1.6) * 0.7)
    cv.tint(inside, (255, 255, 255), lambda y: math.exp(-(((y - top) / height - 0.06) / 0.035) ** 2) * 0.8)
    if name == "single_player":
        ex0, ey0, ex1, ey1 = EXP_RECT
        cv.paint(cv.capsule(ex0 - 0.5, ey0 - 0.5, ex1 + 0.5, ey1 + 0.5), [(0.0, (52, 72, 62)), (1.0, (52, 72, 62))])
        cv.paint(cv.capsule(ex0, ey0, ex1, ey1), [(0.0, (70, 92, 84)), (1.0, (108, 128, 118))])
    return cv.out()


def draw_hp_track() -> Image.Image:
    x0, y0, x1, y1 = HP_BAR
    cv = Canvas(int((x1 + 1) * K) // SS * SS + SS, int((y1 + 1) * K) // SS * SS + SS)
    cv.paint(cv.bar(x0, y0, x1, y1), [(0.0, (96, 122, 104)), (1.0, (60, 84, 70))])
    cv.paint(cv.bar(*HP_TRACK), [(0.0, (255, 255, 255)), (1.0, (226, 232, 226))])
    cv.paint(cv.bar(*HP_INSIDE), [(0.0, (58, 58, 72)), (1.0, (96, 108, 102))])
    return cv.out()


def draw_fill(rect: tuple[float, float, float, float], stops: list[tuple[float, tuple[float, ...]]], gloss: bool) -> tuple[int, int, Image.Image]:
    x0, y0, x1, y1 = rect
    px, py = math.floor(x0 * ZOOM), math.floor(y0 * ZOOM)
    cv = Canvas(int((x1 - x0 + 2) * K) // SS * SS + SS, int((y1 - y0 + 2) * K) // SS * SS + SS, origin=(-px * SS, -py * SS))
    cv.paint(cv.capsule(x0, y0, x1, y1), stops)
    if gloss:
        cv.paint(cv.capsule(x0 + 0.8, y0 + 0.4, x1 - 0.8, y0 + 1.0), [(0.0, (255, 255, 255)), (1.0, (255, 255, 255))])
    return px, py, cv.out()


def draw_pill(colour: tuple[int, int, int]) -> Image.Image:
    hi = tuple(min(255, int(c * 1.25 + 30)) for c in colour)
    lo = tuple(int(c * 0.72) for c in colour)
    x0, y0, x1, y1 = PILL
    cv = Canvas(int((x1 + 1) * K) // SS * SS + SS, int((y1 + 1) * K) // SS * SS + SS)
    cv.paint(cv.capsule(x0, y0, x1, y1), [(0.0, lo), (1.0, lo)])
    cv.paint(cv.capsule(x0 + 0.4, y0 + 0.4, x1 - 0.4, y1 - 0.4), [(0.0, hi), (0.5, colour), (1.0, lo)])
    return cv.out()


def draw_ball() -> Image.Image:
    cv = Canvas(int(8 * K) // SS * SS + SS, int(8 * K) // SS * SS + SS)
    cx, cy, r = cv.gx(3.5), cv.gy(3.5), 3.6 * K
    m = cv.mask()
    ImageDraw.Draw(m).ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    cv.paint(m, [(0.0, (60, 54, 74)), (1.0, (40, 36, 52))])
    r2 = r - 0.7 * K
    top = cv.mask()
    ImageDraw.Draw(top).pieslice((cx - r2, cy - r2, cx + r2, cy + r2), 180, 360, fill=255)
    cv.paint(top, [(0.0, (255, 140, 120)), (1.0, (214, 60, 56))])
    bottom = cv.mask()
    ImageDraw.Draw(bottom).pieslice((cx - r2, cy - r2, cx + r2, cy + r2), 0, 180, fill=255)
    cv.paint(bottom, [(0.0, (255, 255, 255)), (1.0, (196, 196, 210))])
    band = cv.mask()
    ImageDraw.Draw(band).rectangle((cx - r, cy - 0.45 * K, cx + r, cy + 0.45 * K), fill=255)
    cv.paint(band, [(0.0, (50, 46, 62)), (1.0, (50, 46, 62))])
    for rr, col in ((1.5, (50, 46, 62)), (0.9, (255, 255, 255))):
        b = cv.mask()
        ImageDraw.Draw(b).ellipse((cx - rr * K, cy - rr * K, cx + rr * K, cy + rr * K), fill=255)
        cv.paint(b, [(0.0, col), (1.0, col)])
    return cv.out()


def draw_text_box() -> tuple[Image.Image, Image.Image]:
    width, height = BOX_CAP + 4, 48
    cv = Canvas(width * SS, height * SS)

    def rr(x0: float, y0: float, x1: float, y1: float, r: float) -> Image.Image:
        m = cv.mask()
        ImageDraw.Draw(m).rounded_rectangle((x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1), radius=r * SS, fill=255)
        return m

    top, bottom = BOX_TOP - BOX_BAND_TOP, height - BOX_BOTTOM_MARGIN
    left, right = BOX_MARGIN, 400 - BOX_MARGIN
    sh = cv.mask()
    ImageDraw.Draw(sh).rounded_rectangle(((left + 1) * SS, (top + 1.5) * SS, (right - 1) * SS, (bottom + 2.5) * SS), radius=7 * SS, fill=120)
    sh = sh.filter(ImageFilter.GaussianBlur(1.6 * SS))
    cv.img.paste(Image.new("RGBA", sh.size, (30, 40, 34, 255)), (0, 0), sh)

    frame = rr(left, top, right, bottom, 6.5)
    cv.paint(frame, [(0.0, (236, 104, 84)), (0.18, (222, 80, 62)), (1.0, (176, 52, 44))])
    rim = cv.mask()
    ImageDraw.Draw(rim).rounded_rectangle((left * SS, top * SS, right * SS - 1, bottom * SS - 1), radius=6.5 * SS, outline=255, width=int(0.8 * SS))
    cv.tint(rim, (255, 190, 170), lambda y: (max(0.0, 1.0 - y / (height * SS))) ** 2.5)

    cv.paint(rr(left + 6, top + 1, right - 6, bottom - 1, 5), [(0.0, (150, 158, 158)), (1.0, (118, 126, 128))])
    panel = rr(left + 7, top + 2, right - 7, bottom - 2, 4.5)
    cv.paint(panel, [(0.0, (128, 186, 186)), (0.5, (107, 165, 165)), (1.0, (88, 146, 148))])
    cv.tint(panel, (40, 70, 72), lambda y: max(0.0, 1.0 - (y - (top + 2) * SS) / (2.2 * SS)) * 0.27)
    img = cv.out()
    cap = img.crop((0, 0, BOX_CAP, height))
    middle = img.crop((BOX_CAP + 2, 0, BOX_CAP + 3, height))
    return cap, middle


def element(eid: int, x: int, y: int, image: Image.Image) -> bytes:
    rgba = image.convert("RGBA")
    w, h = rgba.size
    return struct.pack("<BBhhHH", eid, 0, x, y, w, h) + rgba.tobytes()


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate procedural HD battle HUD art (hud.bin).")
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()

    items: list[tuple[int, int, int, Image.Image]] = []
    for name, eid in FRAMES.items():
        items.append((eid, 0, 0, draw_frame(name)))
    items.append((ID_HP_TRACK, 0, 0, draw_hp_track()))
    for colour, eid in ID_HP_FILL.items():
        items.append((eid, *draw_fill(HP_INSIDE, FILL_GRADIENTS[colour], True)))
    items.append((ID_EXP_FILL, *draw_fill(EXP_RECT, [(0.0, (170, 236, 255)), (0.5, (66, 196, 255)), (1.0, (30, 128, 214))], False)))
    for status, eid in ID_PILL.items():
        items.append((eid, 0, 0, draw_pill(STATUS_COLOURS[status])))
    items.append((ID_BALL, 0, 0, draw_ball()))
    cap, middle = draw_text_box()
    items.append((ID_BOX_CAP, 0, BOX_BAND_TOP, cap))
    items.append((ID_BOX_MIDDLE, BOX_CAP, BOX_BAND_TOP, middle))

    data = bytearray(MAGIC + struct.pack("<H", len(items)))
    for item in items:
        data += element(*item)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(bytes(data))
    print(f"hud art: {len(data)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
