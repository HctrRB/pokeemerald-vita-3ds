"""Remaps a language's text manifest onto this repository's sources (developers only).

A text manifest (tools/locales/<code>/manifest.json.gz) says, for each source file of the game, which
spans of it hold text and where the same text lives in the language's official ROM: file ->
{sha256, edits: [start, end, rom offset, size, kind]}. The spans are character offsets into the file
*after* the language's patches (tools/locales/<code>/patches/) are applied, and sha256 pins that
patched text: tools/locales/localize.py refuses a file that changed since. No game text is stored.

When the sources change, run this again: every span is found in the new text by the exact text
around it (the same English string in the same place), so an edit can never land on other text; a
span that cannot be found uniquely is listed as unmapped and stays English.

    python tools/locales/remap_manifest.py --locale es
        remaps the current manifest from the sources of the commit it was made for (git history)
    python tools/locales/remap_manifest.py --locale es --source other.json.gz --reference DIR
                                           [--symbols syms.s.gz]
        imports another project's manifest; DIR holds the texts its spans refer to (files missing
        there are taken from this tree), --symbols the language ROM's symbol list, read for the
        Easy Chat ID lists' ROM offsets

Derived from the manifests of Emerald 3DS (tools/locales/*.json.gz, CC BY-NC-SA 4.0, NOTICE.md).
"""
import argparse
import gzip
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from localize import apply_patches, ID_LISTS  # noqa: E402  (shared with the stager)

CONTEXT = (24, 48, 96, 200, 400)


def git(*args, binary=False):
    out = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, check=True).stdout
    return out if binary else out.decode("utf-8")


def find_span(ref, target, s, e):
    """The span [s, e) of `ref` in `target`, found by its own text plus the text around it; None
    when it is not there exactly once"""
    for k in CONTEXT:
        for before, after in ((k, k), (k, 0), (0, k)):
            lo, hi = max(0, s - before), min(len(ref), e + after)
            seg = ref[lo:hi]
            at = target.find(seg)
            if at < 0:
                continue
            if target.find(seg, at + 1) >= 0:
                continue                                    # ambiguous: more context
            return at + (s - lo)
    return None


def patched_tree_text(rel, patches, base_text):
    """`base_text` of `rel` with the language's patches applied"""
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="") as fh:
            fh.write(base_text)
        apply_patches(tmp, patches, only=[rel])
        return open(path, encoding="utf-8").read()


def main():
    args = argparse.ArgumentParser(description="Remaps a text manifest onto this tree")
    args.add_argument("--locale", required=True)
    args.add_argument("--source", help="manifest to import (default: the language's own)")
    args.add_argument("--reference", help="folder with the texts the imported manifest refers to")
    args.add_argument("--symbols", help="symbol list of the language ROM (.s.gz), for an import")
    opts = args.parse_args()
    out_path = os.path.join(HERE, opts.locale, "manifest.json.gz")
    patch_dir = os.path.join(HERE, opts.locale, "patches")
    patches = sorted(os.path.join(patch_dir, p) for p in os.listdir(patch_dir)) if os.path.isdir(patch_dir) else []
    own = opts.source is None
    src = json.loads(gzip.decompress(open(opts.source or out_path, "rb").read()))

    def reference(rel):
        if own:
            # the sources and patches the manifest was made from
            old = git("show", f"{src['sourceCommit']}:{rel}")
            with tempfile.TemporaryDirectory() as tmp:
                old_patches = []
                listing = git("ls-tree", "--name-only", f"{src['sourceCommit']}:tools/locales/{opts.locale}/patches")
                for name in listing.split():
                    p = os.path.join(tmp, name)
                    open(p, "wb").write(git("show", f"{src['sourceCommit']}:tools/locales/{opts.locale}/patches/{name}", binary=True))
                    old_patches.append(p)
                return patched_tree_text(rel, sorted(old_patches), old)
        path = os.path.join(opts.reference, rel) if opts.reference else None
        if path and os.path.exists(path):
            return open(path, encoding="utf-8").read()
        return open(os.path.join(ROOT, rel), encoding="utf-8").read()

    files, unmapped, total = {}, [], 0
    for rel, entry in sorted(src["files"].items()):
        target = patched_tree_text(rel, patches, open(os.path.join(ROOT, rel), encoding="utf-8").read())
        ref = reference(rel)
        edits = []
        for s, e, offset, size, kind in entry["edits"]:
            total += 1
            at = find_span(ref, target, s, e) if ref != target else s
            if at is None:
                unmapped.append([rel, ref[s:e][:60]])
                continue
            edits.append([at, at + (e - s), offset, size, kind])
        edits.sort()
        files[rel] = {"sha256": hashlib.sha256(target.encode("utf-8")).hexdigest(), "edits": edits}

    id_lists = src.get("idLists")
    if id_lists is None:
        if not opts.symbols:
            sys.exit("an imported manifest needs --symbols for the Easy Chat ID lists")
        symbols = gzip.decompress(open(opts.symbols, "rb").read()).decode()
        id_lists = []
        for name, rel, constants in ID_LISTS:
            address = re.search(r"^\.equ %s, (\d+)$" % name, symbols, re.M)
            size = re.search(r"^\.size %s, (\d+)$" % name, symbols, re.M)
            id_lists.append([name, rel, constants, int(address.group(1)) - 0x08000000, int(size.group(1))])
    manifest = {
        "_comment": "Text manifest of this language (tools/locales/remap_manifest.py): source spans and ROM "
                    "offsets, no game text. Derived from Emerald 3DS's manifests (CC BY-NC-SA 4.0, NOTICE.md).",
        "version": 1,
        "romSha1": src.get("romSha1", src.get("rom_sha1")),
        "sourceCommit": git("rev-parse", "HEAD").strip(),
        "files": files,
        "unmapped": unmapped,
        "graphics": src["graphics"],
        "credits": src["credits"],
        "floors": src["floors"],
        "idLists": id_lists,
    }
    data = json.dumps(manifest, ensure_ascii=False, separators=(",", ":"), sort_keys=True).encode("utf-8")
    with open(out_path, "wb") as fh:
        fh.write(gzip.compress(data, mtime=0))
    print(f"{out_path}: {total - len(unmapped)} of {total} text spans mapped, {len(unmapped)} unmapped (stay English)")
    for rel, text in unmapped:
        print(f"  unmapped {rel}: {text!r}")


if __name__ == "__main__":
    main()
