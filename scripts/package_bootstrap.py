#!/usr/bin/env python3
"""Initialize PCB1 reset/NFC UICR in the SoftDevice + USB bootloader image."""
import argparse
from intelhex import IntelHex

def put32(image, address, value):
    for i, byte in enumerate(value.to_bytes(4, "little")):
        image[address + i] = byte

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("merged_hex")
    parser.add_argument("output_hex")
    args = parser.parse_args()
    image = IntelHex(args.merged_hex)
    put32(image, 0x10001200, 18)  # PSELRESET[0]: P0.18
    put32(image, 0x10001204, 18)  # PSELRESET[1]: P0.18
    put32(image, 0x1000120C, 0)   # NFCPINS: P0.09/P0.10 are matrix GPIOs
    put32(image, 0x10001304, 5)   # REGOUT0 3.3 V; unused in normal VDD/VDDH mode
    image.write_hex_file(args.output_hex)

if __name__ == "__main__":
    main()
