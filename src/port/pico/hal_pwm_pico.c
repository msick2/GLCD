#include "hal_pwm.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

bool hal_pwm_init(hal_pwm_channel_t *ch, const hal_pwm_config_t *cfg) {
    if (!ch || !cfg || cfg->freq_hz == 0) return false;

    ch->pin     = cfg->pin;
    ch->slice   = (uint8_t)pwm_gpio_to_slice_num(cfg->pin);
    ch->channel = (uint8_t)pwm_gpio_to_channel(cfg->pin);

    /* Compute top so that sys_clk / (top + 1) equals target freq.
     * Divider is fixed at 1.0 for simplicity (integer period only). */
    const uint32_t sys_hz = clock_get_hz(clk_sys);
    const uint32_t top_plus_1 = sys_hz / cfg->freq_hz;
    if (top_plus_1 < 16 || top_plus_1 > 0x10000u) {
        return false;   /* frequency out of achievable range */
    }
    ch->top = (uint16_t)(top_plus_1 - 1);

    pwm_config pico_cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&pico_cfg, 1.0f);
    pwm_config_set_wrap(&pico_cfg, ch->top);
    pwm_init(ch->slice, &pico_cfg, false);   /* configured but not running */

    gpio_set_function(cfg->pin, GPIO_FUNC_PWM);
    pwm_set_gpio_level(cfg->pin, 0);          /* start muted at 0% duty */
    return true;
}

void hal_pwm_set_duty(hal_pwm_channel_t *ch, float duty) {
    if (!ch) return;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    const uint32_t period = (uint32_t)ch->top + 1u;
    const uint32_t level  = (uint32_t)(duty * (float)period + 0.5f);
    pwm_set_gpio_level(ch->pin, (uint16_t)level);
}

void hal_pwm_enable(hal_pwm_channel_t *ch, bool enable) {
    if (!ch) return;
    pwm_set_enabled(ch->slice, enable);
}
