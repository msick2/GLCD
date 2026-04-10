#ifndef HAL_PWM_H
#define HAL_PWM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Abstract PWM channel handle. Populated by hal_pwm_init().
 * Callers must not interpret the fields — they are MCU-specific. */
typedef struct {
    uint8_t  slice;     /* hardware slice number */
    uint8_t  channel;   /* 0 = A, 1 = B (for RP2040/RP2350) */
    uint8_t  pin;       /* GPIO pin */
    uint16_t top;       /* counter top value: period = (top + 1) counts */
} hal_pwm_channel_t;

typedef struct {
    uint8_t  pin;
    uint32_t freq_hz;
} hal_pwm_config_t;

/* Configure a PWM channel on the given GPIO pin at the given frequency.
 * The slice is configured for level-low start (edge-aligned) mode. Returns
 * true on success. The output is NOT enabled yet; call hal_pwm_enable(). */
bool hal_pwm_init(hal_pwm_channel_t *ch, const hal_pwm_config_t *cfg);

/* Set duty cycle as a fraction 0.0f..1.0f. Values outside this range are
 * clamped. The new duty takes effect on the next PWM wrap, so updates are
 * atomic from the load switch's point of view. */
void hal_pwm_set_duty(hal_pwm_channel_t *ch, float duty);

/* Enable or disable the slice that owns this channel. Note: on RP2040 /
 * RP2350, a slice has two channels (A and B). Enabling or disabling the
 * slice affects both channels. To mute just one channel without touching
 * the slice, call hal_pwm_set_duty(ch, 0.0f). */
void hal_pwm_enable(hal_pwm_channel_t *ch, bool enable);

#ifdef __cplusplus
}
#endif

#endif
