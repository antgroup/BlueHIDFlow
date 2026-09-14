#!/usr/bin/env python3
"""
AntSpark Serial Monitor - ESP32-S3 串口监控工具

功能:
- 实时显示串口输出
- 带时间戳
- 颜色高亮 (错误/警告/成功)
- 自动保存日志到文件
- 支持自动重连

使用方法:
    python serial_monitor.py [port] [baudrate]

示例:
    python serial_monitor.py                          # 使用默认端口
    python serial_monitor.py /dev/cu.usbmodem21301   # 指定端口
    python serial_monitor.py /dev/cu.usbmodem21301 115200  # 指定端口和波特率
"""

import sys
import time
import datetime
import threading
import os
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("请先安装 pyserial: pip install pyserial")
    sys.exit(1)

# 颜色定义
class Colors:
    RESET = '\033[0m'
    RED = '\033[91m'      # 错误
    YELLOW = '\033[93m'   # 警告
    GREEN = '\033[92m'    # 成功
    CYAN = '\033[96m'     # 信息
    BLUE = '\033[94m'     # 调试
    MAGENTA = '\033[95m'  # 状态
    DIM = '\033[90m'      # 暗淡 (时间戳)
    BOLD = '\033[1m'


class SerialMonitor:
    def __init__(self, port=None, baudrate=115200, log_file=None):
        self.port = port
        self.baudrate = baudrate
        self.running = False
        self.ser = None
        self.reconnect_delay = 2  # 重连延迟(秒)

        # 日志文件
        if log_file:
            self.log_path = Path(log_file)
        else:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            self.log_path = Path(__file__).parent / "logs" / f"serial_{timestamp}.log"

        # 确保日志目录存在
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        self.log_file = None

    def find_esp32_port(self):
        """自动查找 ESP32 设备端口"""
        ports = list_ports.comports()
        esp32_keywords = ['usbmodem', 'usbserial', 'USB Serial', 'CP210', 'CH340', 'SLAB']

        for port in ports:
            port_str = f"{port.device} {port.description} {port.hwid}"
            for keyword in esp32_keywords:
                if keyword.lower() in port_str.lower():
                    return port.device

        # 如果没找到，返回第一个可用端口
        if ports:
            return ports[0].device

        return None

    def colorize_line(self, line):
        """根据内容添加颜色"""
        line_lower = line.lower()

        # 错误
        if any(x in line_lower for x in ['error', '错误', '❌', 'failed', 'fail']):
            return f"{Colors.RED}{line}{Colors.RESET}"

        # 警告
        if any(x in line_lower for x in ['warn', '警告', '⚠️']):
            return f"{Colors.YELLOW}{line}{Colors.RESET}"

        # 成功
        if any(x in line_lower for x in ['success', '成功', '✅', 'connected', '已连接']):
            return f"{Colors.GREEN}{line}{Colors.RESET}"

        # 调试
        if any(x in line_lower for x in ['debug', '调试']):
            return f"{Colors.BLUE}{line}{Colors.RESET}"

        # 状态/心跳
        if any(x in line_lower for x in ['heartbeat', '状态', 'status', '[conn]', '[main]', '[mqtt]']):
            return f"{Colors.CYAN}{line}{Colors.RESET}"

        # 配置相关
        if any(x in line_lower for x in ['config', '配置', 'wifi', 'mqtt', 'ap ']):
            return f"{Colors.MAGENTA}{line}{Colors.RESET}"

        return line

    def write_log(self, line):
        """写入日志文件"""
        if self.log_file:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            self.log_file.write(f"[{timestamp}] {line}\n")
            self.log_file.flush()

    def connect(self):
        """连接串口"""
        if not self.port:
            self.port = self.find_esp32_port()
            if not self.port:
                print(f"{Colors.RED}未找到 ESP32 设备{Colors.RESET}")
                return False

        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1,
                write_timeout=1
            )
            # 等待串口稳定
            time.sleep(0.1)
            return True
        except serial.SerialException as e:
            print(f"{Colors.RED}连接失败: {e}{Colors.RESET}")
            return False

    def monitor(self):
        """监控串口输出"""
        buffer = ""

        while self.running:
            try:
                if self.ser and self.ser.is_open:
                    # 读取数据
                    if self.ser.in_waiting > 0:
                        data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='replace')
                        buffer += data

                        # 处理完整的行
                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            line = line.strip('\r')

                            if line:
                                # 时间戳
                                timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

                                # 颜色化
                                colored_line = self.colorize_line(line)

                                # 打印
                                print(f"{Colors.DIM}[{timestamp}]{Colors.RESET} {colored_line}")

                                # 写入日志
                                self.write_log(line)
                    else:
                        time.sleep(0.01)  # 短暂休眠避免 CPU 占用过高
                else:
                    time.sleep(0.1)

            except serial.SerialException as e:
                print(f"{Colors.RED}连接断开: {e}{Colors.RESET}")
                if self.running:
                    print(f"{Colors.YELLOW}尝试重连...{Colors.RESET}")
                    time.sleep(self.reconnect_delay)
                    if self.connect():
                        print(f"{Colors.GREEN}重连成功{Colors.RESET}")
            except Exception as e:
                print(f"{Colors.RED}错误: {e}{Colors.RESET}")
                time.sleep(0.1)

    def start(self):
        """启动监控"""
        print(f"{Colors.BOLD}{'='*50}{Colors.RESET}")
        print(f"{Colors.BOLD}  AntSpark Serial Monitor{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*50}{Colors.RESET}")
        print(f"端口: {self.port or '自动检测'}")
        print(f"波特率: {self.baudrate}")
        print(f"日志文件: {self.log_path}")
        print(f"{Colors.BOLD}{'='*50}{Colors.RESET}")
        print(f"按 Ctrl+C 退出\n")

        # 连接串口
        if not self.connect():
            # 尝试列出可用端口
            print(f"\n{Colors.YELLOW}可用端口:{Colors.RESET}")
            ports = list_ports.comports()
            for port in ports:
                print(f"  - {port.device}: {port.description}")
            return

        print(f"{Colors.GREEN}✅ 已连接到 {self.port}{Colors.RESET}\n")

        # 打开日志文件
        self.log_file = open(self.log_path, 'w', encoding='utf-8')
        self.log_file.write(f"AntSpark Serial Log - {datetime.datetime.now()}\n")
        self.log_file.write(f"Port: {self.port}, Baudrate: {self.baudrate}\n")
        self.log_file.write("=" * 60 + "\n\n")

        self.running = True

        # 启动监控线程
        monitor_thread = threading.Thread(target=self.monitor, daemon=True)
        monitor_thread.start()

        # 主线程等待
        try:
            while self.running:
                time.sleep(0.5)
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}正在停止...{Colors.RESET}")
            self.running = False
            if self.ser and self.ser.is_open:
                self.ser.close()
            if self.log_file:
                self.log_file.close()
            print(f"{Colors.GREEN}已停止，日志已保存到: {self.log_path}{Colors.RESET}")


def main():
    # 解析参数
    port = sys.argv[1] if len(sys.argv) > 1 else None
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    # 启动监控
    monitor = SerialMonitor(port=port, baudrate=baudrate)
    monitor.start()


if __name__ == "__main__":
    main()