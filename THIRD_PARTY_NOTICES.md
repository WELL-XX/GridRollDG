# Third-party sources

GridRollDG board and matrix definitions are derived from the user's PCB1 and P1, read on 2026-10-09. GridRoll is a different PCB.

The PAW3222 GPIO driver, SDIO transport, transport regression test and bootloader/build helper structure are adapted from WELL-XX/GridRoll commit 4a1edef5a75c67d25741d07aa4105d1b40593956 under the MIT license, copyright (c) 2026 PandaKB. Its license is preserved in LICENSE. DYA Studio extensions are not included.

Official ZMK v0.3.0: https://github.com/zmkfirmware/zmk/tree/edf5c0814fd3ea202e43aad2d68fd32e882a518c (MIT). Upstream license files remain with fetched dependencies.
The ready-to-flash package also carries the firmware dependencies' root license files under licenses/firmware.

Adafruit nRF52 Bootloader 0.11.0: https://github.com/adafruit/Adafruit_nRF52_Bootloader/tree/c67f0bcf0fa8e841426335b1bbde91cda6ca1f50 (MIT). The first-flash package includes its license and the Nordic SoftDevice S140 6.1.1 license. The SoftDevice is used only on the Nordic nRF52840 in the E73 module. Bootloader dependencies (nrfx, TinyUSB, UF2) retain their upstream licenses in the package.

The encoder count is based on Alps Alpine EC11E15244B2: https://tech.alpsalpine.com/e/products/detail/EC11E15244B2/ (15 pulses, 30 detents). No manufacturer datasheet is redistributed.
