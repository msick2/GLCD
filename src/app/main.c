#include "lcd.h"
#include "ui.h"
#include "sim.h"
#include "draw.h"
#include "hal_delay.h"

#include <stdio.h>
#include "pico/stdlib.h"

/* Diagnostic pattern: every-other-row horizontal stripes.
 * Easiest pattern to see if init works at all. If you see stripes the
 * pipeline (SPI + page addressing) is OK and only contrast/orientation
 * may need tuning. */
static void draw_stripes(void) {
    uint8_t *fb = lcd_framebuffer();
    for (uint32_t i = 0; i < LCD_FB_SIZE; ++i) {
        fb[i] = 0x55;  /* bits 0,2,4,6 set within each 8-row page */
    }
    lcd_flush();
}

int main(void) {
    stdio_init_all();
    hal_delay_ms(200);
    printf("GLCD: ST75256 256x160 init\n");

    lcd_init();
    printf("GLCD: init done, starting UI demo\n");

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /* Render at 10 Hz (100 ms per frame).
     * Demo state advances every FRAMES_PER_STEP frames so transitions are
     * still readable, but the screen updates 10× per second. */
    const uint32_t FRAMES_PER_STEP = 15;     /* 1.5 s per demo step */
    uint32_t frame = 0;
    while (true) {
        const uint32_t step    = frame / FRAMES_PER_STEP;
        const uint8_t  page    = (uint8_t)((step / 6) % 7);
        const uint8_t  sim_idx = (uint8_t)(step % SIM_STATE_COUNT);
        uint8_t cur = 0, op = 0;
        if (page == 5) {           /* CONTROL */
            cur = (uint8_t)(step % 4);
            op  = (uint8_t)((step / 4) % 4);
        } else if (page == 6) {    /* SETTING */
            cur = (uint8_t)(step % 7);
        }

        ui_render(page, sim_idx, cur, op);

        /* Frame counter overlay in the top-right of the header bar.
         * Increments every frame so the user can see the 10 Hz refresh. */
        char fb[12];
        snprintf(fb, sizeof fb, "%lu", (unsigned long)(frame % 100000u));
        draw_text_right(LCD_WIDTH - 2, 3, fb, LCD_COLOR_WHITE);

        lcd_flush();

        gpio_put(LED_PIN, (frame & 1u) ? 1 : 0);
        hal_delay_ms(100);
        ++frame;
    }
    return 0;
}
