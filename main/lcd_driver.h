#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>

#define LCD_WIDTH   240
#define LCD_HEIGHT  320

#define LCD_PIN_MOSI    11
#define LCD_PIN_CLK     12
#define LCD_PIN_CS      10
#define LCD_PIN_DC      13
#define LCD_PIN_RST     14
#define LCD_BL_PIN      9

#define LCD_SPI_HOST    SPI2_HOST
#define LCD_PIXEL_CLK   40000000  

// RGB565
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFD20
#define COLOR_PURPLE    0x8010
#define COLOR_GRAY      0x8410

void lcd_init(void);
void lcd_fill_screen(uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcd_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcd_set_backlight(uint8_t brightness);

#endif 
