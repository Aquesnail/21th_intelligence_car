import os
import shutil
import sys

def copy_matching_files(sources_root, drivers_root):
    """
    从sources_root目录及其子目录中查找与drivers_root目录中同名的文件，
    并将sources中的文件复制到drivers中对应的位置
    """
    
    # 确保目录存在
    if not os.path.exists(sources_root):
        print(f"错误: Sources目录不存在: {sources_root}")
        return False
        
    if not os.path.exists(drivers_root):
        print(f"错误: Drivers目录不存在: {drivers_root}")
        return False
    
    # 获取Drivers目录下所有的.c和.h文件（包括子目录）
    drivers_files = {}
    for root, dirs, files in os.walk(drivers_root):
        for file in files:
            if file.endswith(('.c', '.h')):
                # 存储相对路径到Drivers根目录的映射
                rel_path = os.path.relpath(os.path.join(root, file), drivers_root)
                drivers_files[file] = {
                    'rel_path': rel_path,
                    'src_path': os.path.join(root, file)
                }
    
    if not drivers_files:
        print("警告: Drivers目录下没有找到.c或.h文件")
        return False
    
    print(f"在Drivers目录中找到 {len(drivers_files)} 个.c/.h文件:")
    for filename in drivers_files:
        print(f"  - {filename} (位置: {drivers_files[filename]['rel_path']})")
    print()
    
    # 在Sources目录中查找匹配的文件
    files_copied = 0
    files_skipped = 0
    
    for root, dirs, files in os.walk(sources_root):
        for file in files:
            if file.endswith(('.c', '.h')):
                if file in drivers_files:
                    # 构建源文件路径和目标文件路径
                    source_file = os.path.join(root, file)
                    target_rel_path = drivers_files[file]['rel_path']
                    target_file = os.path.join(drivers_root, target_rel_path)
                    
                    # 确保目标目录存在
                    target_dir = os.path.dirname(target_file)
                    if not os.path.exists(target_dir):
                        os.makedirs(target_dir)
                    
                    try:
                        # 复制文件
                        shutil.copy2(source_file, target_file)
                        files_copied += 1
                        print(f"✓ 已复制: {source_file} -> {target_file}")
                    except Exception as e:
                        print(f"✗ 复制失败 {source_file} -> {target_file}: {e}")
                else:
                    files_skipped += 1
    
    print(f"\n操作完成:")
    print(f"  - 已复制文件: {files_copied} 个")
    print(f"  - 跳过文件: {files_skipped} 个 (Drivers中没有同名文件)")
    
    return True

if __name__ == "__main__":
    # 根据图片中的目录结构设置路径
    # 假设脚本运行在SOFTWARE目录下
    current_dir = os.getcwd()
    
    # Sources目录路径 (根据图片，这是包含inc和lib的目录)
    # 注意: 图片中Sources目录下还有一个Sources子目录，但不确定具体结构
    # 这里假设从最顶层的Sources开始搜索
    sources_dir = os.path.join(current_dir, "Sources\\AiCube")
    
    # Drivers目录路径
    drivers_dir = os.path.join(current_dir, "Sources\\Drivers")
    
    print("开始复制匹配的.c/.h文件...")
    print(f"Sources目录: {sources_dir}")
    print(f"Drivers目录: {drivers_dir}")
    print("-" * 50)
    
    if copy_matching_files(sources_dir, drivers_dir):
        print("\n所有匹配的文件已成功更新！")
    else:
        print("\n操作失败，请检查目录结构和权限。")
        sys.exit(1)