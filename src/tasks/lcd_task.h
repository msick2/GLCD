#ifndef LCD_TASK_H
#define LCD_TASK_H

/* FreeRTOS task that owns the LCD on Core 1.
 * Renders the UI at 10 Hz and pushes the framebuffer to the panel. */
void lcd_task(void *arg);

#endif
