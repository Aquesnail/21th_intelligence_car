import serial
import struct
import time
import threading
import sys

# --- 配置区域 ---
SERIAL_PORT = 'COM4'  # 请修改为你的串口号
BAUD_RATE = 115200

# STC32 C251 的指针类型定义
MEM_XDATA = 0x01
MEM_EDATA = 0x02

# 全局状态
running = True
vars_map = {} # ID -> Name 映射
meta_map = {}
# CRC16-MODBUS 算法
def calc_crc(data):
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for _ in range(8):
            if (crc & 1) != 0:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc

# 发送修改变量指令 (0x55)
def send_write_cmd(ser, var_id, var_len, value):
    # Payload: Cmd(1) + ID(1) + Len(1) + Data(n)
    payload = bytearray()
    payload.append(0x55)
    payload.append(var_id)
    payload.append(var_len)
    
    # 数据打包 (大端序)
    if var_len == 1:
        payload.append(value & 0xFF)
    elif var_len == 4: # Float or U32? 这里假设 demo 改的是 u8，通用需判断类型
         pass 

    # 计算 CRC
    crc = calc_crc(payload)
    
    # 组帧: 0x55 + Payload + CRC
    frame = bytearray()
    frame.append(0x55)
    frame.extend(payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    ser.write(frame)
    print(f"\n[PC -> MCU] Write ID {var_id} to {value}")

# 发送动态注册指令 (0x56)
def send_register_cmd(ser, mem_type, address_u24, name, var_type):
    # Payload: Cmd(1) + Type(1) + Addr(4) + Name(10)
    payload = bytearray()
    payload.append(0x56)      # Cmd
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
    ser.write(frame)
    print(f"\n[PC -> MCU] Dynamic Register Request: {name}")
def send_text_cmd(ser, text_str):
    # Payload: Cmd(1) + Len(1) + Text(n)
    payload = bytearray()
    payload.append(0x57) # Cmd
    
    text_bytes = text_str.encode('ascii') # 或者 'utf-8'
    if len(text_bytes) > 60: text_bytes = text_bytes[:60] # 限制长度
    
    payload.append(len(text_bytes)) # Len
    payload.extend(text_bytes)
    
    # CRC
    crc = calc_crc(payload)
    
    # Frame
    frame = bytearray()
    frame.append(0x55)
    frame.extend(payload)
    frame.append(crc >> 8)
    frame.append(crc & 0xFF)
    
    ser.write(frame)
    print(f"\n[PC -> MCU] Sent Text: {text_str}")
# 接收线程
def rx_thread(ser):
    print("Start listening...")
    while True:
        try:
            if ser.read(1) == b'\xAA':
                header = ser.read(3) # ID, Type, Len
                if len(header) < 3: continue
                vid, vtype, vlen = header
                
                data = ser.read(vlen)
                crc_bytes = ser.read(2)
                if len(crc_bytes) < 2: continue
                crc_recv = (crc_bytes[0] << 8) | crc_bytes[1]
                
                # 校验
                check_payload = bytearray([vid, vtype, vlen]) + data
                if calc_crc(check_payload) == crc_recv:
                    
                    # ---> CASE 1: 元数据包 (ID=0xFE) <---
                    if vid == 0xFE:
                        # Payload 结构: [ID:1] [Type:1] [Addr:4] [Name:10] = 16 Bytes
                        reg_id = data[0]
                        reg_type = data[1]
                        reg_addr_bytes = data[2:6] # 4 byte
                        reg_name = data[6:16].decode('ascii').rstrip('\0')
                        
                        reg_addr_val = struct.unpack('>I', reg_addr_bytes)[0]
                        
                        # 存入上位机字典
                        meta_map[reg_id] = {
                            "name": reg_name,
                            "type": reg_type,
                            "addr": reg_addr_val
                        }
                        print(f"[META] New Var Registered: ID={reg_id} Name={reg_name} Type={reg_type}")
                        
                    # ---> CASE 2: 系统日志 (ID=0xFF) <---
                    elif vid == 0xFF:
                        log_msg = data.decode('ascii', errors='ignore')
                        print(f"[LOG] {log_msg}")

                    # ---> CASE 3: 普通数值包 <---
                    else:
                        # 尝试从 meta_map 获取名字，如果没收到 meta 包则显示 Unknown
                        var_name = "Unknown"
                        if vid in meta_map:
                            var_name = meta_map[vid]["name"]
                            
                        # 解析数值
                        val = 0
                        if vlen == 1: val = struct.unpack('B', data)[0]
                        elif vlen == 2: val = struct.unpack('>H', data)[0]
                        elif vlen == 4:
                            if vtype == 6: val = struct.unpack('>f', data)[0]
                            else: val = struct.unpack('>I', data)[0]
                            
                        # 简单打印
                        if vid in meta_map: # 只打印已知的
                            # \r 实现原地刷新
                            # print(f"{var_name}: {val}      ", end='\r')
                            pass

        except Exception as e:
            print(e)
            break

# --- 主程序 ---
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    
    t = threading.Thread(target=rx_thread, args=(ser,))
    t.start()
    
    print("Wait 5s to see initial data...")
    time.sleep(5)
    
    # --- 测试用例 1: 修改变量 ---
    # 假设 ID 2 是 'Control' (u8), 当前是 10
    # 我们把它改成 99
    send_write_cmd(ser, var_id=2, var_len=1, value=99)
    
    print("Wait 3s to confirm target changed...")
    time.sleep(3)
    
    # --- 测试用例 2: 动态注册变量 ---
    # 这里的地址 0x011234 是瞎写的！
    # 你必须填入 MCU 串口启动时打印出来的 "Hidden Var Addr"
    # 例如 MCU 打印: [TEST] Hidden Var Addr: 0x01004A
    # 则 mem_type=0x01, address=0x004A
    
    real_addr_str = input("\n\n>>> 请输入 MCU 打印的隐藏变量地址后4位 (例如 004A): 0x")
    real_addr = int(real_addr_str, 16)
    
    # 发送注册指令: Type=4 (U32)
    send_register_cmd(ser, MEM_XDATA, real_addr, "MySecret", 4)
    
    print("Now you should see HIDDEN value appearing in the line...")
    
    print("\n--- Starting Loopback Test ---")
    time.sleep(1)
    
    send_text_cmd(ser, "Hello STC32!")
    time.sleep(0.5)
    
    send_text_cmd(ser, "FreeRTOS is running.")
    time.sleep(0.5)
    
    # 测试一下长文本
    send_text_cmd(ser, "1234567890-ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    running = False
    ser.close()