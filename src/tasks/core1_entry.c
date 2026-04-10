#include "core1_entry.h"
#include "lcd_task.h"
#include "pwm_task.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lcd.h"

void core1_entry(void) {
    /* Core 1 owns the LCD bus. Initialise here so Core 0 never touches SPI0. */
    lcd_init();

    /* Create Core-1 tasks.
     *   lcd_task : LCD rendering at 10 Hz, priority 3
     *   pwm_task : buck PWM control, priority 4 (higher — real-time-ish)
     * More tasks (bms, eeprom, input, learn, uart, log) will be added in
     * later phases. */
    xTaskCreate(lcd_task, "lcd", 2048, NULL, 3, NULL);
    xTaskCreate(pwm_task, "pwm", 1024, NULL, 4, NULL);

    /* Hand control to the FreeRTOS scheduler. vTaskStartScheduler() does not
     * return while the scheduler is running. */
    vTaskStartScheduler();

    /* Unreachable: scheduler should never exit on its own. */
    for (;;) { }
}

/* ---- FreeRTOS required hooks (minimal stubs) ----------------------------- */

void vApplicationMallocFailedHook(void) {
    /* Heap exhausted. Stop here so the problem is obvious during debug. */
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name) {
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}
