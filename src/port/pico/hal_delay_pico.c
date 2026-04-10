#include "hal_delay.h"
#include "pico/time.h"

void hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void hal_delay_us(uint32_t us) {
    sleep_us(us);
}
