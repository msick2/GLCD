#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "hal_spi.h"

/* ----- LCD module: JLX256160G-680 (ST75256, 256x160 mono) ----- */

#define LCD_SPI_BUS         HAL_SPI_BUS_0
#define LCD_SPI_BAUD_HZ     8000000u

/* JLX256160G-680 wiring (4-wire SPI): module D0=SCK, D1=SDA(MOSI), RS=DC */
#define LCD_PIN_SCK         18u   /* D0 */
#define LCD_PIN_MOSI        19u   /* D1 */
#define LCD_PIN_CS          17u
#define LCD_PIN_DC          22u   /* RS */
#define LCD_PIN_RST         16u

#define LCD_HAS_BL          0
#define LCD_PIN_BL          0xFFu
#define LCD_BL_ACTIVE_HIGH  1

/* ST75256 Vop (contrast). Tuned for JLX256160G-680. */
#define LCD_VOP_LOW         0x24u
#define LCD_VOP_HIGH        0x04u

#endif
