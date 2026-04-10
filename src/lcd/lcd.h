#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH   256
#define LCD_HEIGHT  160
/* 4-gray native mode: each chip page is 4 rows, 1 byte per (page, column)
 * encodes 4 pixels packed as 2 bits each. */
#define LCD_PAGES   (LCD_HEIGHT / 4)
#define LCD_FB_SIZE (LCD_WIDTH * LCD_PAGES)

/* Native 4-level gray (2 bits per pixel). */
#define LCD_COLOR_WHITE  0u   /* 00 */
#define LCD_COLOR_LIGHT  1u   /* 01 */
#define LCD_COLOR_DARK   2u   /* 10 */
#define LCD_COLOR_BLACK  3u   /* 11 */

void lcd_init(void);

void lcd_display_on(void);
void lcd_display_off(void);

void lcd_set_contrast(uint8_t value);

void lcd_backlight(bool on);

void lcd_clear(void);
void lcd_fill(uint8_t color);

void lcd_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);

void lcd_flush(void);

uint8_t *lcd_framebuffer(void);

/* Diagnostic: blast raw bytes (bypasses framebuffer). */
void lcd_test_fill_raw(uint8_t value);

/* Diagnostic: fill the entire screen with one of the 4 native gray levels. */
void lcd_test_fill_4gray(uint8_t color);

#ifdef __cplusplus
}
#endif

#endif
