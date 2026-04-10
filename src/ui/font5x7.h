#ifndef FONT5X7_HDR
#define FONT5X7_HDR

#include <stdint.h>
#include <stdbool.h>

#define FONT5X7_WIDTH  5
#define FONT5X7_HEIGHT 7

bool font5x7_get(char c, uint8_t out[FONT5X7_HEIGHT]);

#endif
