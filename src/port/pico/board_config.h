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

/* ----- Buck converter PWM pins (RP2040/RP2350) -----
 * GP0 and GP1 share PWM slice 0 (channel A and B). That means both buck
 * converters run at exactly the same frequency, which keeps EMI prediction
 * simple. Duty is independent per channel. */
#define BUCK_CHARGE_PWM_PIN      0u     /* slice 0 A */
#define BUCK_DISCHARGE_PWM_PIN   1u     /* slice 0 B */
#define BUCK_PWM_FREQ_HZ         85000u

#endif
