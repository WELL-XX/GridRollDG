#!/usr/bin/env python3
"""Validate actual flash payload and byte-for-byte UF2/HEX agreement."""
import argparse
import struct
from pathlib import Path
from intelhex import IntelHex

APP_START, APP_END, BOOT_START, FLASH_END = 0x26000, 0xEC000, 0xF4000, 0x100000
UICR_FIELDS = {0x10001014, 0x10001018, 0x10001200, 0x10001204,
               0x1000120C, 0x10001304}

def read32(image, address):
    return int.from_bytes(bytes(image[address + i] for i in range(4)), "little")

def validate_hex(path, kind):
    image = IntelHex(str(path))
    flash = [a for a in image.addresses() if a < 0x10000000]
    assert flash, "Empty flash image"
    allowed_uicr = {a+i for a in UICR_FIELDS for i in range(4)}
    assert all(a < FLASH_END or a in allowed_uicr for a in image.addresses()), "Unexpected memory address"
    if kind == "app":
        assert all(APP_START <= a < APP_END for a in flash), "Application overlaps reserved flash"
    else:
        assert all(a < APP_START or BOOT_START <= a < FLASH_END or
                   (kind == "merged" and APP_START <= a < APP_END) for a in flash), "Reserved flash overwritten"
        assert any(a < 0x1000 for a in flash), "MBR missing"
        assert any(0x1000 <= a < APP_START for a in flash), "SoftDevice missing"
        assert any(BOOT_START <= a < 0xFDC00 for a in flash), "Bootloader missing"
        assert read32(image, 0x10001014) == BOOT_START, "Incorrect bootloader UICR"
        assert read32(image, 0x10001200) == read32(image, 0x10001204) == 18, "Reset pin mismatch"
        assert read32(image, 0x1000120C) == 0, "NFC pins were not released"
        assert read32(image, 0x10001304) == 5, "Incorrect REGOUT0"
    if kind in ("app", "merged"):
        sp, reset = read32(image, APP_START), read32(image, APP_START+4)
        assert 0x20000000 < sp <= 0x20040000, "Invalid initial stack pointer"
        assert reset & 1 and APP_START <= (reset & ~1) < APP_END, "Invalid application reset vector"
    return image, flash

def validate_uf2(path, image, flash):
    data = Path(path).read_bytes()
    assert data and len(data) % 512 == 0, "Invalid UF2 length"
    covered = {}
    for off in range(0, len(data), 512):
        block = data[off:off+512]
        m1, m2, flags, addr, size, number, count, family = struct.unpack_from("<8I", block)
        assert (m1, m2) == (0x0A324655, 0x9E5D5157)
        assert struct.unpack_from("<I", block, 508)[0] == 0x0AB16F30
        assert flags & 0x2000 and family == 0xADA52840, "Wrong UF2 family"
        assert number == off//512 and count == len(data)//512
        assert 0 < size <= 476 and APP_START <= addr and addr+size <= APP_END, "UF2 overwrites reserved flash"
        for i, value in enumerate(block[32:32+size]):
            assert addr+i not in covered, "Duplicate UF2 payload address"
            covered[addr+i] = value
    assert all(covered.get(a) == image[a] for a in flash), "UF2/HEX content mismatch"
    hex_addresses = set(image.addresses())
    assert all(a in hex_addresses or value == 0xFF for a, value in covered.items()), "Unexpected UF2 data"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("hex", type=Path)
    parser.add_argument("--kind", choices=["app", "bootloader", "merged"], default="app")
    parser.add_argument("--uf2", type=Path)
    args = parser.parse_args()
    image, flash = validate_hex(args.hex, args.kind)
    if args.uf2:
        validate_uf2(args.uf2, image, flash)
    print(f"{args.hex.name}: payload validated ({min(flash):#x}–{max(flash):#x})")

if __name__ == "__main__":
    main()
