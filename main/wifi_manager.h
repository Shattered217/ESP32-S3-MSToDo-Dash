/**
 * @file wifi_manager.h
 * @brief WiFi连接管理模块
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化并连接WiFi
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief 检查WiFi是否已连接
 * @return true 已连接, false 未连接
 */
bool wifi_is_connected(void);

/**
 * @brief 获取本地IP地址字符串
 * @param ip_str 存储IP地址的缓冲区
 * @param len 缓冲区长度
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t wifi_get_ip_string(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
