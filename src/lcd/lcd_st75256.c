#include "lcd.h"
#include "lcd_st75256_cmds.h"
#include "hal_spi.h"
#include "hal_gpio.h"
#include "hal_delay.h"
#include "board_config.h"

#include <string.h>

static uint8_t s_fb[LCD_FB_SIZE];

static inline void cs_low(void)  { hal_gpio_write(LCD_PIN_CS,  false); }
static inline void cs_high(void) { hal_gpio_write(LCD_PIN_CS,  true);  }
static inline void dc_cmd(void)  { hal_gpio_write(LCD_PIN_DC,  false); }
static inline void dc_data(void) { hal_gpio_write(LCD_PIN_DC,  true);  }

/* All transactions hold CS low across the command byte and any data bytes
 * that follow, so the chip never exits the current write mode mid-sequence. */

static void cmd0(uint8_t c) {
    cs_low();
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, c);
    cs_high();
}

static void cmd1(uint8_t c, uint8_t d0) {
    cs_low();
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, c);
    dc_data();
    hal_spi_write_byte(LCD_SPI_BUS, d0);
    cs_high();
}

static void cmd2(uint8_t c, uint8_t d0, uint8_t d1) {
    cs_low();
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, c);
    dc_data();
    uint8_t d[2] = { d0, d1 };
    hal_spi_write(LCD_SPI_BUS, d, 2);
    cs_high();
}

static void cmd3(uint8_t c, uint8_t d0, uint8_t d1, uint8_t d2) {
    cs_low();
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, c);
    dc_data();
    uint8_t d[3] = { d0, d1, d2 };
    hal_spi_write(LCD_SPI_BUS, d, 3);
    cs_high();
}

static void cmd_buf(uint8_t c, const uint8_t *buf, size_t len) {
    cs_low();
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, c);
    dc_data();
    hal_spi_write(LCD_SPI_BUS, buf, len);
    cs_high();
}

static void hw_reset(void) {
    hal_gpio_write(LCD_PIN_RST, true);
    hal_delay_ms(50);
    hal_gpio_write(LCD_PIN_RST, false);
    hal_delay_ms(50);
    hal_gpio_write(LCD_PIN_RST, true);
    hal_delay_ms(100);
}

/* Canonical JLX256160G/ST75256 init sequence.
 * Values match the JLX reference C code byte for byte.
 * All multi-byte commands now hold CS low across cmd+data via cmdN(). */
static void st75256_init_seq(void) {
    cmd0(0x30);                     /* EXT=0 */
    cmd0(0x94);                     /* sleep out */

    cmd0(0x31);                     /* EXT=1 */
    cmd1(0xD7, 0x9F);               /* disable auto read */
    cmd3(0x32, 0x00, 0x01, 0x04);   /* analog circuit set, bias 1/12 */

    cs_low();                        /* gray level table (16 entries, 4 per level) */
    dc_cmd();
    hal_spi_write_byte(LCD_SPI_BUS, 0x20);
    dc_data();
    {
        /* Widely spaced so the 4 native gray levels are visually distinct.
         * Group of 4 entries per level, level 0 = lightest, level 3 = darkest. */
        static const uint8_t gl[16] = {
            0x00, 0x00, 0x00, 0x00,   /* level 0 — near white   */
            0x0A, 0x0A, 0x0A, 0x0A,   /* level 1 — ~33%        */
            0x14, 0x14, 0x14, 0x14,   /* level 2 — ~66%        */
            0x1F, 0x1F, 0x1F, 0x1F    /* level 3 — full black  */
        };
        hal_spi_write(LCD_SPI_BUS, gl, 16);
    }
    cs_high();

    cmd0(0x30);                     /* EXT=0 */
    cmd2(0x75, 0x00, 0x14);         /* page addr 0..0x14 (canonical value) */
    cmd2(0x15, 0x00, 0xFF);         /* column addr 0..255 */
    cmd2(0xBC, 0x00, 0xA6);         /* data scan direction */
    cmd3(0xCA, 0x00, 0x9F, 0x20);   /* display control: duty=160, n-line off */
    cmd1(0xF0, 0x11);               /* display mode: 4-level gray */
    cmd2(0x81, LCD_VOP_LOW, LCD_VOP_HIGH);  /* set Vop from board config */
    cmd1(0x20, 0x0B);               /* power control: booster + reg + follower ON */
    hal_delay_ms(100);
    cmd0(0xAF);                     /* display ON */
}

void lcd_init(void) {
    hal_gpio_init(LCD_PIN_CS,  HAL_GPIO_DIR_OUT);
    hal_gpio_init(LCD_PIN_DC,  HAL_GPIO_DIR_OUT);
    hal_gpio_init(LCD_PIN_RST, HAL_GPIO_DIR_OUT);
#if LCD_HAS_BL
    hal_gpio_init(LCD_PIN_BL,  HAL_GPIO_DIR_OUT);
#endif

    cs_high();

    const hal_spi_config_t cfg = {
        .bus         = LCD_SPI_BUS,
        .baudrate_hz = LCD_SPI_BAUD_HZ,
        .pin_sck     = LCD_PIN_SCK,
        .pin_mosi    = LCD_PIN_MOSI,
        .pin_miso    = 0xFF,
    };
    hal_spi_init(&cfg);

    hw_reset();
    st75256_init_seq();

    lcd_clear();
    lcd_flush();
    lcd_backlight(true);
}

void lcd_display_on(void)  { cmd0(0xAF); }
void lcd_display_off(void) { cmd0(0xAE); }

void lcd_set_contrast(uint8_t value) {
    cmd2(0x81, value, LCD_VOP_HIGH);
}

void lcd_backlight(bool on) {
#if LCD_HAS_BL
  #if LCD_BL_ACTIVE_HIGH
    hal_gpio_write(LCD_PIN_BL, on);
  #else
    hal_gpio_write(LCD_PIN_BL, !on);
  #endif
#else
    (void)on;
#endif
}

/* 4-gray framebuffer layout (chip-native):
 *   Each chip page = 4 rows. 40 pages total for 160 rows.
 *   Each (page, col) is one byte holding 4 pixels packed 2 bits per pixel.
 *   Bit positions within the byte (MSB-top): pixel 0 uses [7:6], pixel 1 [5:4],
 *   pixel 2 [3:2], pixel 3 [1:0]. Byte index = page * LCD_WIDTH + col.
 */

static inline uint8_t color_byte(uint8_t color) {
    const uint8_t c = color & 0x03u;
    return (uint8_t)((c << 6) | (c << 4) | (c << 2) | c);
}

void lcd_clear(void) {
    memset(s_fb, 0x00, sizeof(s_fb));
}

void lcd_fill(uint8_t color) {
    memset(s_fb, color_byte(color), LCD_FB_SIZE);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    const uint8_t  page    = (uint8_t)(y >> 2);            /* 4 rows per page */
    const uint8_t  in_page = (uint8_t)(y & 3u);            /* 0..3 within page */
    const uint8_t  shift   = (uint8_t)((3u - in_page) * 2u); /* MSB-top */
    const uint32_t idx     = (uint32_t)page * LCD_WIDTH + x;
    const uint8_t  mask    = (uint8_t)(0x03u << shift);

    s_fb[idx] = (uint8_t)((s_fb[idx] & (uint8_t)~mask) | ((color & 0x03u) << shift));
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    for (uint16_t yy = 0; yy < h; ++yy) {
        for (uint16_t xx = 0; xx < w; ++xx) {
            lcd_draw_pixel((uint16_t)(x + xx), (uint16_t)(y + yy), color);
        }
    }
}

static void set_window_full_page(uint8_t page) {
    cmd2(0x75, page, (uint8_t)(LCD_PAGES - 1));
    cmd2(0x15, 0x00, (uint8_t)(LCD_WIDTH - 1));
}

void lcd_flush(void) {
    for (uint8_t page = 0; page < LCD_PAGES; ++page) {
        set_window_full_page(page);
        cmd_buf(0x5C, &s_fb[(uint32_t)page * LCD_WIDTH], LCD_WIDTH);
    }
}

void lcd_test_fill_raw(uint8_t value) {
    static uint8_t row[LCD_WIDTH];
    for (uint16_t i = 0; i < LCD_WIDTH; ++i) row[i] = value;

    for (uint8_t page = 0; page < LCD_PAGES; ++page) {
        set_window_full_page(page);
        cmd_buf(0x5C, row, LCD_WIDTH);
    }
}

void lcd_test_fill_4gray(uint8_t color) {
    const uint8_t b = color_byte(color);

    static uint8_t row[LCD_WIDTH];
    for (uint16_t i = 0; i < LCD_WIDTH; ++i) row[i] = b;

    for (uint8_t page = 0; page < LCD_PAGES; ++page) {
        set_window_full_page(page);
        cmd_buf(0x5C, row, LCD_WIDTH);
    }
}

uint8_t *lcd_framebuffer(void) { return s_fb; }
