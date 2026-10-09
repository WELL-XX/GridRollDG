#!/usr/bin/env python3
"""Merge first-flash images without overlap and inventory downloadable artifacts."""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path
from intelhex import IntelHex
from check_addresses import validate_hex

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--bootloader", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    shutil.copytree(args.firmware, out, dirs_exist_ok=True)
    shutil.copytree(args.bootloader, out, dirs_exist_ok=True)
    bootstrap = args.bootloader/"GridRollDG-first-flash.hex"
    validate_hex(bootstrap, "bootloader")
    for side in ("left", "right"):
        app_path = args.firmware/f"GridRollDG-{side}.hex"
        validate_hex(app_path, "app")
        image = IntelHex(str(bootstrap))
        image.merge(IntelHex(str(app_path)), overlap="error")
        target = out/f"GridRollDG-{side}-first-flash.hex"
        image.write_hex_file(str(target))
        validate_hex(target, "merged")
    for name in ("README.md", "THIRD_PARTY_NOTICES.md", "LICENSE"):
        shutil.copyfile(ROOT/name, out/name)
    shutil.copytree(ROOT/"docs", out/"docs", dirs_exist_ok=True)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    version = {"repository": "WELL-XX/GridRollDG", "commit": commit,
               "zmk": "edf5c0814fd3ea202e43aad2d68fd32e882a518c",
               "zephyr": "dacab4875df72109b96cc8977547a0dc04875bcd",
               "bootloader": json.loads((ROOT/"bootloader/upstream.json").read_text(encoding="utf-8")),
               "hardware_validation": "PCB1 netlist and compiled devicetree checked",
               "physical_validation": "not yet performed"}
    (out/"version.json").write_text(json.dumps(version, indent=2)+"\n", encoding="utf-8")
    files = sorted(p for p in out.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
    (out/"SHA256SUMS").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest()+"  "+
                                         p.relative_to(out).as_posix()+"\n" for p in files), encoding="utf-8")
    print(f"Packaged {len(files)} files, including both complete first-flash images")

if __name__ == "__main__":
    main()
