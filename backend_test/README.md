# TODO API 测试后端

这是一个简单的Flask服务器，用于测试ESP32的TODO应用。

## 安装依赖

```bash
pip install -r requirements.txt
```

## 运行服务器

```bash
python app.py
```

服务器将在 `http://0.0.0.0:5000` 上运行

## API 端点

### 获取所有TODO项

```http
GET /api/todos
```

返回示例：

```json
{
  "value": [
    {
      "id": "1",
      "title": "完成ESP32项目",
      "body": "实现TODO列表显示功能",
      "status": "notStarted",
      "importance": "high",
      "createdDateTime": "2026-01-29T10:00:00Z",
      "lastModifiedDateTime": "2026-01-29T10:00:00Z",
      "isCompleted": false
    }
  ],
  "count": 1
}
```

### 获取指定TODO项

```http
GET /api/todos/{id}
```

### 创建新TODO项

```http
POST /api/todos
Content-Type: application/json

{
  "title": "新任务",
  "body": "任务描述",
  "importance": "normal"
}
```

### 更新TODO项

```http
PUT /api/todos/{id}
Content-Type: application/json

{
  "title": "更新的标题",
  "isCompleted": true
}
```

### 删除TODO项

```http
DELETE /api/todos/{id}
```

### 标记为完成

```http
POST /api/todos/{id}/complete
```

### 标记为未完成

```http
POST /api/todos/{id}/uncomplete
```

### 获取统计信息

```http
GET /api/stats
```

## 数据格式说明

### TODO项字段

- `id`: 唯一标识符
- `title`: 标题（必填）
- `body`: 详细描述（可选）
- `status`: 状态
  - `notStarted`: 未开始
  - `inProgress`: 进行中
  - `completed`: 已完成
- `importance`: 重要性
  - `low`: 低
  - `normal`: 普通
  - `high`: 高
- `isCompleted`: 是否完成（布尔值）
- `createdDateTime`: 创建时间（ISO 8601格式）
- `lastModifiedDateTime`: 最后修改时间（ISO 8601格式）

## 测试命令

使用curl测试：

```bash
# 获取所有TODO
curl http://localhost:5000/api/todos

# 创建新TODO
curl -X POST http://localhost:5000/api/todos \
  -H "Content-Type: application/json" \
  -d '{"title":"测试任务","body":"这是一个测试"}'

# 标记完成
curl -X POST http://localhost:5000/api/todos/1/complete

# 获取统计
curl http://localhost:5000/api/stats
```

## 注意事项

1. 这是一个内存数据库，服务器重启后数据会丢失
2. 默认包含3个示例TODO项
3. 支持CORS，可以从ESP32直接访问
4. 数据格式参考了微软TODO API的结构
