## ESP32-S3 TODO 小工具

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)](#) [![Framework](https://img.shields.io/badge/Framework-ESP--IDF%206.0-green)](#) [![UI](https://img.shields.io/badge/UI-LVGL%208.3-purple)](#) [![Backend](https://img.shields.io/badge/Backend-Flask%20%2B%20Microsoft%20Graph-orange)](#)

一个基于 **ESP32-S3 + LVGL** 的待办事项小工具，界面显示跑在 240x320 LCD 上，通过 HTTP 与后端 **Flask** 服务交互，支持触摸操作、滚动列表、长按查看详情等功能。

### 功能特点

- **待办事项展示**
  - 显示来自服务器的 TODO 列表（当前最多 `MAX_TODOS = 6` 条）
  - 支持滚动查看任务（超过 3 条时自动出现滚动）
  - 任务卡片支持颜色区分完成/未完成状态
- **触摸交互**
  - 短按任务卡片：切换完成/未完成（立即同步到服务器）
  - 长按任务卡片：弹出半透明遮罩，显示任务详细内容
  - 点击遮罩空白区域关闭详情
- **顶部标题栏交互**
  - 顶栏显示标题“待办事项”
  - 点击顶栏：**立即向服务器刷新 TODO 列表**
  - 顶栏点击刷新会重置自动刷新计时
- **底部状态栏**
  - 显示当前日期时间，格式：`MM-DD  HH:MM:SS`
  - 使用 SNTP 网络对时（UTC+8）
- **自动刷新策略**
  - 上电后首次获取一次 TODO 列表
  - 之后每 **30 分钟** 自动从服务器刷新一次
  - 手动点击顶栏可随时触发刷新
- **HTTP API Key 鉴权**
  - 所有与 Flask 后端的请求都携带 `X-API-Key` 头
  - 简单安全地限制访问来源

---

### 目录结构简要说明

- `main/`
  - `main.c`  
    ESP32 应用入口：初始化 LCD/LVGL、WiFi、SNTP、TODO 客户端，主循环处理 LVGL 和 TODO 刷新逻辑。
  - `todo_ui.c` / `todo_ui.h`  
    使用 LVGL 实现的 UI 界面（标题栏、滚动列表、底栏时间、长按详情弹窗、顶栏点击刷新等）。
  - `todo_client.c` / `todo_client.h`  
    ESP32 侧 HTTP 客户端，负责与 Flask 后端交互（获取列表、切换完成状态、创建任务），基于 `esp_http_client` + `cJSON`。
  - `lvgl_driver.c` / `lvgl_driver.h`  
    LVGL 驱动封装，注册显示驱动与缓冲区。
  - `lcd_driver.c` / `lcd_driver.h` + `Vernon_ST7789T/`  
    ST7789T LCD 驱动，包含 RGB/BGR 配置、MADCTL 修正等。
  - `touch_driver.c` / `touch_driver.h` / `touch_cst328.c`  
    电容触摸屏 CST328 驱动，基于 ESP-IDF v6 新 I2C Master API。
  - `wifi_manager.c` / `wifi_manager.h`  
    WiFi STA 连接管理（连接到你的局域网）。
  - `lv_conf.h`  
    LVGL 配置文件，仅启用必须的字体（Montserrat 14/22 + 自定义中文字体）。
  - `lv_font_chinese_14.c`  
    自定义中文字体，用于标题、正文、提示文字显示中文。

---

### 硬件需求

- **微雪电子 2.8 寸 ESP32-S3 屏幕套件**  
  （集成 ESP32-S3、240x320 LCD 和电容触摸，项目已按该套件的引脚和屏幕参数适配）

> 更详细的原理图与引脚定义请参考微雪电子官方文档，本仓库中的 `main.c` 与 `touch_driver.h` 已经按该套件默认引脚进行配置。

---

### 软件环境

- ESP-IDF **v5.x / v6.x**（本工程按 v6 API 适配）
- Python 3.8+（用于运行 Flask 后端）
- UV（推荐）

---

### 后端（Flask）快速启动

> 本项目的 Flask 后端示例代码位于单独仓库：  
> [`MicrosoftToDo-Telegram-AIBot` `feature/esp32-flask-server`](https://github.com/Shattered217/MicrosoftToDo-Telegram-AIBot/tree/feature/esp32-flask-server)

1. 克隆后端仓库分支并进入目录：

   ```bash
   git clone -b feature/esp32-flask-server https://github.com/Shattered217/MicrosoftToDo-Telegram-AIBot.git
   cd MicrosoftToDo-Telegram-AIBot/esp32_server
   ```

2. 使用 uv 安装依赖并运行（推荐）：

   ```bash
   # 安装 uv（如尚未安装）
   pip install uv

   # 安装依赖
   uv sync
   ```

3. 参照 [`env_example.txt`](https://github.com/Shattered217/MicrosoftToDo-Telegram-AIBot/blob/feature/esp32-flask-server/env_example.txt)自定义配置后启动 Flask 服务：

   ```bash
   uv run python app.py
   ```

   请确认你的 IP，并在 ESP32 端配置为 `SERVER_URL`。以及放在公网的话请修改密钥。

---

### ESP32 端配置

#### 1. 配置服务器地址

在 `main.c` 中：

```c
#define SERVER_URL "http://192.168.101.224:5000"
```

请将 `192.168.101.224` 修改为 **你运行 Flask 后端的电脑 IP**。

#### 2. 配置 API Key

ESP32 端配置与 Flask 端一致的密钥（`todo_client.c`）：

```c
#define API_KEY "esp32-todo-secret-key-2025"
```

#### 3. WiFi 配置

在 `wifi_manager.c` 中配置你的 WiFi SSID / 密码

### HTTP 接口

- **获取任务列表**

  ```http
  GET /api/todos?limit=6
  X-API-Key: esp32-todo-secret-key-2025
  ```

  响应示例：

  ```json
  {
    "value": [
      {
        "id": "AQMkADAwATM3...",
        "listId": "AQMkADAwATM3...",
        "title": "任务标题",
        "body": "任务内容",
        "isCompleted": false,
        "importance": "normal",
        "lastModifiedDateTime": "2025-01-30T10:00:00Z"
      }
    ],
    "listId": "AQMkADAwATM3..." // 默认列表ID
  }
  ```

- **切换任务完成状态**

  ```http
  POST /api/todos/{完整任务ID}/complete
  X-API-Key: esp32-todo-secret-key-2025
  Content-Type: application/json

  {
    "listId": "对应的列表ID"
  }
  ```

  ESP32 端由 `todo_client_set_completed()` 负责构造并发送该请求。

---

### 如果这个项目对你有帮助 🙂

- **欢迎点一个 Star ⭐**，我很需要它 💖
- 也欢迎 Fork / 提 Issue，一起把这个 ESP32-S3 TODO 面板打磨得更好
