#ifndef HAL_DELAY_H
#define HAL_DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hal_delay_ms(uint32_t ms);
void hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
