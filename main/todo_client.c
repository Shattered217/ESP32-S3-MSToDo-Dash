/**
 * @file todo_client.c
 * @brief TODO HTTP客户端实现
 */

#include "todo_client.h"
#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "todo_client";
static char server_url[128] = {0};

#define API_KEY "esp32-todo-secret-key-2025"

#define HTTP_BUFFER_SIZE 4096
static char http_buffer[HTTP_BUFFER_SIZE];
static int http_buffer_index = 0;

/**
 * @brief HTTP事件处理函数
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (http_buffer_index + evt->data_len < HTTP_BUFFER_SIZE) {
                memcpy(http_buffer + http_buffer_index, evt->data, evt->data_len);
                http_buffer_index += evt->data_len;
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t todo_client_init(const char *url)
{
    if (url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(server_url, url, sizeof(server_url) - 1);
    ESP_LOGI(TAG, "TODO客户端初始化，服务器: %s", server_url);
    
    return ESP_OK;
}

esp_err_t todo_client_get_list(todo_list_t *list)
{
    if (list == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(list, 0, sizeof(todo_list_t));
    http_buffer_index = 0;
    memset(http_buffer, 0, HTTP_BUFFER_SIZE);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/todos?limit=%d", server_url, MAX_TODOS);
    
    ESP_LOGI(TAG, "获取TODO列表: %s", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "X-API-Key", API_KEY);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP状态码 = %d, 响应长度 = %d", status, http_buffer_index);
        
        if (status == 200) {
            http_buffer[http_buffer_index] = '\0';
            cJSON *root = cJSON_Parse(http_buffer);
            
            if (root != NULL) {
                cJSON *root_listId = cJSON_GetObjectItem(root, "listId");
                if (cJSON_IsString(root_listId)) {
                    strncpy(list->default_listId, root_listId->valuestring, TODO_LIST_ID_MAX_LEN - 1);
                    ESP_LOGI(TAG, "默认列表ID: %s", list->default_listId);
                }
                
                cJSON *value_array = cJSON_GetObjectItem(root, "value");
                if (cJSON_IsArray(value_array)) {
                    int array_size = cJSON_GetArraySize(value_array);
                    list->count = (array_size > MAX_TODOS) ? MAX_TODOS : array_size;
                    
                    for (int i = 0; i < list->count; i++) {
                        cJSON *item = cJSON_GetArrayItem(value_array, i);
                        if (item) {
                            cJSON *id = cJSON_GetObjectItem(item, "id");
                            if (cJSON_IsString(id)) {
                                strncpy(list->items[i].id, id->valuestring, TODO_ID_MAX_LEN - 1);
                            }
                            
                            cJSON *listId = cJSON_GetObjectItem(item, "listId");
                            if (cJSON_IsString(listId)) {
                                strncpy(list->items[i].listId, listId->valuestring, TODO_LIST_ID_MAX_LEN - 1);
                            } else if (strlen(list->default_listId) > 0) {
                                strncpy(list->items[i].listId, list->default_listId, TODO_LIST_ID_MAX_LEN - 1);
                            }
                            
                            cJSON *title = cJSON_GetObjectItem(item, "title");
                            if (cJSON_IsString(title)) {
                                strncpy(list->items[i].title, title->valuestring, TODO_TITLE_MAX_LEN - 1);
                            }
                            
                            cJSON *body = cJSON_GetObjectItem(item, "body");
                            if (cJSON_IsString(body)) {
                                strncpy(list->items[i].body, body->valuestring, TODO_BODY_MAX_LEN - 1);
                            }
                            
                            cJSON *is_completed = cJSON_GetObjectItem(item, "isCompleted");
                            if (cJSON_IsBool(is_completed)) {
                                list->items[i].is_completed = cJSON_IsTrue(is_completed);
                            }
                            
                            cJSON *importance = cJSON_GetObjectItem(item, "importance");
                            if (cJSON_IsString(importance)) {
                                strncpy(list->items[i].importance, importance->valuestring, 15);
                            }
                            
                            cJSON *last_modified = cJSON_GetObjectItem(item, "lastModifiedDateTime");
                            if (cJSON_IsString(last_modified)) {
                                strncpy(list->items[i].last_modified_date, last_modified->valuestring, TODO_DATE_MAX_LEN - 1);
                            }
                            
                            ESP_LOGI(TAG, "TODO[%d]: %s - %s (listId: %s)", i, list->items[i].title, 
                                    list->items[i].is_completed ? "已完成" : "未完成", list->items[i].listId);
                        }
                    }
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "JSON解析失败");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP请求失败，状态码: %d", status);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP请求执行失败: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t todo_client_set_completed(const char *todo_id, const char *list_id, bool completed)
{
    if (todo_id == NULL || list_id == NULL) {
        ESP_LOGE(TAG, "todo_id或list_id为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    char url[512];
    snprintf(url, sizeof(url), "%s/api/todos/%s/%s", 
             server_url, todo_id, completed ? "complete" : "uncomplete");
    
    ESP_LOGI(TAG, "设置完成状态: %s (listId: %s)", url, list_id);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "listId", list_id);
    char *json_str = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-API-Key", API_KEY);
    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "状态码 = %d", status);
        if (status != 200) {
            err = ESP_FAIL;
        }
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_str);
    
    return err;
}

esp_err_t todo_client_create(const char *title, const char *body)
{
    if (title == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/todos", server_url);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "title", title);
    if (body) {
        cJSON_AddStringToObject(root, "body", body);
    }
    
    char *json_str = cJSON_PrintUnformatted(root);
    
    ESP_LOGI(TAG, "创建TODO: %s", json_str);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-API-Key", API_KEY);
    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "状态码 = %d", status);
        if (status != 201 && status != 200) {
            err = ESP_FAIL;
        }
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_str);
    
    return err;
}
