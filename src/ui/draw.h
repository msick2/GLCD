#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <stdint.h>

void draw_hline(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color);
void draw_vline(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color);
void draw_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);

/* Draws one 5x7 char at (x,y). Returns x advanced by 6 (5 pixels + 1 spacing). */
uint16_t draw_char(uint16_t x, uint16_t y, char c, uint8_t color);

/* Left-aligned text. Returns final x. */
uint16_t draw_text(uint16_t x, uint16_t y, const char *s, uint8_t color);

/* Right-aligned text ending at rx (last char's rightmost pixel at rx). */
void draw_text_right(uint16_t rx, uint16_t y, const char *s, uint8_t color);

#endif
