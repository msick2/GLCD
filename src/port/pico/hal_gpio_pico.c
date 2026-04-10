#include "hal_gpio.h"
#include "hardware/gpio.h"

void hal_gpio_init(hal_gpio_pin_t pin, hal_gpio_dir_t dir) {
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, dir == HAL_GPIO_DIR_OUT);
}

void hal_gpio_write(hal_gpio_pin_t pin, bool level) {
    gpio_put((uint)pin, level);
}

bool hal_gpio_read(hal_gpio_pin_t pin) {
    return gpio_get((uint)pin);
}
