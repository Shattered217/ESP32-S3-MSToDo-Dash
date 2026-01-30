#!/usr/bin/env python3
"""
快速配置服务器IP地址
自动修改main.c中的SERVER_URL
"""

import socket
import sys

def get_local_ip():
    """获取本机IP地址"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return None

def update_server_url(ip_address):
    """更新main.c中的SERVER_URL"""
    main_c_path = "main/main.c"
    
    try:
        with open(main_c_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 查找并替换SERVER_URL
        import re
        pattern = r'#define SERVER_URL "http://[^"]+\"'
        new_url = f'#define SERVER_URL "http://{ip_address}:5000"'
        
        if re.search(pattern, content):
            new_content = re.sub(pattern, new_url, content)
            
            with open(main_c_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            
            print(f"✅ 成功更新 {main_c_path}")
            print(f"   SERVER_URL = http://{ip_address}:5000")
            return True
        else:
            print(f"❌ 未找到 SERVER_URL 定义")
            return False
            
    except Exception as e:
        print(f"❌ 错误: {e}")
        return False

def main():
    print("=" * 60)
    print("ESP32 TODO 应用 - 服务器IP配置工具")
    print("=" * 60)
    
    # 获取本机IP
    local_ip = get_local_ip()
    if local_ip:
        print(f"\n检测到本机IP地址: {local_ip}")
        print(f"Flask服务器将在: http://{local_ip}:5000")
    else:
        print("\n无法自动检测IP地址")
    
    # 让用户确认或输入
    print("\n请选择:")
    print("1. 使用检测到的IP地址" + (f" ({local_ip})" if local_ip else " (未检测到)"))
    print("2. 手动输入IP地址")
    print("3. 退出")
    
    choice = input("\n请输入选择 (1/2/3): ").strip()
    
    target_ip = None
    
    if choice == "1":
        if local_ip:
            target_ip = local_ip
        else:
            print("❌ 未检测到IP地址，请手动输入")
            return
    elif choice == "2":
        target_ip = input("请输入服务器IP地址: ").strip()
    elif choice == "3":
        print("退出")
        return
    else:
        print("无效的选择")
        return
    
    # 验证IP格式
    try:
        socket.inet_aton(target_ip)
    except socket.error:
        print(f"❌ 无效的IP地址: {target_ip}")
        return
    
    # 更新配置
    print(f"\n正在更新配置...")
    if update_server_url(target_ip):
        print("\n" + "=" * 60)
        print("配置完成!")
        print("=" * 60)
        print("\n下一步:")
        print("1. 启动Flask服务器:")
        print(f"   cd backend_test")
        print(f"   python app.py")
        print()
        print("2. 编译并烧录ESP32:")
        print(f"   idf.py build")
        print(f"   idf.py flash monitor")
        print()
        print(f"服务器地址: http://{target_ip}:5000")
        print("=" * 60)
    else:
        print("\n❌ 配置失败")

if __name__ == "__main__":
    main()
