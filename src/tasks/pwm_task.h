#ifndef PWM_TASK_H
#define PWM_TASK_H

/* FreeRTOS task that owns the two buck PWM channels on Core 1.
 *
 * Current implementation is a demo: it initializes the PWM hardware
 * and slowly sweeps duty on both bucks so the outputs can be verified
 * with a scope. This task will be replaced (or extended) in Phase 5
 * with the PI/CC-CV control logic driven by ADC feedback. */
void pwm_task(void *arg);

#endif
