"""Compiles tools/locales/<code>/ui.json into the UI catalog a build packs as app0:ui2/locale.bin.

A language's ui.json maps English strings (the keys of en/ui.json, written by extract_ui.py) to their
translation: {"strings": {"Sort: %s": "Orden: %s", ...}}. A missing or empty entry falls back to
English at run time. The build stops when a translation:
  - has printf conversions that differ from the English ones (count, order or type), which would
    read the wrong arguments;
  - uses a letter its language does not list in tools/locales/languages.json, which the atlas would
    not have;
or when two English keys share a hash (the runtime looks strings up by hash).
It also writes <out>.txt: the missing strings and the translations much wider than the English
text, to check on the console. Format of locale.bin: src/platform/ui/ui_locale.c.

    python tools/locales/compile_ui.py --locale es --out build-vita-es/ui2/locale.bin
"""
import argparse
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
LANGUAGES = os.path.join(HERE, "languages.json")
TEMPLATE = os.path.join(HERE, "en", "ui.json")
FONTS = os.path.join(ROOT, "tools", "ui2", "fonts")
BASE_LETTERS = "éÉ·×"            # beyond ASCII, in every build's atlas (tools/ui2 bake pages, CHARS)
WIDER = 1.35                      # a translation this much wider than its English text is reported

CONVERSION = re.compile(r"%(%|[-+ #0]*(\*|\d+)?(?:\.(\*|\d+))?(hh|h|ll|l|z|j|t|L)?([diouxXeEfgGcspn]))")
TYPES = {"d": "int", "i": "int", "o": "uint", "u": "uint", "x": "uint", "X": "uint", "c": "int",
         "e": "double", "E": "double", "f": "double", "g": "double", "G": "double",
         "s": "string", "p": "pointer", "n": "count"}


def fnv1a(text):
    h = 2166136261
    for b in text.encode("utf-8"):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def conversions(text):
    """The arguments a format string reads: [(length, type)], '*' widths included"""
    out = []
    for m in CONVERSION.finditer(text):
        if m.group(1) == "%":
            continue
        out += [("", "int")] * ((m.group(2) == "*") + (m.group(3) == "*"))
        out.append((m.group(4) or "", TYPES[m.group(5)]))
    return out


def advance_table():
    """code point -> advance of VT323 32 (face 1), the UI's body text, base sheet + locale letters"""
    table = {}
    for name in ("ui_fonts.json", "locale_glyphs.json"):
        path = os.path.join(FONTS, name)
        if not os.path.exists(path):
            continue
        for f in json.load(open(path, encoding="utf-8"))["fonts"]:
            if (f["face"], f["px"]) == (1, 32):
                table.update({g[0]: g[7] for g in f["glyphs"]})
    return table


def width(text, adv):
    return sum(adv.get(ord(c), adv.get(ord("?"), 16)) for c in text)


def main():
    args = argparse.ArgumentParser(description="Compiles a language's UI catalog")
    args.add_argument("--locale", required=True)
    args.add_argument("--out", required=True)
    opts = args.parse_args()
    lang = next((l for l in json.load(open(LANGUAGES, encoding="utf-8"))["languages"] if l["code"] == opts.locale), None)
    if lang is None:
        sys.exit(f"unknown locale '{opts.locale}': not in {LANGUAGES}")
    keys = json.load(open(TEMPLATE, encoding="utf-8"))["strings"]
    source = os.path.join(HERE, opts.locale, "ui.json")
    strings = json.load(open(source, encoding="utf-8"))["strings"] if os.path.exists(source) else {}
    allowed = set(BASE_LETTERS) | set(lang["letters"])
    errors, missing, unused, wide = [], [], [], []

    hashes = {}
    for key in keys:
        other = hashes.setdefault(fnv1a(key), key)
        if other != key:
            errors.append(f"hash collision between {other!r} and {key!r}: reword one")
    adv = advance_table()
    entries = []
    for key in keys:
        text = strings.get(key)
        if not text:
            missing.append(key)
            continue
        if conversions(text) != conversions(key):
            errors.append(f"{key!r} -> {text!r}: printf conversions differ from the English ones")
        bad = sorted({c for c in text if ord(c) > 0x7E and c not in allowed} | {c for c in text if ord(c) < 0x20 and c != "\n"})
        if bad:
            errors.append(f"{key!r} -> {text!r}: letters not listed for '{opts.locale}' in languages.json: {''.join(bad)!r}")
        if width(text, adv) > width(key, adv) * WIDER + 16:
            wide.append((width(text, adv) / max(1, width(key, adv)), key, text))
        entries.append((fnv1a(key), text))
    unused = [k for k in strings if k not in keys]
    for e in errors:
        print(f"{source}: {e}")
    if errors:
        sys.exit(1)

    entries.sort()
    blob, table = bytearray(), bytearray()
    for h, text in entries:
        table += struct.pack("<II", h, len(blob))
        blob += text.encode("utf-8") + b"\0"
    data = b"UILC" + struct.pack("<HHI", 1, 0, len(entries)) + bytes(table) + bytes(blob)
    if not blob:
        data += b"\0"                                  # the loader wants a blob that ends with NUL
    os.makedirs(os.path.dirname(os.path.abspath(opts.out)), exist_ok=True)
    with open(opts.out, "wb") as fh:
        fh.write(data)
    with open(opts.out + ".txt", "w", encoding="utf-8", newline="\n") as fh:
        fh.write(f"{opts.locale}: {len(entries)} of {len(keys)} strings translated\n\n")
        fh.write(f"Missing ({len(missing)}), shown in English:\n" + "".join(f"  {k!r}\n" for k in missing))
        fh.write(f"\nIn ui.json but no longer used ({len(unused)}):\n" + "".join(f"  {k!r}\n" for k in unused))
        fh.write(f"\nMuch wider than the English text ({len(wide)}), check they fit:\n")
        fh.write("".join(f"  x{r:.2f} {k!r} -> {t!r}\n" for r, k, t in sorted(wide, reverse=True)))
    print(f"{opts.out}: {len(entries)} of {len(keys)} strings ({len(missing)} missing, {len(wide)} much wider; see {opts.out}.txt)")


if __name__ == "__main__":
    main()
