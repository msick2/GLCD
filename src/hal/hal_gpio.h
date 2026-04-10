#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t hal_gpio_pin_t;

typedef enum {
    HAL_GPIO_DIR_IN  = 0,
    HAL_GPIO_DIR_OUT = 1,
} hal_gpio_dir_t;

void hal_gpio_init(hal_gpio_pin_t pin, hal_gpio_dir_t dir);

void hal_gpio_write(hal_gpio_pin_t pin, bool level);

bool hal_gpio_read(hal_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif
