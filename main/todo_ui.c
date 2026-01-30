/**
 * @file todo_ui.c
 * @brief TODO列表UI实现
 */

#include "todo_ui.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "todo_client.h"

LV_FONT_DECLARE(lv_font_chinese_14);

static const char *TAG = "todo_ui";

static lv_obj_t *main_screen = NULL;
static lv_obj_t *title_label = NULL;
static lv_obj_t *scroll_container = NULL;
static lv_obj_t *todo_items[MAX_TODOS] = {NULL};
static lv_obj_t *todo_title_labels[MAX_TODOS] = {NULL};
static lv_obj_t *todo_deadline_labels[MAX_TODOS] = {NULL};
static lv_obj_t *loading_label = NULL;
static lv_obj_t *detail_popup = NULL;
static lv_obj_t *footer_bar = NULL;
static lv_obj_t *time_label = NULL;
static lv_timer_t *time_timer = NULL;

static todo_list_t current_todo_list;

static bool long_press_triggered = false;
static bool header_refresh_requested = false;

static bool click_processing = false;
static uint32_t last_click_time = 0;
#define CLICK_DEBOUNCE_MS 500 
#define FOOTER_HEIGHT 40 // 底栏高度

#define COLOR_BACKGROUND    lv_color_hex(0xF5F5F5) // 背景色
#define COLOR_PRIMARY       lv_color_make(174, 173, 227) // 主题色
#define COLOR_TEXT          lv_color_hex(0x212121) // 文字色
#define COLOR_TEXT_GRAY     lv_color_hex(0x9E9E9E) // 灰色文字色
#define COLOR_COMPLETED     lv_color_make(213, 212, 236) // 已完成背景色
#define COLOR_PENDING       lv_color_hex(0xFFFFFF) // 未完成背景色

/**
 * @brief 更新时间显示的定时器回调
 */
static void update_time_cb(lv_timer_t *timer)
{
    if (time_label == NULL) {
        return;
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%02d-%02d  %02d:%02d:%02d", 
             timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    lv_label_set_text(time_label, time_str);
}

/**
 * @brief 顶栏点击事件回调（手动刷新TODO）
 */
static void header_clicked_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "顶栏被点击，触发手动刷新请求");
    header_refresh_requested = true;
    todo_ui_show_loading(true);
}

/**
 * @brief 关闭详细信息弹窗（点击背景遮罩）
 */
static void close_detail_popup_bg(lv_event_t *e)
{
    (void)e;  // 避免未使用参数警告
    if (detail_popup != NULL) {
        lv_obj_del(detail_popup);
        detail_popup = NULL;
    }
}

/**
 * @brief TODO项点击事件回调（短按：切换状态）
 */
