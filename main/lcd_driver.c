#include "lcd_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LCD";
static spi_device_handle_t spi_handle;

#define ST7789_SWRESET  0x01
#define ST7789_SLPOUT   0x11
#define ST7789_NORON    0x13
#define ST7789_INVON    0x21
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_MADCTL   0x36
#define ST7789_COLMOD   0x3A

static void lcd_send_cmd(uint8_t cmd) {
    gpio_set_level(LCD_PIN_DC, 0); 
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

static void lcd_send_data(uint8_t data) {
    gpio_set_level(LCD_PIN_DC, 1); 
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

static void lcd_send_data_array(const uint8_t *data, int len) {
    gpio_set_level(LCD_PIN_DC, 1); 
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    lcd_send_cmd(ST7789_CASET);
    lcd_send_data(x1 >> 8);
    lcd_send_data(x1 & 0xFF);
    lcd_send_data(x2 >> 8);
    lcd_send_data(x2 & 0xFF);

    lcd_send_cmd(ST7789_RASET);
    lcd_send_data(y1 >> 8);
    lcd_send_data(y1 & 0xFF);
    lcd_send_data(y2 >> 8);
    lcd_send_data(y2 & 0xFF);

    lcd_send_cmd(ST7789_RAMWR);
}

void lcd_init(void) {
    ESP_LOGI(TAG, "初始化LCD驱动");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC) | (1ULL << LCD_PIN_RST) | (1ULL << LCD_BL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = LCD_PIXEL_CLK,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_HOST, &devcfg, &spi_handle));

    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_send_cmd(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_send_cmd(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_send_cmd(ST7789_COLMOD);
    lcd_send_data(0x55); 

    lcd_send_cmd(ST7789_MADCTL);
    lcd_send_data(0x00);  

    lcd_send_cmd(ST7789_INVON);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_send_cmd(ST7789_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_send_cmd(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(120));

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LCD_BL_PIN,
        .duty = 255,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    ESP_LOGI(TAG, "LCD初始化完成");
}

void lcd_set_backlight(uint8_t brightness) {
    uint32_t duty = (brightness * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void lcd_fill_screen(uint16_t color) {
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    lcd_set_window(x, y, x, y);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    lcd_send_data_array(data, 2);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    lcd_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    
    gpio_set_level(LCD_PIN_DC, 1);
    
    const int buffer_size = 4096;
    uint8_t *buffer = malloc(buffer_size);
    if (buffer) {
        for (int i = 0; i < buffer_size; i += 2) {
            buffer[i] = color_high;
            buffer[i + 1] = color_low;
        }
        
        int total_pixels = w * h * 2;
        int sent = 0;
        while (sent < total_pixels) {
            int chunk = (total_pixels - sent) > buffer_size ? buffer_size : (total_pixels - sent);
            spi_transaction_t t = {
                .length = chunk * 8,
                .tx_buffer = buffer,
            };
            spi_device_polling_transmit(spi_handle, &t);
            sent += chunk;
        }
        free(buffer);
    }
}

void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    lcd_draw_line(x, y, x + w - 1, y, color);
    lcd_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    lcd_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    lcd_draw_line(x, y, x, y + h - 1, color);
}

void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        lcd_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int x = r;
    int y = 0;
    int err = 0;

    while (x >= y) {
        lcd_draw_pixel(x0 + x, y0 + y, color);
        lcd_draw_pixel(x0 + y, y0 + x, color);
        lcd_draw_pixel(x0 - y, y0 + x, color);
        lcd_draw_pixel(x0 - x, y0 + y, color);
        lcd_draw_pixel(x0 - x, y0 - y, color);
        lcd_draw_pixel(x0 - y, y0 - x, color);
        lcd_draw_pixel(x0 + y, y0 - x, color);
        lcd_draw_pixel(x0 + x, y0 - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void lcd_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                lcd_draw_pixel(x0 + x, y0 + y, color);
            }
        }
    }
}
