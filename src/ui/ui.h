#ifndef UI_H
#define UI_H

#include <stdint.h>

/* Renders the UI into the LCD framebuffer (does not flush).
 * page    : 0..6 active tab
 * sim_idx : 0..5 active simulated state
 * cur     : cursor index (CONTROL/SETTING pages only)
 * op      : selected operation (CONTROL page only, 0=STOP 1=CHG 2=DCHG 3=LEARN)
 */
void ui_render(uint8_t page, uint8_t sim_idx, uint8_t cur, uint8_t op);

#endif
