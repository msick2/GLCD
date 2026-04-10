#include "lcd_task.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lcd.h"
#include "ui.h"
#include "draw.h"
#include "sim.h"

#include <stdio.h>

void lcd_task(void *arg) {
    (void)arg;

    uint32_t frame = 0;
    TickType_t last_wake = xTaskGetTickCount();

    /* Demo cadence identical to the pre-RTOS super-loop:
     *   - sim state advances every 15 frames (1.5 s)
     *   - page advances every 6 sim steps (9 s)
     *   - CONTROL / SETTING cursor and op also tick on the demo */
    const uint32_t FRAMES_PER_STEP = 15;

    while (true) {
        const uint32_t step    = frame / FRAMES_PER_STEP;
        const uint8_t  page    = (uint8_t)((step / 6) % 7);
        const uint8_t  sim_idx = (uint8_t)(step % SIM_STATE_COUNT);
        uint8_t cur = 0, op = 0;
        if (page == 5) {
            cur = (uint8_t)(step % 4);
            op  = (uint8_t)((step / 4) % 4);
        } else if (page == 6) {
            cur = (uint8_t)(step % 7);
        }

        ui_render(page, sim_idx, cur, op);

        /* Frame counter overlay on the header, proves the 10 Hz refresh. */
        char fb[12];
        snprintf(fb, sizeof fb, "%lu", (unsigned long)(frame % 100000u));
        draw_text_right(LCD_WIDTH - 2, 3, fb, LCD_COLOR_WHITE);

        lcd_flush();

        ++frame;
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));
    }
}
