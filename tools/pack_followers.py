#!/usr/bin/env python3
"""Packs assets/followers/{mon,text}/*.bin into a single PKF1 container.

The container stores entries uncompressed and sorted lexicographically by
relative key (e.g. "mon/025_f.bin", "text/es.bin") so the runtime can
binary-search the table of contents in RAM and read payloads with a single
seek+read, avoiding hundreds of small-file creations during VPK install.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys

MAGIC = b"PKF1"
VERSION = 1
HEADER_STRUCT = struct.Struct("<4sIII")
ENTRY_STRUCT = struct.Struct("<24sII")
MAX_KEY_BYTES = 23  # 24-byte slot including NUL terminator
ALIGN_BYTES = 4
ALLOWED_SUBDIRS = ("mon", "text")


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def collect_entries(input_dir: str) -> list[tuple[str, str]]:
    entries: list[tuple[str, str]] = []
    for subdir in ALLOWED_SUBDIRS:
        sub_path = os.path.join(input_dir, subdir)
        if not os.path.isdir(sub_path):
            continue
        for name in sorted(os.listdir(sub_path)):
            if not name.endswith(".bin"):
                continue
            full_path = os.path.join(sub_path, name)
            if not os.path.isfile(full_path):
                continue
            key = f"{subdir}/{name}"
            key_bytes = key.encode("ascii")
            if len(key_bytes) > MAX_KEY_BYTES:
                raise ValueError(f"Key '{key}' exceeds {MAX_KEY_BYTES} ASCII bytes")
            entries.append((key, full_path))

    entries.sort(key=lambda item: item[0].encode("ascii"))
    return entries


def pack_followers(input_dir: str, output_path: str) -> int:
    if not os.path.isdir(input_dir):
        raise FileNotFoundError(f"Input directory not found: {input_dir}")

    entries = collect_entries(input_dir)
    if not entries:
        raise ValueError(f"No .bin entries found under {input_dir}/{{{','.join(ALLOWED_SUBDIRS)}}}")

    table_offset = HEADER_STRUCT.size
    data_offset = align_up(table_offset + ENTRY_STRUCT.size * len(entries), ALIGN_BYTES)

    payloads: list[bytes] = []
    toc_records: list[tuple[bytes, int, int]] = []
    cursor = data_offset

    for key, full_path in entries:
        with open(full_path, "rb") as f:
            data = f.read()
        size = len(data)
        toc_records.append((key.encode("ascii"), cursor, size))
        payloads.append(data)
        cursor = align_up(cursor + size, ALIGN_BYTES)

    out_dir = os.path.dirname(os.path.abspath(output_path))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "wb") as out:
        out.write(HEADER_STRUCT.pack(MAGIC, VERSION, len(entries), 0))
        for key_bytes, offset, size in toc_records:
            out.write(ENTRY_STRUCT.pack(key_bytes, offset, size))

        pos = HEADER_STRUCT.size + ENTRY_STRUCT.size * len(entries)
        if pos < data_offset:
            out.write(b"\x00" * (data_offset - pos))

        for data, (_, offset, size) in zip(payloads, toc_records):
            out.write(data)
            padded = align_up(offset + size, ALIGN_BYTES) - (offset + size)
            if padded:
                out.write(b"\x00" * padded)

    return len(entries)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Pack follower .bin assets into a PKF1 container.")
    parser.add_argument("--input", default="assets/followers", help="Input followers directory")
    parser.add_argument("--output", required=True, help="Output .pak file path")
    args = parser.parse_args(argv)

    count = pack_followers(args.input, args.output)
    size_kb = os.path.getsize(args.output) / 1024.0
    print(f"Packed {count} follower assets into {args.output} ({size_kb:.1f} KiB)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
