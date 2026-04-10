#ifndef LCD_ST75256_CMDS_H
#define LCD_ST75256_CMDS_H

#define ST75256_EXT1                0x30
#define ST75256_EXT2                0x31

#define ST75256_SLEEP_OUT           0x94
#define ST75256_SLEEP_IN            0x95

#define ST75256_DISPLAY_OFF         0xAE
#define ST75256_DISPLAY_ON          0xAF

#define ST75256_SET_PAGE_ADDR       0x75
#define ST75256_SET_COL_ADDR        0x15
#define ST75256_WRITE_DATA          0x5C

#define ST75256_DATA_SCAN_DIR       0xBC
#define ST75256_DISPLAY_CTRL        0xCA
#define ST75256_DISPLAY_MODE_MONO   0xF0

#define ST75256_CONTRAST            0x81
#define ST75256_POWER_CTRL          0x20
#define ST75256_ANALOG_CTRL         0x32
#define ST75256_AUTO_READ_CTRL      0xD7
#define ST75256_OLED_MODE           0x9F
#define ST75256_GRAYSCALE_LVL       0x20

#endif