static void todo_item_clicked_cb(lv_event_t *e)
{
    if (long_press_triggered) {
        long_press_triggered = false;
        ESP_LOGI(TAG, "忽略长按后的点击事件");
        return;
    }
    
    if (click_processing) {
        ESP_LOGI(TAG, "点击正在处理中，忽略重复点击");
        return;
    }
    
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_click_time < CLICK_DEBOUNCE_MS) {
        ESP_LOGI(TAG, "点击间隔过短 (%lu ms)，忽略抖动", now - last_click_time);
        return;
    }
    
    lv_obj_t *item = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    
    if (index < current_todo_list.count) {
        const char *todo_id = current_todo_list.items[index].id;
        const char *list_id = current_todo_list.items[index].listId;
        bool current_status = current_todo_list.items[index].is_completed;
        bool new_status = !current_status;
        
        ESP_LOGI(TAG, "TODO[%d] 点击，切换状态: %s -> %s (listId: %s)", 
                 index, current_status ? "完成" : "未完成", new_status ? "完成" : "未完成", list_id);
        
        click_processing = true;
        last_click_time = now;
        
        esp_err_t ret = todo_client_set_completed(todo_id, list_id, new_status);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "服务器状态更新成功");
            current_todo_list.items[index].is_completed = new_status;
            
            if (new_status) {
                lv_obj_set_style_bg_color(item, COLOR_COMPLETED, 0);
                lv_obj_set_style_text_color(todo_title_labels[index], COLOR_TEXT_GRAY, 0);
                lv_obj_set_style_text_decor(todo_title_labels[index], LV_TEXT_DECOR_STRIKETHROUGH, 0);
                if (todo_deadline_labels[index]) {
                    lv_obj_set_style_text_color(todo_deadline_labels[index], COLOR_TEXT_GRAY, 0);
                }
            } else {
                lv_obj_set_style_bg_color(item, COLOR_PENDING, 0);
                lv_obj_set_style_text_color(todo_title_labels[index], COLOR_TEXT, 0);
                lv_obj_set_style_text_decor(todo_title_labels[index], LV_TEXT_DECOR_NONE, 0);
                if (todo_deadline_labels[index]) {
                    lv_obj_set_style_text_color(todo_deadline_labels[index], COLOR_TEXT_GRAY, 0);
                }
            }
        } else {
            ESP_LOGE(TAG, "服务器状态更新失败");
        }
        
        click_processing = false;
    }
}

/**
 * @brief TODO项长按事件回调（长按：显示详细信息）
 */
static void todo_item_long_pressed_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    
    if (index >= current_todo_list.count) {
        return;
    }
    
    long_press_triggered = true;
    
    ESP_LOGI(TAG, "TODO[%d] 长按，显示详细信息", index);
    
    if (detail_popup != NULL) {
        lv_obj_del(detail_popup);
    }
    
    lv_obj_t *bg_mask = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg_mask, 240, 320);
    lv_obj_set_pos(bg_mask, 0, 0);
    lv_obj_set_style_bg_color(bg_mask, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg_mask, LV_OPA_50, 0);
    lv_obj_set_style_border_width(bg_mask, 0, 0);
    lv_obj_set_style_radius(bg_mask, 0, 0);  // 背景无圆角
    lv_obj_clear_flag(bg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bg_mask, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bg_mask, close_detail_popup_bg, LV_EVENT_CLICKED, NULL);
    
    detail_popup = lv_obj_create(bg_mask);
    lv_obj_set_size(detail_popup, 200, 150);
    lv_obj_center(detail_popup);
    lv_obj_set_style_bg_color(detail_popup, lv_color_white(), 0);
    lv_obj_set_style_border_color(detail_popup, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(detail_popup, 3, 0);
    lv_obj_set_style_radius(detail_popup, 10, 0);
    lv_obj_set_style_shadow_width(detail_popup, 20, 0);
    lv_obj_set_style_shadow_opa(detail_popup, LV_OPA_30, 0);
    lv_obj_clear_flag(detail_popup, LV_OBJ_FLAG_CLICKABLE);  // 弹窗内容不响应点击
    
    lv_obj_t *title = lv_label_create(detail_popup);
    lv_label_set_text(title, current_todo_list.items[index].title);
    lv_obj_set_style_text_font(title, &lv_font_chinese_14, 0);
    lv_obj_set_style_text_color(title, COLOR_PRIMARY, 0);
    lv_obj_set_pos(title, 10, 10);
    lv_obj_set_width(title, 180);
    
    if (strlen(current_todo_list.items[index].body) > 0) {
        lv_obj_t *body = lv_label_create(detail_popup);
        lv_label_set_text(body, current_todo_list.items[index].body);
        lv_obj_set_style_text_font(body, &lv_font_chinese_14, 0);  // 改用中文字体
        lv_obj_set_style_text_color(body, COLOR_TEXT, 0);
        lv_obj_set_pos(body, 10, 40);
        lv_obj_set_width(body, 180);
        lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    }
    
    detail_popup = bg_mask;
}

