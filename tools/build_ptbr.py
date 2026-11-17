#!/usr/bin/env python3
"""Prepare an isolated PT-BR source tree without changing the English repository.

Examples:
    python tools/build_ptbr.py --stage-dir ../pokeemerald-ptbr-build
    python tools/build_ptbr.py --stage-dir ../pokeemerald-ptbr-build --build

Run the normal build_vita.ps1 / build_vita.sh from the staged tree if
the required converted assets and VitaSDK toolchain are available.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def ignored(_directory: str, names: list[str]) -> set[str]:
    # Keep generated game assets, which the normal build requires.
    return {name for name in names if name in {
        ".git", "build", "build-vita", "__pycache__", ".pytest_cache",
        ".venv", "_transfer", ".cache"
    } or name.lower().endswith((".vpk", ".gba", ".elf", ".velf"))}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, required=True,
                        help="new sibling folder for the opt-in PT-BR build")
    parser.add_argument("--build", action="store_true",
                        help="invoke the standard Vita build from the staging folder")
    args = parser.parse_args()
    destination = args.stage_dir.resolve()
    if destination == ROOT or ROOT in destination.parents or destination in ROOT.parents:
        parser.error("the staging folder must be outside the source tree")
    if destination.exists():
        parser.error(f"staging folder already exists: {destination}")

    # Validate on the English baseline before copying any file.
    verify = [sys.executable, str(ROOT / "tools/localize_portuguese.py"),
              "--tree", str(ROOT), "--check"]
    subprocess.run(verify, check=True)

    print(f"Copying unmodified sources and existing game assets into {destination}", flush=True)
    shutil.copytree(ROOT, destination, ignore=ignored)
    # The translation script writes only into the new staging tree.
    subprocess.run([sys.executable, str(destination / "tools/localize_portuguese.py"),
                    "--tree", str(destination)], check=True)
    print(f"PT-BR source prepared: {destination}", flush=True)

    if args.build:
        if sys.platform == "win32":
            command = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                       "-File", str(destination / "build_vita.ps1")]
        else:
            command = ["bash", str(destination / "build_vita.sh")]
        subprocess.run(command, cwd=destination, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
