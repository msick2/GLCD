#include "draw.h"
#include "font5x7.h"
#include "lcd.h"

#include <string.h>

void draw_hline(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
    if (x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    for (uint16_t x = x1; x <= x2; ++x) lcd_draw_pixel(x, y, color);
}

void draw_vline(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color) {
    if (y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }
    for (uint16_t y = y1; y <= y2; ++y) lcd_draw_pixel(x, y, color);
}

void draw_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    if (w == 0 || h == 0) return;
    draw_hline(x,         (uint16_t)(x + w - 1), y,                       color);
    draw_hline(x,         (uint16_t)(x + w - 1), (uint16_t)(y + h - 1),   color);
    draw_vline(x,                  y,            (uint16_t)(y + h - 1),   color);
    draw_vline((uint16_t)(x + w - 1), y,         (uint16_t)(y + h - 1),   color);
}

uint16_t draw_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    uint8_t rows[FONT5X7_HEIGHT];
    if (!font5x7_get(c, rows)) return (uint16_t)(x + 6);
    for (int r = 0; r < FONT5X7_HEIGHT; ++r) {
        const uint8_t rw = rows[r];
        for (int col = 0; col < FONT5X7_WIDTH; ++col) {
            if (rw & (uint8_t)(0x10u >> col)) {
                lcd_draw_pixel((uint16_t)(x + col), (uint16_t)(y + r), color);
            }
        }
    }
    return (uint16_t)(x + 6);
}

uint16_t draw_text(uint16_t x, uint16_t y, const char *s, uint8_t color) {
    while (*s) {
        x = draw_char(x, y, *s, color);
        ++s;
    }
    return x;
}

void draw_text_right(uint16_t rx, uint16_t y, const char *s, uint8_t color) {
    const size_t len = strlen(s);
    /* Each glyph is 5px wide + 1px space = 6px advance. The right edge of
     * the last glyph sits at the start_x + len*6 - 2 (5 used + 1 space, but
     * we want the rightmost pixel of the last glyph). Match JS dtr: x = rx - len*6 + 1. */
    const uint16_t x = (uint16_t)((int)rx - (int)len * 6 + 1);
    draw_text(x, y, s, color);
}
