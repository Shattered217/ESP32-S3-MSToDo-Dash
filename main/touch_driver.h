/**
 * @file touch_driver.h
 * @brief CST328触摸驱动
 */

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include "esp_lcd_touch.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_TOUCH_SCL_IO            3
#define I2C_TOUCH_SDA_IO            1
#define I2C_TOUCH_INT_IO            4
#define I2C_TOUCH_RST_IO            2
#define I2C_TOUCH_MASTER_NUM        I2C_NUM_1
#define I2C_MASTER_FREQ_HZ          400000

#define ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS (0x1A)

#define ESP_LCD_TOUCH_CST328_READ_Number_REG    (0xD005)
#define ESP_LCD_TOUCH_CST328_READ_XY_REG        (0xD000)
#define ESP_LCD_TOUCH_CST328_READ_Checksum_REG  (0x80FF)
#define ESP_LCD_TOUCH_CST328_CONFIG_REG         (0x8047)

#define CST328_REG_DEBUG_INFO_MODE              0xD101
#define CST328_REG_RESET_MODE                   0xD102
#define CST328_REG_DEBUG_RECALIBRATION_MODE     0xD104
#define CST328_REG_DEEP_SLEEP_MODE              0xD105
#define CST328_REG_DEBUG_POINT_MODE             0xD108
#define CST328_REG_NORMAL_MODE                  0xD109
#define CST328_REG_DEBUG_RAWDATA_MODE           0xD10A
#define CST328_REG_DEBUG_DIFF_MODE              0xD10D
#define CST328_REG_DEBUG_FACTORY_MODE           0xD119
#define CST328_REG_DEBUG_FACTORY_MODE_2         0xD120

#define CST328_REG_DEBUG_INFO_BOOT_TIME         0xD1FC
#define CST328_REG_DEBUG_INFO_RES_Y             0xD1FA
#define CST328_REG_DEBUG_INFO_RES_X             0xD1F8
#define CST328_REG_DEBUG_INFO_KEY_NUM           0xD1F7
#define CST328_REG_DEBUG_INFO_TP_NRX            0xD1F6
#define CST328_REG_DEBUG_INFO_TP_NTX            0xD1F4

#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

extern esp_lcd_touch_handle_t tp;

/**
 * @brief 初始化I2C总线
 * @return ESP_OK 成功
 */
esp_err_t touch_i2c_init(void);

/**
 * @brief 初始化CST328触摸驱动
 * @return ESP_OK 成功
 */
esp_err_t touch_cst328_init(void);

/**
 * @brief 创建CST328触摸设备
 */
esp_err_t esp_lcd_touch_new_i2c_cst328(const esp_lcd_panel_io_handle_t io, 
                                       const esp_lcd_touch_config_t *config, 
                                       esp_lcd_touch_handle_t *out_touch);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_DRIVER_H
