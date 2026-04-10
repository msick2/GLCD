#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_SPI_BUS_0 = 0,
    HAL_SPI_BUS_1 = 1,
} hal_spi_bus_t;

typedef struct {
    hal_spi_bus_t bus;
    uint32_t      baudrate_hz;
    uint8_t       pin_sck;
    uint8_t       pin_mosi;
    uint8_t       pin_miso;
} hal_spi_config_t;

void hal_spi_init(const hal_spi_config_t *cfg);

void hal_spi_write(hal_spi_bus_t bus, const uint8_t *data, size_t len);

void hal_spi_write_byte(hal_spi_bus_t bus, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif
