#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_sntp.h"
#include "Vernon_ST7789T/Vernon_ST7789T.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "lvgl_driver.h"
#include "wifi_manager.h"
#include "todo_client.h"
#include "todo_ui.h"

static const char *TAG = "TODO_APP";

#define SERVER_URL CONFIG_TODO_SERVER_URL

#define REFRESH_INTERVAL_MS (30 * 60 * 1000)

#define LCD_H_RES              240
#define LCD_V_RES              320
#define LCD_PIN_NUM_MOSI       45
#define LCD_PIN_NUM_CLK        40
#define LCD_PIN_NUM_CS         42
#define LCD_PIN_NUM_DC         41
#define LCD_PIN_NUM_RST        39
#define LCD_PIN_BL             5
#define LCD_SPI_HOST           SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

esp_lcd_panel_handle_t panel_handle = NULL;

static void lcd_backlight_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LCD_PIN_BL,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void lcd_backlight_on(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void lcd_init(void)
{
    ESP_LOGI(TAG, "Initialize LCD");

    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_NUM_DC,
        .cs_gpio_num = LCD_PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_st7789t_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789t(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // 修复文字镜像问题：设置X轴镜像
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    lcd_backlight_init();
    ESP_LOGI(TAG, "LCD initialized");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "ESP32-S3 TODO应用启动");
    ESP_LOGI(TAG, "=================================");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "初始化LCD...");
    lcd_init();
    LVGL_Init();
    
    ESP_LOGI(TAG, "创建TODO界面...");
    todo_ui_init();
    todo_ui_show_loading(true);
    
    vTaskDelay(pdMS_TO_TICKS(50));
    lv_timer_handler();
    lcd_backlight_on();
    
    ESP_LOGI(TAG, "连接WiFi...");
    ret = wifi_init_sta();
    
    if (ret == ESP_OK) {
        char ip_str[16];
        if (wifi_get_ip_string(ip_str, sizeof(ip_str)) == ESP_OK) {
            ESP_LOGI(TAG, "本机IP: %s", ip_str);
            todo_ui_show_wifi_status(true, ip_str);
        } else {
            todo_ui_show_wifi_status(true, NULL);
        }
        
        ESP_LOGI(TAG, "初始化SNTP时间同步...");
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_setservername(1, "cn.pool.ntp.org");
        esp_sntp_init();
        
        // 设置时区
        setenv("TZ", "CST-8", 1);
        tzset();
        
        int retry = 0;
        const int max_retry = 30;
        bool sync_completed = false;
        
        while (retry < max_retry) {
            sntp_sync_status_t status = esp_sntp_get_sync_status();
            if (status == SNTP_SYNC_STATUS_COMPLETED) {
                sync_completed = true;
                break;
            }
            if (retry % 5 == 0) {
                ESP_LOGI(TAG, "等待时间同步... (%d/%d秒)", retry + 1, max_retry);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            retry++;
        }
        
        if (sync_completed) {
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "时间同步成功: %04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            ESP_LOGW(TAG, "时间同步超时（等待了%d秒），SNTP将在后台继续同步", max_retry);
            ESP_LOGI(TAG, "时间显示将在同步完成后自动更新");
        }
        
        ESP_LOGI(TAG, "初始化TODO客户端...");
        ESP_LOGI(TAG, "服务器地址: %s", SERVER_URL);
        ESP_LOGW(TAG, "请确保修改SERVER_URL为您的电脑IP地址！");
        todo_client_init(SERVER_URL);
        
        static todo_list_t todo_list;
        if (todo_client_get_list(&todo_list) == ESP_OK) {
            ESP_LOGI(TAG, "成功获取TODO列表，共%d项", todo_list.count);
            todo_ui_update(&todo_list);
        } else {
            ESP_LOGE(TAG, "获取TODO列表失败");
        }
        
        todo_ui_show_loading(false);
        
        ESP_LOGI(TAG, "进入主循环...");
        uint32_t last_refresh = 0;
        
        while (1) {
            lv_timer_handler();
            
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if (todo_ui_take_refresh_request()) {
                ESP_LOGI(TAG, "手动刷新TODO列表");
                if (todo_client_get_list(&todo_list) == ESP_OK) {
                    todo_ui_update(&todo_list);
                    last_refresh = now;  // 重置自动刷新计时
                    todo_ui_show_loading(false);
                } else {
                    ESP_LOGE(TAG, "手动刷新TODO列表失败");
                    todo_ui_show_loading(false);
                }
            }
            
            if (now - last_refresh > REFRESH_INTERVAL_MS) {
                ESP_LOGI(TAG, "自动刷新TODO列表...");
                if (todo_client_get_list(&todo_list) == ESP_OK) {
                    todo_ui_update(&todo_list);
                    last_refresh = now;
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
    } else {
        ESP_LOGE(TAG, "WiFi连接失败！");
        todo_ui_show_wifi_status(false, NULL);
        todo_ui_show_loading(false);
        
        while (1) {
            lv_timer_handler();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
