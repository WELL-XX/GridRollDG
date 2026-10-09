#!/usr/bin/env python3
"""Build all locked ZMK targets and validate their effective configurations."""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path
import yaml

ROOT = Path(__file__).resolve().parents[1]
TARGETS = {e["artifact-name"].removeprefix("GridRollDG-"): e
           for e in yaml.safe_load((ROOT/"build.yaml").read_text(encoding="utf-8"))["include"]}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--only", choices=TARGETS)
    args = parser.parse_args()
    workspace, out = args.workspace.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    for name, entry in TARGETS.items():
        if args.only and args.only != name:
            continue
        stem = entry["artifact-name"]
        build = workspace/"build"/name
        command = ["west", "build", "-p", "-s", str(workspace/"zmk/app"),
                   "-d", str(build), "-b", entry["board"]]
        if entry.get("snippet"):
            command += ["-S", entry["snippet"]]
        command += ["--", f"-DZMK_CONFIG={ROOT}/config", f"-DZMK_EXTRA_MODULES={ROOT}",
                    f"-DSHIELD={entry['shield']}"]
        log = out/f"{stem}.log"
        print(f"Building {stem}", flush=True)
        with log.open("w") as stream:
            result = subprocess.run(command, cwd=workspace, stdout=stream,
                                    stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text(encoding="utf-8", errors="replace")[-12000:])
            sys.exit(result.returncode)
        subprocess.run([sys.executable, str(ROOT/"scripts/check_hardware.py"),
                        "--build", str(build), "--target", name], check=True)
        for ext in ("uf2", "hex", "bin", "elf", "map"):
            shutil.copyfile(build/f"zephyr/zmk.{ext}", out/f"{stem}.{ext}")
        shutil.copyfile(build/"zephyr/.config", out/f"{stem}.config")
        shutil.copyfile(build/"zephyr/zephyr.dts", out/f"{stem}.dts")
        subprocess.run([sys.executable, str(ROOT/"scripts/check_addresses.py"),
                        str(out/f"{stem}.hex"), "--uf2", str(out/f"{stem}.uf2"),
                        "--bin", str(out/f"{stem}.bin")], check=True)
    license_roots = {"ZMK": workspace/"zmk", "Zephyr": workspace/"zephyr",
                     "CMSIS": workspace/"modules/hal/cmsis",
                     "Nordic-nrfx": workspace/"modules/hal/nordic/nrfx",
                     "TinyCrypt": workspace/"modules/crypto/tinycrypt",
                     "nanopb": workspace/"modules/lib/nanopb",
                     "ZMK-Studio-messages": workspace/"modules/msgs/zmk-studio-messages",
                     "SEGGER": workspace/"modules/debug/segger"}
    for name, source in license_roots.items():
        candidates = sorted(set(source.glob("LICENSE*")) | set(source.glob("license*")))
        if name in ("ZMK", "Zephyr"):
            assert candidates, f"Missing upstream license: {name}"
        for candidate in candidates:
            if candidate.is_file():
                target = out/"licenses/firmware"/name/candidate.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(candidate, target)

if __name__ == "__main__":
    main()
