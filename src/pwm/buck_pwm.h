#ifndef BUCK_PWM_H
#define BUCK_PWM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUCK_CHARGE    = 0,
    BUCK_DISCHARGE = 1,
    BUCK_COUNT
} buck_id_t;

/* Configure both buck PWM channels from board_config.h pins at
 * BUCK_PWM_FREQ_HZ. Both start at 0% duty and the slice is enabled so
 * the HW is free-running but the output stays low until a non-zero duty
 * is set via buck_pwm_set_duty(). */
void buck_pwm_init(void);

/* Set duty as a fraction 0.0f..1.0f. Clamped internally.
 * NOTE: this does not perform any safety checks — the caller (control
 * loop or demo task) is responsible for bounds and soft-start. */
void buck_pwm_set_duty(buck_id_t id, float duty);

/* Read back the last commanded duty (cached in RAM). */
float buck_pwm_get_duty(buck_id_t id);

/* Mute a channel by forcing duty to 0. The slice keeps running.
 * For a hard disable, use buck_pwm_emergency_stop(). */
void buck_pwm_mute(buck_id_t id);

/* Force both bucks to 0% duty. Used from fault handlers. */
void buck_pwm_emergency_stop(void);

#ifdef __cplusplus
}
#endif

#endif
