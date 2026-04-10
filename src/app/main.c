#include "core1_entry.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include <stdio.h>

/* Core 0 is bare-metal and hosts the real-time control loop in later phases
 * (PWM HW, ADC DMA, 8.5 kHz PI tick, OCP NMI). It must never call FreeRTOS
 * APIs. For now Core 0 just boots, hands the LCD/UI work to Core 1 via
 * FreeRTOS, then idles on WFI. */
int main(void) {
    stdio_init_all();
    sleep_ms(200);
    printf("GLCD: boot, launching Core 1 with FreeRTOS\n");

    multicore_launch_core1(core1_entry);

    while (true) {
        __wfi();
    }
    return 0;
}
