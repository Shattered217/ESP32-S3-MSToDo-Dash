"""
Flask TODO API 测试后端
模拟微软 TODO API 的基本功能
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
from datetime import datetime
from functools import wraps
import uuid

app = Flask(__name__)
CORS(app)  # 允许跨域访问

# API密钥配置
API_KEY = "esp32-todo-secret-key-2025"  # 修改为你自己的密钥

def require_api_key(f):
    """API密钥验证装饰器"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        api_key = request.headers.get('X-API-Key')
        if api_key != API_KEY:
            return jsonify({'error': 'Invalid or missing API key'}), 401
        return f(*args, **kwargs)
    return decorated_function

# 模拟数据存储（增加更多数据用于测试滚动）
todos = [
    {
        "id": "1",
        "title": "完成ESP32项目",
        "body": "实现TODO列表显示功能",
        "status": "notStarted",
        "importance": "high",
        "createdDateTime": "2026-01-29T10:00:00Z",
        "lastModifiedDateTime": "2026-01-29T10:00:00Z",
        "isCompleted": False
    },
    {
        "id": "2",
        "title": "学习LVGL",
        "body": "掌握LVGL界面设计",
        "status": "inProgress",
        "importance": "normal",
        "createdDateTime": "2026-01-29T11:00:00Z",
        "lastModifiedDateTime": "2026-01-29T11:00:00Z",
        "isCompleted": False
    },
    {
        "id": "3",
        "title": "测试WiFi连接",
        "body": "验证ESP32 WiFi功能",
        "status": "completed",
        "importance": "normal",
        "createdDateTime": "2026-01-29T09:00:00Z",
        "lastModifiedDateTime": "2026-01-29T12:00:00Z",
        "isCompleted": True
    },
    {
        "id": "4",
        "title": "编写文档",
        "body": "完善项目README文档",
        "status": "notStarted",
        "importance": "normal",
        "createdDateTime": "2026-01-29T14:00:00Z",
        "lastModifiedDateTime": "2026-01-29T14:00:00Z",
        "isCompleted": False
    },
    {
        "id": "5",
        "title": "代码审查",
        "body": "检查代码质量和风格",
        "status": "notStarted",
        "importance": "high",
        "createdDateTime": "2026-01-29T15:00:00Z",
        "lastModifiedDateTime": "2026-01-29T15:00:00Z",
        "isCompleted": False
    },
    {
        "id": "6",
        "title": "添加触摸功能",
        "body": "实现触摸交互",
        "status": "completed",
        "importance": "high",
        "createdDateTime": "2026-01-29T08:00:00Z",
        "lastModifiedDateTime": "2026-01-29T16:00:00Z",
        "isCompleted": True
    },
    {
        "id": "7",
        "title": "优化UI布局",
        "body": "改善界面美观度",
        "status": "inProgress",
        "importance": "normal",
        "createdDateTime": "2026-01-29T17:00:00Z",
        "lastModifiedDateTime": "2026-01-29T17:00:00Z",
        "isCompleted": False
    },
    {
        "id": "8",
        "title": "测试滚动功能",
        "body": "验证列表滚动效果",
        "status": "notStarted",
        "importance": "normal",
        "createdDateTime": "2026-01-29T18:00:00Z",
        "lastModifiedDateTime": "2026-01-29T18:00:00Z",
        "isCompleted": False
    }
]

@app.route('/')
def index():
    """首页"""
    return jsonify({
        "message": "TODO API Test Server",
        "version": "1.0",
        "endpoints": {
            "GET /api/todos": "获取所有TODO项",
            "GET /api/todos/<id>": "获取指定TODO项",
            "POST /api/todos": "创建新TODO项",
            "PUT /api/todos/<id>": "更新TODO项",
            "DELETE /api/todos/<id>": "删除TODO项",
            "POST /api/todos/<id>/complete": "标记TODO为完成",
            "POST /api/todos/<id>/uncomplete": "标记TODO为未完成"
        }
    })

@app.route('/api/todos', methods=['GET'])
@require_api_key
def get_todos():
    """获取所有TODO项"""
    # 支持过滤
    status = request.args.get('status')
    if status:
        filtered = [t for t in todos if t['status'] == status]
        return jsonify({"value": filtered, "count": len(filtered)})
    
    return jsonify({"value": todos, "count": len(todos)})

