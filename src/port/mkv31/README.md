# MKV31 Port (Stub)

This folder is the placeholder for the NXP Kinetis MKV31 port. When you migrate
the project from the Raspberry Pi Pico to the MKV31, only the files in this
folder need to be implemented — the `lcd/`, `hal/`, and `app/` layers stay
unchanged.

## Files to implement

Mirror the names used in `port/pico/`:

- `board_config.h` — pin numbers, SPI peripheral selection, baud rate. Same
  symbol names as `port/pico/board_config.h` (`LCD_PIN_CS`, `LCD_PIN_DC`,
  `LCD_PIN_RST`, `LCD_PIN_BL`, `LCD_PIN_SCK`, `LCD_PIN_MOSI`, `LCD_SPI_BUS`,
  `LCD_SPI_BAUD_HZ`, `LCD_BL_ACTIVE_HIGH`).
- `hal_spi_mkv31.c` — implement `hal_spi_init`, `hal_spi_write`,
  `hal_spi_write_byte` from `hal/hal_spi.h` against the MKV31 DSPI/SPI
  peripheral driver (KSDK or bare register).
- `hal_gpio_mkv31.c` — implement `hal_gpio_init`, `hal_gpio_write`,
  `hal_gpio_read` from `hal/hal_gpio.h` against the MKV31 GPIO/PORT modules.
- `hal_delay_mkv31.c` — implement `hal_delay_ms`, `hal_delay_us` (SysTick or
  KSDK delay).

## CMake selection

Build the MKV31 variant by passing `-DMCU_TARGET=mkv31` to CMake. The top-level
`CMakeLists.txt` adds `src/port/${MCU_TARGET}/` to the include and source paths.
You will also need to replace `pico_sdk_import.cmake` / `pico_sdk_init()` with
the MKV31 toolchain file and SDK setup, but the `lcd/` and `hal/` layers will
not change.

## Mapping notes for MKV31

- ST75256 wants SPI mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit frames. The MKV31
  DSPI peripheral supports this directly.
- Maximum SPI clock for ST75256 is around 12 MHz; start at 8 MHz and tune.
- DC and RST are plain GPIO outputs; CS can be a GPIO or the DSPI hardware PCS
  line — using a GPIO keeps the HAL interface simple.
