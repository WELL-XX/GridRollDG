"""Different execution hints are accepted; overlapping payload remains rejected."""
import sys
import tempfile
import unittest
from pathlib import Path
from intelhex import IntelHex, AddressOverlapError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"scripts"))
from package_firmware import merge_first_flash

def word(image, address, value):
    for i, byte in enumerate(value.to_bytes(4, "little")):
        image[address+i] = byte

class FirstFlashTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        boot = IntelHex()
        for address in (0, 0x1000, 0xF4000):
            boot[address] = 0
        for address, value in ((0x10001014,0xF4000),(0x10001200,18),
                               (0x10001204,18),(0x1000120C,0),(0x10001304,5)):
            word(boot, address, value)
        boot.start_addr = {"EIP":0xF4001}
        boot.write_hex_file(str(self.root/"boot.hex"))
        self.app = IntelHex()
        word(self.app,0x26000,0x20040000)
        word(self.app,0x26004,0x26009)
        self.app[0x26008] = 0
        self.app.start_addr = {"EIP":0x26009}
        self.app.write_hex_file(str(self.root/"app.hex"))

    def test_different_execution_hints_and_all_payload_preserved(self):
        merge_first_flash(self.root/"boot.hex", self.root/"app.hex", self.root/"merged.hex")
        merged = IntelHex(str(self.root/"merged.hex"))
        self.assertIsNone(merged.start_addr)
        for name in ("boot.hex","app.hex"):
            source = IntelHex(str(self.root/name))
            self.assertTrue(all(merged[a] == source[a] for a in source.addresses()))

    def test_uicr_overlap_still_rejected(self):
        word(self.app,0x1000120C,1)
        self.app.write_hex_file(str(self.root/"app.hex"))
        with self.assertRaises(AddressOverlapError):
            merge_first_flash(self.root/"boot.hex", self.root/"app.hex", self.root/"bad.hex")

if __name__ == "__main__":
    unittest.main()
