"""Corrupt artifacts must fail instead of being packaged for first flash."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from intelhex import IntelHex

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"scripts"))
from check_addresses import validate_hex, validate_uf2

class PayloadTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)/"app.hex"
        self.image = IntelHex()
        for address, value in ((0x26000, 0x20040000), (0x26004, 0x26009)):
            for i, byte in enumerate(value.to_bytes(4, "little")):
                self.image[address+i] = byte
        self.image[0x26008] = 0
        self.image.write_hex_file(str(self.path))

    def test_valid_application(self):
        validate_hex(self.path, "app")

    def uf2_block(self, address=0x26000, family=0xADA52840):
        block = bytearray(512)
        struct.pack_into("<8I", block, 0, 0x0A324655, 0x9E5D5157,
                         0x2000, address, 256, 0, 1, family)
        block[32:288] = bytes(self.image[address+i] for i in range(256))
        struct.pack_into("<I", block, 508, 0x0AB16F30)
        return block

    def test_valid_uf2_and_padding(self):
        path = Path(self.temp.name)/"valid.uf2"
        path.write_bytes(self.uf2_block())
        validate_uf2(path, self.image, self.image.addresses())

    def test_uf2_payload_mismatch_rejected(self):
        block = self.uf2_block()
        block[32] ^= 1
        path = Path(self.temp.name)/"mismatch.uf2"
        path.write_bytes(block)
        with self.assertRaises(AssertionError):
            validate_uf2(path, self.image, self.image.addresses())

    def test_uf2_storage_overlap_rejected(self):
        path = Path(self.temp.name)/"overlap.uf2"
        path.write_bytes(self.uf2_block(address=0xEC000))
        with self.assertRaises(AssertionError):
            validate_uf2(path, self.image, self.image.addresses())

    def test_uf2_unexpected_non_ff_padding_rejected(self):
        block = self.uf2_block()
        block[100] = 0
        path = Path(self.temp.name)/"padding.uf2"
        path.write_bytes(block)
        with self.assertRaises(AssertionError):
            validate_uf2(path, self.image, self.image.addresses())

    def test_storage_overlap_rejected(self):
        self.image[0xEC000] = 0
        self.image.write_hex_file(str(self.path))
        with self.assertRaises(AssertionError):
            validate_hex(self.path, "app")

    def test_invalid_vector_rejected(self):
        self.image[0x26004] = 0
        self.image.write_hex_file(str(self.path))
        with self.assertRaises(AssertionError):
            validate_hex(self.path, "app")

    def test_wrong_uf2_family_rejected(self):
        block = bytearray(512)
        struct.pack_into("<8I", block, 0, 0x0A324655, 0x9E5D5157,
                         0x2000, 0x26000, 256, 0, 1, 0xDEADBEEF)
        struct.pack_into("<I", block, 508, 0x0AB16F30)
        path = Path(self.temp.name)/"bad.uf2"
        path.write_bytes(block)
        with self.assertRaises(AssertionError):
            validate_uf2(path, self.image, self.image.addresses())

    def test_bootloader_without_nfc_reset_setup_rejected(self):
        with self.assertRaises(AssertionError):
            validate_hex(self.path, "bootloader")

if __name__ == "__main__":
    unittest.main()