@app.route('/api/todos/<todo_id>', methods=['GET'])
def get_todo(todo_id):
    """获取指定TODO项"""
    todo = next((t for t in todos if t['id'] == todo_id), None)
    if todo:
        return jsonify(todo)
    return jsonify({"error": "TODO not found"}), 404

@app.route('/api/todos', methods=['POST'])
@require_api_key
def create_todo():
    """创建新TODO项"""
    data = request.json
    
    new_todo = {
        "id": str(uuid.uuid4()),
        "title": data.get('title', ''),
        "body": data.get('body', ''),
        "status": data.get('status', 'notStarted'),
        "importance": data.get('importance', 'normal'),
        "createdDateTime": datetime.utcnow().isoformat() + 'Z',
        "lastModifiedDateTime": datetime.utcnow().isoformat() + 'Z',
        "isCompleted": data.get('isCompleted', False)
    }
    
    todos.append(new_todo)
    return jsonify(new_todo), 201

@app.route('/api/todos/<todo_id>', methods=['PUT'])
@require_api_key
def update_todo(todo_id):
    """更新TODO项"""
    todo = next((t for t in todos if t['id'] == todo_id), None)
    if not todo:
        return jsonify({"error": "TODO not found"}), 404
    
    data = request.json
    
    # 更新字段
    if 'title' in data:
        todo['title'] = data['title']
    if 'body' in data:
        todo['body'] = data['body']
    if 'status' in data:
        todo['status'] = data['status']
    if 'importance' in data:
        todo['importance'] = data['importance']
    if 'isCompleted' in data:
        todo['isCompleted'] = data['isCompleted']
        if data['isCompleted']:
            todo['status'] = 'completed'
    
    todo['lastModifiedDateTime'] = datetime.utcnow().isoformat() + 'Z'
    
    return jsonify(todo)

@app.route('/api/todos/<todo_id>', methods=['DELETE'])
@require_api_key
def delete_todo(todo_id):
    """删除TODO项"""
    global todos
    todo = next((t for t in todos if t['id'] == todo_id), None)
    if not todo:
        return jsonify({"error": "TODO not found"}), 404
    
    todos = [t for t in todos if t['id'] != todo_id]
    return jsonify({"message": "TODO deleted", "id": todo_id})

@app.route('/api/todos/<todo_id>/complete', methods=['POST'])
@require_api_key
def complete_todo(todo_id):
    """标记TODO为完成"""
    todo = next((t for t in todos if t['id'] == todo_id), None)
    if not todo:
        return jsonify({"error": "TODO not found"}), 404
    
    todo['isCompleted'] = True
    todo['status'] = 'completed'
    todo['lastModifiedDateTime'] = datetime.utcnow().isoformat() + 'Z'
    
    return jsonify(todo)

@app.route('/api/todos/<todo_id>/uncomplete', methods=['POST'])
@require_api_key
def uncomplete_todo(todo_id):
    """标记TODO为未完成"""
    todo = next((t for t in todos if t['id'] == todo_id), None)
    if not todo:
        return jsonify({"error": "TODO not found"}), 404
    
    todo['isCompleted'] = False
    todo['status'] = 'notStarted'
    todo['lastModifiedDateTime'] = datetime.utcnow().isoformat() + 'Z'
    
    return jsonify(todo)

@app.route('/api/stats', methods=['GET'])
@require_api_key
def get_stats():
    """获取统计信息"""
    total = len(todos)
    completed = len([t for t in todos if t['isCompleted']])
    pending = total - completed
    
    return jsonify({
        "total": total,
        "completed": completed,
        "pending": pending,
        "completionRate": f"{(completed/total*100):.1f}%" if total > 0 else "0%"
    })

if __name__ == '__main__':
    print("=" * 50)
    print("TODO API Test Server")
    print("=" * 50)
    print("Server running on: http://0.0.0.0:5000")
    print("Local access: http://127.0.0.1:5000")
    print("Network access: http://<your-ip>:5000")
    print("=" * 50)
    app.run(host='0.0.0.0', port=5000, debug=True)
