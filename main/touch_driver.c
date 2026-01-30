/**
 * @file touch_driver.c
 * @brief CST328触摸驱动实现
 */

#include "touch_driver.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

static const char *TAG = "touch_driver";

esp_lcd_touch_handle_t tp = NULL;

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t touch_i2c_init(void)
{
    ESP_LOGI(TAG, "初始化I2C总线 (新API)");
    
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_TOUCH_MASTER_NUM,
        .sda_io_num = I2C_TOUCH_SDA_IO,
        .scl_io_num = I2C_TOUCH_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C总线创建失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C总线初始化完成 (总线句柄: %p)", i2c_bus_handle);
    return ESP_OK;
}

esp_err_t touch_cst328_init(void)
{
    ESP_LOGI(TAG, "初始化CST328触摸驱动");
    
    esp_err_t ret = touch_i2c_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags = {
            .disable_control_phase = 1,
        }
    };
    
    ESP_LOGI(TAG, "创建触摸IO设备 (地址: 0x%02x, 频率: %d Hz)", 
             ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS, I2C_MASTER_FREQ_HZ);
    
    ret = esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建触摸IO失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "触摸IO创建成功");
    
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 240,
        .y_max = 320,
        .rst_gpio_num = I2C_TOUCH_RST_IO,
        .int_gpio_num = I2C_TOUCH_INT_IO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    
    ret = esp_lcd_touch_new_i2c_cst328(tp_io_handle, &tp_cfg, &tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建触摸设备失败");
        return ret;
    }
    
    ESP_LOGI(TAG, "CST328触摸驱动初始化完成");
    return ESP_OK;
}
