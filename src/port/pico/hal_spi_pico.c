#include "hal_spi.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

static spi_inst_t *bus_to_inst(hal_spi_bus_t bus) {
    return (bus == HAL_SPI_BUS_1) ? spi1 : spi0;
}

void hal_spi_init(const hal_spi_config_t *cfg) {
    spi_inst_t *inst = bus_to_inst(cfg->bus);
    spi_init(inst, cfg->baudrate_hz);
    spi_set_format(inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(cfg->pin_sck,  GPIO_FUNC_SPI);
    gpio_set_function(cfg->pin_mosi, GPIO_FUNC_SPI);
    if (cfg->pin_miso != 0xFF) {
        gpio_set_function(cfg->pin_miso, GPIO_FUNC_SPI);
    }
}

void hal_spi_write(hal_spi_bus_t bus, const uint8_t *data, size_t len) {
    spi_write_blocking(bus_to_inst(bus), data, len);
}

void hal_spi_write_byte(hal_spi_bus_t bus, uint8_t b) {
    spi_write_blocking(bus_to_inst(bus), &b, 1);
}
