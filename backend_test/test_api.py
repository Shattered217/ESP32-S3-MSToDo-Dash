"""
API测试脚本
用于测试Flask后端的所有功能
"""

import requests
import json

BASE_URL = "http://localhost:5000"

# API密钥（需要与后端保持一致）
API_KEY = "esp32-todo-secret-key-2024"
HEADERS = {"X-API-Key": API_KEY, "Content-Type": "application/json"}

def print_response(title, response):
    """打印响应"""
    print(f"\n{'='*60}")
    print(f"{title}")
    print(f"{'='*60}")
    print(f"状态码: {response.status_code}")
    try:
        print(f"响应:\n{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
    except:
        print(f"响应: {response.text}")

def test_all():
    """测试所有API端点"""
    
    # 1. 获取所有TODO
    print_response(
        "1. 获取所有TODO项",
        requests.get(f"{BASE_URL}/api/todos", headers=HEADERS)
    )
    
    # 2. 创建新TODO
    new_todo = {
        "title": "通过API创建的任务",
        "body": "这是通过测试脚本创建的",
        "importance": "high"
    }
    response = requests.post(
        f"{BASE_URL}/api/todos",
        json=new_todo,
        headers=HEADERS
    )
    print_response("2. 创建新TODO项", response)
    
    # 保存新创建的ID
    new_id = None
    if response.status_code == 201:
        new_id = response.json()['id']
    
    # 3. 获取指定TODO
    if new_id:
        print_response(
            f"3. 获取指定TODO (ID: {new_id})",
            requests.get(f"{BASE_URL}/api/todos/{new_id}", headers=HEADERS)
        )
    
    # 4. 更新TODO
    if new_id:
        update_data = {
            "title": "更新后的标题",
            "status": "inProgress"
        }
        print_response(
            f"4. 更新TODO (ID: {new_id})",
            requests.put(f"{BASE_URL}/api/todos/{new_id}", json=update_data)
        )
    
    # 5. 标记为完成
    if new_id:
        print_response(
            f"5. 标记TODO为完成 (ID: {new_id})",
            requests.post(f"{BASE_URL}/api/todos/{new_id}/complete", headers=HEADERS)
        )
    
    # 6. 获取统计信息
    print_response(
        "6. 获取统计信息",
        requests.get(f"{BASE_URL}/api/stats", headers=HEADERS)
    )
    
    # 7. 标记为未完成
    if new_id:
        print_response(
            f"7. 标记TODO为未完成 (ID: {new_id})",
            requests.post(f"{BASE_URL}/api/todos/{new_id}/uncomplete", headers=HEADERS)
        )
    
    # 8. 删除TODO
    if new_id:
        print_response(
            f"8. 删除TODO (ID: {new_id})",
            requests.delete(f"{BASE_URL}/api/todos/{new_id}")
        )
    
    # 9. 再次获取所有TODO，确认删除成功
    print_response(
        "9. 删除后获取所有TODO项",
        requests.get(f"{BASE_URL}/api/todos", headers=HEADERS)
    )

if __name__ == "__main__":
    print("=" * 60)
    print("TODO API 测试脚本")
    print("=" * 60)
    print(f"测试服务器: {BASE_URL}")
    print("确保Flask服务器正在运行...")
    print("=" * 60)
    
    try:
        test_all()
        print("\n" + "=" * 60)
        print("测试完成！")
        print("=" * 60)
    except requests.exceptions.ConnectionError:
        print("\n错误: 无法连接到服务器")
        print("请先运行: python app.py")
    except Exception as e:
        print(f"\n错误: {e}")
