/**
 * @file todo_client.h
 * @brief TODO HTTP客户端
 */

#ifndef TODO_CLIENT_H
#define TODO_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO最大数量
#define MAX_TODOS 6

#define TODO_TITLE_MAX_LEN 64
#define TODO_BODY_MAX_LEN 128
#define TODO_ID_MAX_LEN 256
#define TODO_LIST_ID_MAX_LEN 256
#define TODO_DATE_MAX_LEN 32

/**
 * @brief TODO项结构
 */
typedef struct {
    char id[TODO_ID_MAX_LEN];
    char listId[TODO_LIST_ID_MAX_LEN];
    char title[TODO_TITLE_MAX_LEN];
    char body[TODO_BODY_MAX_LEN];
    bool is_completed;
    char importance[16];
    char last_modified_date[TODO_DATE_MAX_LEN];
} todo_item_t;

/**
 * @brief TODO列表结构
 */
typedef struct {
    todo_item_t items[MAX_TODOS];
    int count;
    char default_listId[TODO_LIST_ID_MAX_LEN];
} todo_list_t;

/**
 * @brief 初始化TODO客户端
 * @param server_url 服务器地址，例如 "http://192.168.1.100:5000"
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t todo_client_init(const char *server_url);

/**
 * @brief 从服务器获取TODO列表
 * @param list 用于存储TODO列表的结构
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t todo_client_get_list(todo_list_t *list);

/**
 * @brief 标记TODO为完成/未完成
 * @param todo_id TODO的ID
 * @param list_id 列表ID（用于Graph API）
 * @param completed true表示完成，false表示未完成
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t todo_client_set_completed(const char *todo_id, const char *list_id, bool completed);

/**
 * @brief 创建新TODO
 * @param title 标题
 * @param body 描述
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t todo_client_create(const char *title, const char *body);

#ifdef __cplusplus
}
#endif

#endif