esp_err_t todo_ui_init(void)
{
    ESP_LOGI(TAG, "初始化TODO UI");
    
    main_screen = lv_scr_act();
    lv_obj_set_style_bg_color(main_screen, COLOR_BACKGROUND, 0);
    
    lv_obj_t *header = lv_obj_create(main_screen);
    lv_obj_set_size(header, 240, 40);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);  // 标题栏无圆角
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(header, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(header, header_clicked_cb, LV_EVENT_CLICKED, NULL);
    
    title_label = lv_label_create(header);
    lv_label_set_text(title_label, "待办事项");
    lv_obj_set_style_text_font(title_label, &lv_font_chinese_14, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    scroll_container = lv_obj_create(main_screen);
    lv_obj_set_size(scroll_container, 240, 320 - 40 - FOOTER_HEIGHT);
    lv_obj_set_pos(scroll_container, 0, 40);
    lv_obj_set_style_bg_color(scroll_container, COLOR_BACKGROUND, 0);
    lv_obj_set_style_border_width(scroll_container, 0, 0);
    lv_obj_set_style_radius(scroll_container, 0, 0);
    lv_obj_set_style_pad_all(scroll_container, 5, 0);
    lv_obj_set_style_pad_row(scroll_container, 10, 0);  // 行间距
    lv_obj_set_flex_flow(scroll_container, LV_FLEX_FLOW_COLUMN);  // 垂直排列
    lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER);  // 只允许垂直滚动
    lv_obj_set_scrollbar_mode(scroll_container, LV_SCROLLBAR_MODE_AUTO);  // 自动显示滚动条

    for (int i = 0; i < MAX_TODOS; i++) {
        todo_items[i] = lv_obj_create(scroll_container);
        lv_obj_set_size(todo_items[i], 220, 65);
        lv_obj_set_style_bg_color(todo_items[i], COLOR_PENDING, 0);
        lv_obj_set_style_border_color(todo_items[i], lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_border_width(todo_items[i], 2, 0);
        lv_obj_set_style_radius(todo_items[i], 12, 0);  // 圆角
        lv_obj_set_style_pad_all(todo_items[i], 10, 0);
        lv_obj_set_style_shadow_width(todo_items[i], 8, 0);  // 阴影
        lv_obj_set_style_shadow_opa(todo_items[i], LV_OPA_20, 0);
        lv_obj_clear_flag(todo_items[i], LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_add_flag(todo_items[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(todo_items[i], todo_item_clicked_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(todo_items[i], todo_item_long_pressed_cb, LV_EVENT_LONG_PRESSED, (void*)(intptr_t)i);
        
        todo_title_labels[i] = lv_label_create(todo_items[i]);
        lv_label_set_text(todo_title_labels[i], "");
        lv_obj_set_style_text_font(todo_title_labels[i], &lv_font_chinese_14, 0);
        lv_obj_set_style_text_color(todo_title_labels[i], COLOR_TEXT, 0);
        lv_obj_set_pos(todo_title_labels[i], 0, 0);
        lv_obj_set_width(todo_title_labels[i], 200);
        
        todo_deadline_labels[i] = lv_label_create(todo_items[i]);
        lv_label_set_text(todo_deadline_labels[i], "");
        lv_obj_set_style_text_font(todo_deadline_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(todo_deadline_labels[i], COLOR_TEXT_GRAY, 0);
        lv_obj_set_pos(todo_deadline_labels[i], 0, 22);
        lv_obj_set_width(todo_deadline_labels[i], 200);
        
        lv_obj_add_flag(todo_items[i], LV_OBJ_FLAG_HIDDEN);
    }

    loading_label = lv_label_create(main_screen);
    lv_label_set_text(loading_label, "加载中...");
    lv_obj_set_style_text_font(loading_label, &lv_font_chinese_14, 0);
    lv_obj_set_style_text_color(loading_label, COLOR_PRIMARY, 0);
    lv_obj_set_pos(loading_label, 90, 150);  // 居中位置
    
    footer_bar = lv_obj_create(main_screen);
    lv_obj_set_size(footer_bar, 240, FOOTER_HEIGHT);
    lv_obj_set_pos(footer_bar, 0, 320 - FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer_bar, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(footer_bar, 0, 0);
    lv_obj_set_style_radius(footer_bar, 0, 0);
    lv_obj_set_style_pad_all(footer_bar, 0, 0);
    lv_obj_clear_flag(footer_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    time_label = lv_label_create(footer_bar);
    lv_label_set_text(time_label, "01-29  00:00:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(time_label, 1, 0);  // 增加字间距
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    
    time_timer = lv_timer_create(update_time_cb, 1000, NULL);
    update_time_cb(NULL);
    
    ESP_LOGI(TAG, "TODO UI初始化完成");
    return ESP_OK;
}

void todo_ui_update(const todo_list_t *list)
{
    if (list == NULL) {
        return;
    }
    
    memcpy(&current_todo_list, list, sizeof(todo_list_t));
    
    ESP_LOGI(TAG, "更新UI，TODO数量: %d", list->count);
    
    lv_obj_add_flag(loading_label, LV_OBJ_FLAG_HIDDEN);
    
    int count = list->count > MAX_TODOS ? MAX_TODOS : list->count;
    
    for (int i = 0; i < MAX_TODOS; i++) {
        lv_obj_add_flag(todo_items[i], LV_OBJ_FLAG_HIDDEN);
    }
    
    for (int i = 0; i < count && i < MAX_TODOS; i++) {
        lv_obj_clear_flag(todo_items[i], LV_OBJ_FLAG_HIDDEN);
        
        lv_label_set_text(todo_title_labels[i], list->items[i].title);
        
        if (strlen(list->items[i].last_modified_date) >= 16) {
            char datetime_str[20] = {0};
            snprintf(datetime_str, sizeof(datetime_str), "%.2s-%.2s %.2s:%.2s",
                     &list->items[i].last_modified_date[5],   // 月
                     &list->items[i].last_modified_date[8],   // 日
                     &list->items[i].last_modified_date[11],  // 时
                     &list->items[i].last_modified_date[14]); // 分
            lv_label_set_text(todo_deadline_labels[i], datetime_str);
        } else {
            lv_label_set_text(todo_deadline_labels[i], "");
        }
        
        if (list->items[i].is_completed) {
            lv_obj_set_style_bg_color(todo_items[i], COLOR_COMPLETED, 0);
            lv_obj_set_style_text_color(todo_title_labels[i], COLOR_TEXT_GRAY, 0);
            lv_obj_set_style_text_decor(todo_title_labels[i], LV_TEXT_DECOR_STRIKETHROUGH, 0);
            lv_obj_set_style_text_color(todo_deadline_labels[i], COLOR_TEXT_GRAY, 0);
        } else {
            lv_obj_set_style_bg_color(todo_items[i], COLOR_PENDING, 0);
            lv_obj_set_style_text_color(todo_title_labels[i], COLOR_TEXT, 0);
            lv_obj_set_style_text_decor(todo_title_labels[i], LV_TEXT_DECOR_NONE, 0);
            lv_obj_set_style_text_color(todo_deadline_labels[i], COLOR_TEXT_GRAY, 0);
        }
    }
}

void todo_ui_show_loading(bool loading)
{
    if (loading_label == NULL) {
        return;
    }
    
    if (loading) {
        lv_obj_clear_flag(loading_label, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < MAX_TODOS; i++) {
            lv_obj_add_flag(todo_items[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(loading_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void todo_ui_show_wifi_status(bool connected, const char *ip)
{
    (void)connected;
    (void)ip;
}

bool todo_ui_take_refresh_request(void)
{
    bool requested = header_refresh_requested;
    header_refresh_requested = false;
    return requested;
}
