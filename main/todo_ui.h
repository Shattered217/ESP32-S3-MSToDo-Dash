/**
 * @file todo_ui.h
 * @brief TODO列表UI界面
 */

#ifndef TODO_UI_H
#define TODO_UI_H

#include "lvgl.h"
#include "todo_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化TODO UI
 * @return ESP_OK 成功
 */
esp_err_t todo_ui_init(void);

/**
 * @brief 更新TODO列表显示
 * @param list TODO列表数据
 */
void todo_ui_update(const todo_list_t *list);

/**
 * @brief 显示加载状态
 * @param loading true显示加载中，false隐藏
 */
void todo_ui_show_loading(bool loading);

/**
 * @brief 显示WiFi连接状态
 * @param connected true已连接，false未连接
 * @param ip_str IP地址字符串（可选）
 */
void todo_ui_show_wifi_status(bool connected, const char *ip_str);

/**
 * @brief 是否有来自UI的手动刷新请求（点击顶栏）
 *
 * 调用该函数会读取并清除内部标志，相当于“取走”这次刷新请求。
 * @return true 有刷新请求，false 无
 */
bool todo_ui_take_refresh_request(void);

#ifdef __cplusplus
}
#endif

#endif
