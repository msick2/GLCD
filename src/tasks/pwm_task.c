#include "pwm_task.h"
#include "buck_pwm.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

/* Sweep parameters.
 *   CHARGE buck:    0%  → 50% → 0%  at 10 ms per 1% step
 *   DISCHARGE buck: 0%  → 80% → 0%  at 10 ms per 2% step (faster)
 *
 * Both sweeps run independently so you can see the two channels move on
 * a scope and confirm that GP0 and GP1 are driven at 85 kHz with the
 * expected duty. */
void pwm_task(void *arg) {
    (void)arg;

    buck_pwm_init();
    printf("pwm_task: bucks initialised at 85 kHz\n");

    TickType_t last_wake   = xTaskGetTickCount();
    const TickType_t step  = pdMS_TO_TICKS(10);

    float duty_chg  = 0.0f;
    float duty_dch  = 0.0f;
    int   dir_chg   = +1;
    int   dir_dch   = +1;

    while (true) {
        /* Charge buck sweep: ±0.01 per step, clamp 0..0.5 */
        duty_chg += (float)dir_chg * 0.01f;
        if (duty_chg >= 0.50f) { duty_chg = 0.50f; dir_chg = -1; }
        if (duty_chg <= 0.00f) { duty_chg = 0.00f; dir_chg = +1; }

        /* Discharge buck sweep: ±0.02 per step, clamp 0..0.8 */
        duty_dch += (float)dir_dch * 0.02f;
        if (duty_dch >= 0.80f) { duty_dch = 0.80f; dir_dch = -1; }
        if (duty_dch <= 0.00f) { duty_dch = 0.00f; dir_dch = +1; }

        buck_pwm_set_duty(BUCK_CHARGE,    duty_chg);
        buck_pwm_set_duty(BUCK_DISCHARGE, duty_dch);

        vTaskDelayUntil(&last_wake, step);
    }
}
