// SPDX-License-Identifier: MIT
#ifndef GRIDROLLDG_BOOT_BOARD_H
#define GRIDROLLDG_BOOT_BOARD_H
#define LEDS_NUMBER        0
#define NEOPIXELS_NUMBER   0
#define UICR_REGOUT0_VALUE UICR_REGOUT0_VOUT_3V3
// PCB1 supplies VDD and VDDH together from its external 3.3 V LDO.
#define ENABLE_DCDC_0       0
#define ENABLE_DCDC_1       0
#define BLEDIS_MANUFACTURER "WELL-XX"
#define BLEDIS_MODEL        "GridRollDG E73"
// pid.codes in-house test identity; replace before distribution/manufacturing.
#define USB_DESC_VID          0x1209
#define USB_DESC_UF2_PID      0x0001
#define USB_DESC_CDC_ONLY_PID 0x0002
#define UF2_PRODUCT_NAME      "GridRollDG Bootloader"
#define UF2_VOLUME_LABEL      "GRIDDGBOOT"
#define UF2_BOARD_ID          "nRF52840-GridRollDG-E73-v1"
#define UF2_INDEX_URL         "https://github.com/WELL-XX/GridRollDG"
#endif
