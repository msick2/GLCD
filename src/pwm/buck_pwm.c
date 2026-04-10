#include "buck_pwm.h"
#include "hal_pwm.h"
#include "board_config.h"

static hal_pwm_channel_t s_channels[BUCK_COUNT];
static float             s_duty[BUCK_COUNT];
static bool              s_initialised;

void buck_pwm_init(void) {
    if (s_initialised) return;

    const hal_pwm_config_t chg_cfg = {
        .pin     = BUCK_CHARGE_PWM_PIN,
        .freq_hz = BUCK_PWM_FREQ_HZ,
    };
    const hal_pwm_config_t dch_cfg = {
        .pin     = BUCK_DISCHARGE_PWM_PIN,
        .freq_hz = BUCK_PWM_FREQ_HZ,
    };

    hal_pwm_init(&s_channels[BUCK_CHARGE],    &chg_cfg);
    hal_pwm_init(&s_channels[BUCK_DISCHARGE], &dch_cfg);

    s_duty[BUCK_CHARGE]    = 0.0f;
    s_duty[BUCK_DISCHARGE] = 0.0f;

    /* Enable slice(s). If both channels happen to share the same slice
     * (GP0 / GP1 both on slice 0), enabling one enables both. */
    hal_pwm_enable(&s_channels[BUCK_CHARGE], true);
    if (s_channels[BUCK_CHARGE].slice != s_channels[BUCK_DISCHARGE].slice) {
        hal_pwm_enable(&s_channels[BUCK_DISCHARGE], true);
    }

    s_initialised = true;
}

void buck_pwm_set_duty(buck_id_t id, float duty) {
    if (id >= BUCK_COUNT) return;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    s_duty[id] = duty;
    hal_pwm_set_duty(&s_channels[id], duty);
}

float buck_pwm_get_duty(buck_id_t id) {
    return (id < BUCK_COUNT) ? s_duty[id] : 0.0f;
}

void buck_pwm_mute(buck_id_t id) {
    buck_pwm_set_duty(id, 0.0f);
}

void buck_pwm_emergency_stop(void) {
    buck_pwm_set_duty(BUCK_CHARGE,    0.0f);
    buck_pwm_set_duty(BUCK_DISCHARGE, 0.0f);
}
