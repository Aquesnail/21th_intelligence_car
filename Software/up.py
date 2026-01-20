import serial
import struct
import time
import threading
import sys
import os
import serial.tools.list_ports
from datetime import datetime

# --- 配置区域 ---
SERIAL_PORT = None  # 初始为None，程序运行时选择
BAUD_RATE = 115200

# STC32 C251 的指针类型定义
MEM_XDATA = 0x01
MEM_EDATA = 0x02

# 全局状态
running = True
vars_map = {}  # ID -> Name 映射
meta_map = {}
ser = None  # 串口对象

# --- 串口相关功能 ---
def scan_serial_ports():
    """扫描可用的串口设备"""
    ports = []
    print("\n" + "="*60)
    print("串口设备扫描中...")
    print("="*60)
    
    try:
        available_ports = list(serial.tools.list_ports.comports())
        
        if not available_ports:
            print("未找到可用的串口设备！")
            print("请检查：")
            print("1. STC32开发板是否已连接到电脑")
            print("2. CH340/CH341驱动是否已正确安装")
            print("3. 数据线是否正常")
            return []
        
        print(f"找到 {len(available_ports)} 个串口设备：")
        print("-"*60)
        
        for i, port in enumerate(available_ports, 1):
            port_info = {
                'index': i,
                'device': port.device,
                'description': port.description,
                'hwid': port.hwid if hasattr(port, 'hwid') else 'N/A',
                'vid': 'N/A',
                'pid': 'N/A',
                'manufacturer': 'N/A',
                'product': 'N/A'
            }
            
            # 尝试提取更多信息
            if hasattr(port, 'vid') and port.vid:
                port_info['vid'] = f"0x{port.vid:04X}"
            if hasattr(port, 'pid') and port.pid:
                port_info['pid'] = f"0x{port.pid:04X}"
            if hasattr(port, 'manufacturer') and port.manufacturer:
                port_info['manufacturer'] = port.manufacturer
            if hasattr(port, 'product') and port.product:
                port_info['product'] = port.product
            
            ports.append(port_info)
            
            # 彩色显示
            is_likely_stc = False
            stc_indicators = ["CH340", "CH341", "STC", "USB", "Serial"]
            
            description_lower = str(port_info['description']).upper()
            for indicator in stc_indicators:
                if indicator.upper() in description_lower:
                    is_likely_stc = True
                    break
            
            if is_likely_stc:
                print(f"\033[92m[{i:2d}] {port_info['device']:8s} - {port_info['description']:30s}\033[0m")
                if port_info['vid'] != 'N/A':
                    print(f"      VID:PID={port_info['vid']}:{port_info['pid']} | {port_info['manufacturer']}")
            else:
                print(f"\033[90m[{i:2d}] {port_info['device']:8s} - {port_info['description']:30s}\033[0m")
        
        print("-"*60)
        return ports
        
    except Exception as e:
        print(f"扫描串口时出错: {e}")
        return []

def select_serial_port():
    """让用户选择串口"""
    while True:
        ports = scan_serial_ports()
        
        if not ports:
            retry = input("\n未找到串口设备，按R重新扫描，其他键退出: ").upper()
            if retry == 'R':
                continue
            else:
                return None
        
        print("\n操作选项:")
        print("  [数字]  选择对应序号的串口")
        print("  [R]     重新扫描串口")
        print("  [Q]     退出程序")
        
        choice = input("\n请选择操作: ").strip().upper()
        
        if choice == 'Q':
            return None
        elif choice == 'R':
            continue
        elif choice.isdigit():
            index = int(choice)
            if 1 <= index <= len(ports):
                selected_port = ports[index-1]
                print(f"\n\033[93m已选择: {selected_port['device']} - {selected_port['description']}\033[0m")
                return selected_port['device']
            else:
                print(f"错误: 请输入 1-{len(ports)} 之间的数字")
        else:
            print("无效输入，请重试")

def test_serial_connection(port_name):
    """测试串口连接"""
    print(f"\n正在连接 {port_name} ...")
    
    try:
        # 尝试打开串口
        test_ser = serial.Serial(
            port=port_name,
            baudrate=BAUD_RATE,
            timeout=0.1,
            bytesize=8,
            parity='N',
            stopbits=1,
            rtscts=False,
            dsrdtr=False
        )
        
        # 发送测试数据
        test_data = b'\x55\x00\x00'  # 简单的测试帧
        test_ser.write(test_data)
        
        # 短暂延迟
        time.sleep(0.1)
        
        # 尝试读取
        if test_ser.in_waiting > 0:
            data = test_ser.read(test_ser.in_waiting)
            if data:
                print(f"\033[92m连接成功！收到 {len(data)} 字节数据\033[0m")
            else:
                print("\033[93m连接成功，但未收到数据\033[0m")
        else:
            print("\033[93m连接成功，但未收到数据\033[0m")
        
        test_ser.close()
        return True
        
    except serial.SerialException as e:
        print(f"\033[91m连接失败: {e}\033[0m")
        return False
    except Exception as e:
        print(f"\033[91m未知错误: {e}\033[0m")
        return False

def save_port_config(port_name):
    """保存串口配置到文件"""
    try:
        with open("serial_config.txt", "w") as f:
            f.write(f"PORT={port_name}\n")
            f.write(f"BAUD={BAUD_RATE}\n")
            f.write(f"LAST_USED={datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        print(f"已保存配置到 serial_config.txt")
    except:
        pass

def load_port_config():
    """从文件加载串口配置"""
    try:
        if os.path.exists("serial_config.txt"):
            with open("serial_config.txt", "r") as f:
                for line in f:
                    if line.startswith("PORT="):
                        return line.strip().split("=")[1]
    except:
        pass
    return None

# CRC16-MODBUS 算法
def calc_crc(data):
    """
    计算 CRC-16-MODBUS
    :param data: 字节列表，每个元素为 0-255 的整数
    :return: CRC 值（整数）
    """
    crc = 0xFFFF  # 初始值 INIT = 0xFFFF
    for pos in data:
        crc ^= pos
        for _ in range(8):
            if crc & 1:
                crc >>= 1
                crc ^= 0xA001  # 多项式 0x8005 的反转（0xA001）
            else:
                crc >>= 1
    return crc

# 发送修改变量指令 (0x55)
def send_write_cmd(ser, var_id, var_len, value):
    if not ser or not ser.is_open:
        print("错误: 串口未打开")
        return False
    
    # 1. 构建核心负载 (Inner Payload)
    # 对应 MCU 的: id = rxBuf[3], vLen = rxBuf[4], Data...
    inner_payload = bytearray()
    inner_payload.append(var_id)
    inner_payload.append(var_len)
    
    # 数据打包 (大端序)
    if var_len == 1:
        inner_payload.append(value & 0xFF)
    elif var_len == 2:
        inner_payload.append((value >> 8) & 0xFF)
        inner_payload.append(value & 0xFF)
    elif var_len == 4:
        inner_payload.append((value >> 24) & 0xFF)
        inner_payload.append((value >> 16) & 0xFF)
        inner_payload.append((value >> 8) & 0xFF)
        inner_payload.append(value & 0xFF)

    # 2. 构建协议层负载 (Cmd + Len + InnerPayload)
    # MCU 校验 CRC 的范围是: CMD, LEN, PAYLOAD
    full_payload = bytearray()
    full_payload.append(0x55)           # Cmd
    full_payload.append(len(inner_payload)) # <--- [关键修复] 自动计算并插入长度
    full_payload.extend(inner_payload)
    
    # 3. 计算 CRC
    crc = calc_crc(full_payload)
    
    # 4. 组装最终帧
    frame = bytearray()
    frame.append(0x55)      # 帧头
    frame.extend(full_payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    try:
        ser.write(frame)
        print(f"\n[PC -> MCU] 写入 ID {var_id} 值 {value}")
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False

# [修复] 发送动态注册指令 (0x56)
def send_register_cmd(ser, mem_type, address_u24, name, var_type):
    if not ser or not ser.is_open:
        print("错误: 串口未打开")
        return False
    
    # 1. 构建核心负载
    inner_payload = bytearray()
    inner_payload.append(var_type)  # Type
    inner_payload.append(mem_type)            
    inner_payload.append((address_u24 >> 16) & 0xFF)
    inner_payload.append((address_u24 >> 8) & 0xFF)
    inner_payload.append(address_u24 & 0xFF)
    
    name_bytes = name.encode('ascii').ljust(10, b'\0')[:10]
    inner_payload.extend(name_bytes)
    
    # 2. 构建协议层负载
    full_payload = bytearray()
    full_payload.append(0x56)           # Cmd
    full_payload.append(len(inner_payload)) # <--- [关键修复]
    full_payload.extend(inner_payload)
    
    # 3. CRC
    crc = calc_crc(full_payload)
    
    # 4. 组帧
    frame = bytearray()
    frame.append(0x55)
    frame.extend(full_payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    try:
        ser.write(frame)
        print(f"\n[PC -> MCU] 动态注册请求: {name}")
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False

# [修复] 发送文本指令 (0x57)
def send_text_cmd(ser, text_str):
    if not ser or not ser.is_open:
        print("错误: 串口未打开")
        return False
    
    # 1. 构建核心负载 (文本内容)
    text_bytes = text_str.encode('ascii', errors='ignore')
    if len(text_bytes) > 60:
        text_bytes = text_bytes[:60]
    
    # 2. 构建协议层负载
    full_payload = bytearray()
    full_payload.append(0x57)           # Cmd
    full_payload.append(len(text_bytes))    # <--- [关键修复] 长度是文本本身的长度
    full_payload.extend(text_bytes)
    
    # 3. CRC
    crc = calc_crc(full_payload)
    
    # 4. 组帧
    frame = bytearray()
    frame.append(0x55)
    frame.extend(full_payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    try:
        ser.write(frame)
        print(f"\n[PC -> MCU] 发送文本: {text_str}")
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False

# 发送动态注册指令 (0x56)
def send_register_cmd(ser, mem_type, address_u24, name, var_type):
    if not ser or not ser.is_open:
        print("错误: 串口未打开")
        return False
    
    # Payload: Cmd(1) + Type(1) + Addr(4) + Name(10)
    payload = bytearray()
    payload.append(0x56)      # Cmd
    payload.append(15)      # Len = 1+4+10
    payload.append(var_type)  # Type
    payload.append(mem_type)            
    payload.append((address_u24 >> 16) & 0xFF)
    payload.append((address_u24 >> 8) & 0xFF)
    payload.append(address_u24 & 0xFF)
    
    name_bytes = name.encode('ascii').ljust(10, b'\0')[:10]
    payload.extend(name_bytes)
    
    crc = calc_crc(payload)
    
    frame = bytearray()
    frame.append(0x55)
    frame.extend(payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    try:
        ser.write(frame)
        print(f"\n[PC -> MCU] 动态注册请求: {name}")
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False

def send_text_cmd(ser, text_str):
    if not ser or not ser.is_open:
        print("错误: 串口未打开")
        return False
    
    # 构建payload
    payload = bytearray()
    payload.append(0x57)  # Cmd
    
    text_bytes = text_str.encode('ascii', errors='ignore')
    if len(text_bytes) > 60:
        text_bytes = text_bytes[:60]
    
    payload.append(len(text_bytes))  # Len
    payload.extend(text_bytes)
    
    # 构建CRC计算数据：帧头 + payload
    crc_data = bytearray()
    #crc_data.append(0x55)  # 包含帧头
    crc_data.extend(payload)
    
    # 计算CRC
    crc = calc_crc(crc_data)
    
    # 完整帧
    frame = bytearray()
    frame.append(0x55)  # 帧头
    frame.extend(payload)  # payload
    frame.append(crc >> 8)  # CRC高字节
    frame.append(crc & 0xFF)  # CRC低字节
    
    try:
        ser.write(frame)
        print(f"\n[PC -> MCU] 发送文本: {text_str}")
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False
def perform_handshake(ser):
    """
    执行握手流程
    发送: 0x55 0x00 0x04 [DEADBEEF] [CRC]
    接收: 0xAA 0xFD 0x00 0x04 [DEADBEEF] [CRC]
    """
    if not ser or not ser.is_open:
        return False
        
    print("正在与 MCU 进行握手...")
    
    # 1. 清空缓冲区，防止旧数据干扰
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    # 2. 构造握手包 (Magic = 0xDEADBEEF)
    payload = bytearray()
    payload.append(0x00) # Cmd = Handshake
    payload.append(0x04) # Len = 4
    payload.extend(b'\xDE\xAD\xBE\xEF') # Magic Number
    
    crc = calc_crc(payload)
    
    frame = bytearray()
    frame.append(0x55)
    frame.extend(payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    # 3. 发送
    ser.write(frame)
    
    # 4. 阻塞等待响应 (超时时间 1秒)
    end_time = time.time() + 1.0
    buffer = bytearray()
    
    while time.time() < end_time:
        if ser.in_waiting > 0:
            buffer.extend(ser.read(ser.in_waiting))
            
            # 检查是否有足够的长度: Head(1)+ID(1)+Type(1)+Len(1)+Data(4)+CRC(2) = 10 bytes
            if len(buffer) >= 10:
                # 简单查找帧头 0xAA
                try:
                    start_idx = buffer.index(0xAA)
                    # 确保剩余长度够
                    if len(buffer) - start_idx < 10:
                        continue
                        
                    packet = buffer[start_idx : start_idx+10]
                    
                    # 解析
                    vid = packet[1]
                    vlen = packet[3]
                    magic_recv = packet[4:8]
                    crc_recv = (packet[8] << 8) | packet[9]
                    
                    # 校验 CRC
                    check_payload = packet[1:8] # ID, Type, Len, Data
                    if calc_crc(check_payload) == crc_recv:
                        # 校验 ID 和 Magic
                        if vid == 0xFD and magic_recv == b'\xDE\xAD\xBE\xEF':
                            print(f"\033[92m握手成功! 设备在线。\033[0m")
                            return True
                        else:
                            print(f"握手响应内容错误: ID={vid:02X}, Data={magic_recv.hex()}")
                    else:
                        # CRC 错，可能是噪声，移除坏头继续找
                        buffer.pop(0)
                        continue
                        
                except ValueError:
                    # 没有0xAA，清空
                    buffer = bytearray()
    
    print("\033[91m握手超时！请检查设备连接或波特率。\033[0m")
    return False


# 接收线程
def rx_thread(ser):
    print("开始监听串口数据...")
    buffer = bytearray()
    current_values = {}  # 存储所有变量的当前值
    display_frequency = 5  # 每5个包刷新一次显示，避免刷新太快
    packet_count = 0
    while running and ser.is_open:
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
                # 打印原始数据用于调试
                #if len(data) > 0:
                    #print(f"[DEBUG] 收到 {len(data)} 字节原始数据: {data.hex()}")
                    #print(f"[DEBUG] 当前缓冲区: {len(buffer)} 字节")
                
                # 处理缓冲区中的所有完整数据包
                while len(buffer) > 0:
                    # 查找帧头
                    if buffer[0] != 0xAA:
                        # 如果不是帧头，丢弃第一个字节
                        #print(f"[DEBUG] 丢弃非帧头字节: 0x{buffer[0]:02X}")
                        buffer.pop(0)
                        continue
                    
                    # 检查是否有足够的数据读取头部
                    if len(buffer) < 4:  # 帧头(1)+ID(1)+Type(1)+Len(1)
                        #print(f"[DEBUG] 缓冲区不足，需要4字节，当前{len(buffer)}字节")
                        break
                    
                    vid, vtype, vlen = buffer[1], buffer[2], buffer[3]
                   # print(f"[DEBUG] 找到包: VID=0x{vid:02X}, VType=0x{vtype:02X}, VLen={vlen}")
                    
                    # 检查是否有完整的数据包
                    # 完整包长度 = 帧头(1)+ID(1)+Type(1)+Len(1)+数据(vlen)+CRC(2) = 6+vlen
                    required_len = 6 + vlen
                    if len(buffer) < required_len:
                        #print(f"[DEBUG] 包不完整，需要{required_len}字节，当前{len(buffer)}字节")
                        break
                    
                    # 提取完整数据包
                    packet = buffer[:required_len]
                    buffer = buffer[required_len:]
                    
                    #print(f"[DEBUG] 提取完整包 ({len(packet)}字节): {packet.hex()}")
                    
                    # 验证CRC
                    # CRC计算范围是从ID开始到数据结束
                    check_payload = bytearray([vid, vtype, vlen])
                    if vlen > 0:
                        check_payload.extend(packet[4:4+vlen])
                    
                    crc_recv = (packet[4+vlen] << 8) | packet[5+vlen]
                    crc_calc = calc_crc(check_payload)
                    
                    # 首先检查是否是元数据包
                    #if vid == 0xFE:
                    #    print(f"[DEBUG] 这是元数据包!")
                    #    print(f"[DEBUG] 期望vtype=0xFF, 实际vtype=0x{vtype:02X}")
                    #    print(f"[DEBUG] 期望vlen=16, 实际vlen={vlen}")
                        
                    #    if vtype == 0xFF:
                    #        print(f"[DEBUG] vtype正确，继续处理...")
                    #    else:
                    #        print(f"[DEBUG] vtype不正确!")
                    #
                    if crc_calc == crc_recv:
                        data_part = packet[4:4+vlen]
                        #print(f"[DEBUG] CRC校验通过")
                        #print(f"[DEBUG] 数据部分 ({len(data_part)}字节): {data_part.hex()}")
                        
                        # ---> CASE 1: 元数据包 (ID=0xFE) <---
                        if vid == 0xFE and vtype == 0xFF:
                            if vlen == 16:  # 元数据包固定16字节
                                reg_id = data_part[0]
                                reg_type = data_part[1]
                                reg_addr_bytes = data_part[2:6]
                                reg_name = data_part[6:16].decode('ascii', errors='ignore').rstrip('\x00')
                                
                                # 根据下位机memcpy(&txPkt[6], &debugTable[id].addr, 4)
                                # 这应该是原始字节序，通常是小端
                                reg_addr_val = struct.unpack('>I', reg_addr_bytes)[0]   #下位机用大端序解包
                                # = bytes(reg_addr_bytes)
                                # 保存到元数据映射
                                meta_map[reg_id] = {
                                    "name": reg_name,
                                    "type": reg_type,
                                    "addr": reg_addr_val
                                }
                                print(f"\n[META] 注册变量: ID={reg_id} 名称={reg_name} 类型={reg_type} 地址=0x{reg_addr_val:08X}")
                            else:
                                print(f"[ERROR] 元数据包长度错误: 期望16字节，实际{vlen}字节")
                        
                        # ---> CASE 2: 系统日志 (ID=0xFF) <---
                        elif vid == 0xFF:
                            try:
                                log_msg = data_part[:vlen].decode('ascii', errors='ignore')
                                log_msg = log_msg.rstrip('\x00')
                                if log_msg.strip():
                                    print(f"[LOG] {log_msg}")
                            except UnicodeDecodeError:
                                print(f"[LOG] 解码失败，原始数据: {data_part.hex()}")
                        
                        # ---> CASE 3: 握手响应包 (ID=0xFD) <---
                        elif vid == 0xFD:
                            pass
                            
                        # ---> CASE 4: 普通数值包 <---
                        # 在循环外定义一个字典存储所有变量值
                        # ---> CASE 4: 普通数值包 <---
                        else:
                            var_info = meta_map.get(vid)
                            if var_info:
                                var_name = var_info.get("name", f"Var_{vid}")
                            else:
                                var_name = f"Var_{vid}"
                            
                            val = 0
                            if vlen == 1:
                                val = struct.unpack('B', data_part)[0]
                            elif vlen == 2:
                                val = struct.unpack('>H', data_part)[0]
                            elif vlen == 4:
                                if var_info and var_info.get("type") == 6:  # Float
                                    val = struct.unpack('>f', data_part)[0]
                                else:  # U32/I32
                                    val = struct.unpack('>I', data_part)[0]
                            
                            # 存储当前值
                            current_values[vid] = (var_name, val, var_info.get("type") if var_info else 0)
                            
                            # 每N个包刷新一次显示
                            packet_count += 1
                            if packet_count >= display_frequency:
                                # 移动到屏幕上方固定位置显示变量监控
                                # 使用 \033[s 保存光标位置，显示后恢复
                                sys.stdout.write("\033[s")  # 保存当前光标位置
                                
                                # 将光标移动到屏幕顶部
                                sys.stdout.write("\033[1;1H")  # 移动到第1行第1列
                                
                                print("="*50)
                                print("实时变量监控")
                                print("="*50)
                                
                                # 按ID排序显示
                                sorted_vids = sorted(current_values.keys())
                                for i, vid in enumerate(sorted_vids):
                                    var_name, val, vtype = current_values[vid]
                                    
                                    # 根据类型格式化显示
                                    if vtype == 6:  # Float
                                        val_str = f"{val:>10.6f}"
                                    else:
                                        val_str = f"{val:>10}"
                                        
                                    # 清除当前行并重新输出
                                    sys.stdout.write(f"\033[KID={vid:3d} 名称={var_name:15s} 值={val_str} 类型={vtype}\n")
                                
                                print("="*50)
                                print("\n")  # 添加一个空行，与其他输出分隔
                                
                                sys.stdout.write("\033[u")  # 恢复之前保存的光标位置
                                
                                packet_count = 0
                            sys.stdout.flush()
                    else:
                        print(f"[ERROR] CRC校验失败")
                        print(f"  Packet: {packet.hex()}")
                        print(f"  VID: 0x{vid:02X}, VType: 0x{vtype:02X}, VLen: {vlen}")
                        print(f"  CRC计算: 0x{crc_calc:04X}, CRC接收: 0x{crc_recv:04X}")
                        print(f"  CRC计算数据: {check_payload.hex()}")
            
            time.sleep(0.01)
            
        except Exception as e:
            print(f"\n接收错误: {e}")
            import traceback
            traceback.print_exc()
            break
    
    print("接收线程结束")
# 主程序
def main():
    global running, ser, SERIAL_PORT
    
    print("\n" + "="*60)
    print("STC32 FreeRTOS 调试器")
    print("="*60)
    
    # 尝试加载之前的配置
    saved_port = load_port_config()
    if saved_port:
        print(f"检测到之前的配置: {saved_port}")
        use_saved = input("是否使用此串口? (Y/N): ").strip().upper()
        if use_saved == 'Y':
            SERIAL_PORT = saved_port
            if not test_serial_connection(SERIAL_PORT):
                SERIAL_PORT = None
    
    # 如果还没有选择串口，让用户选择
    if not SERIAL_PORT:
        SERIAL_PORT = select_serial_port()
        if not SERIAL_PORT:
            print("程序退出")
            return
    
    # 保存配置
    save_port_config(SERIAL_PORT)
    
    try:
        # 打开串口
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            timeout=0.1,
            bytesize=8,
            parity='N',
            stopbits=1
        )
        
        print(f"\n\033[92m成功打开串口: {SERIAL_PORT}\033[0m")
        print(f"波特率: {BAUD_RATE}")
        print("-"*60)
        if not perform_handshake(ser):
            print("握手失败，无法激活 MCU 数据流")
            return
        print("握手成功，正在接收元数据...")
        # 启动接收线程
        rx_thread_instance = threading.Thread(target=rx_thread, args=(ser,))
        rx_thread_instance.daemon = True
        rx_thread_instance.start()
        
        # 用户交互界面
        while running and ser.is_open:
            print("\n" + "="*60)
            print("STC32 调试器 - 主菜单")
            print("="*60)
            print("1. 修改变量值")
            print("2. 动态注册变量")
            print("3. 发送文本")
            print("4. 重新扫描串口")
            print("5. 显示已注册变量")
            print("6. 退出程序")
            print("="*60)
            
            try:
                choice = input("\n请选择操作 (1-6): ").strip()
                
                if choice == '1':
                    # 修改变量
                    try:
                        var_id = int(input("变量ID: "))
                        var_len = int(input("数据长度(1/2/4): "))
                        value = int(input("新值: "))
                        send_write_cmd(ser, var_id, var_len, value)
                    except ValueError:
                        print("输入错误")
                
                elif choice == '2':
                    # 动态注册变量
                    try:
                        mem_type = int(input("内存类型(1=XDATA,2=EDATA): "))
                        addr_str = input("地址(十六进制，如004A): ")
                        address = int(addr_str, 16)
                        name = input("变量名(最长10字符): ")
                        var_type = int(input("变量类型(0=uint8,1=int8,2=uint16,3=int16,4=uint32,5=int32,6=float): "))
                        send_register_cmd(ser, mem_type, address, name, var_type)
                    except ValueError as e:
                        print(f"输入错误: {e}")
                
                elif choice == '3':
                    # 发送文本
                    text = input("要发送的文本: ")
                    send_text_cmd(ser, text)
                
                elif choice == '4':
                    # 重新扫描串口
                    print("正在重新扫描串口...")
                    new_port = select_serial_port()
                    if new_port:
                        # 关闭当前串口
                        if ser and ser.is_open:
                            ser.close()
                        
                        # 保存新配置
                        SERIAL_PORT = new_port
                        save_port_config(SERIAL_PORT)
                        
                        # 重新打开串口
                        ser = serial.Serial(
                            port=SERIAL_PORT,
                            baudrate=BAUD_RATE,
                            timeout=0.1
                        )
                        
                        print(f"已切换到串口: {SERIAL_PORT}")
                
                elif choice == '5':
                    # 显示已注册变量
                    print(f"\n已注册变量 ({len(meta_map)} 个):")
                    print("-"*50)
                    for var_id, var_info in sorted(meta_map.items()):
                        print(f"ID:{var_id:3d}  {var_info['name']:12s}  Type:{var_info['type']}  Addr:0x{var_info['addr']:08X}")
                    if not meta_map:
                        print("暂无已注册变量")
                
                elif choice == '6':
                    # 退出
                    print("正在退出程序...")
                    running = False
                    break
                
                else:
                    print("无效选择，请重试")
                
            except KeyboardInterrupt:
                print("\n\n检测到Ctrl+C，正在退出...")
                running = False
                break
            except Exception as e:
                print(f"错误: {e}")
        
        # 关闭串口
        if ser and ser.is_open:
            ser.close()
            print("串口已关闭")
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except Exception as e:
        print(f"程序错误: {e}")
    finally:
        running = False

if __name__ == "__main__":
    main()