import os
import time
import shutil
import logging
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from pathlib import Path

class TwoWaySyncHandler(FileSystemEventHandler):
    """
    双向同步处理器 - 监控两个目录的文件变化并自动同步
    """
    
    def __init__(self, source_dir, target_dir, file_extensions=('.c', '.h'), sync_delay=0.5):
        self.source_dir = Path(source_dir)
        self.target_dir = Path(target_dir)
        self.file_extensions = file_extensions
        self.sync_delay = sync_delay
        self.last_sync_time = {}
        self.setup_logging()
        
    def setup_logging(self):
        """设置日志"""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('sync_log.txt'),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)
    
    def should_sync(self, file_path):
        """判断是否应该同步文件（避免频繁同步）"""
        current_time = time.time()
        file_path = str(file_path)
        
        if file_path in self.last_sync_time:
            if current_time - self.last_sync_time[file_path] < self.sync_delay:
                return False
        
        self.last_sync_time[file_path] = current_time
        return True
    
    def is_target_file(self, file_path):
        """判断是否是目标文件类型"""
        return any(str(file_path).endswith(ext) for ext in self.file_extensions)
    
    def find_matching_files(self, filename, search_dir):
        """在目录中查找匹配的文件名"""
        matching_files = []
        for root, dirs, files in os.walk(search_dir):
            if filename in files:
                matching_files.append(Path(root) / filename)
        return matching_files
    
    def sync_file(self, src_path, event_type="modified"):
        """
        同步单个文件
        event_type: created, modified, moved, deleted
        """
        if not self.is_target_file(src_path):
            return
        
        if not self.should_sync(src_path):
            return
        
        src_path = Path(src_path)
        filename = src_path.name
        
        # 确定源目录和目标目录
        if src_path.is_relative_to(self.source_dir):
            from_dir, to_dir = self.source_dir, self.target_dir
            direction = "Source → Driver"
        else:
            from_dir, to_dir = self.target_dir, self.source_dir
            direction = "Driver → Source"
        
        try:
            if event_type in ["created", "modified", "moved"]:
                # 文件创建、修改或移动
                if src_path.exists():
                    # 在目标目录中查找同名文件
                    target_files = self.find_matching_files(filename, to_dir)
                    
                    if target_files:
                        # 同步到所有找到的目标文件
                        for target_file in target_files:
                            shutil.copy2(src_path, target_file)
                            self.logger.info(f"🔄 {direction}: {src_path} -> {target_file}")
                    else:
                        self.logger.warning(f"⚠️  跳过: {filename} (在目标目录中未找到匹配文件)")
                else:
                    self.logger.warning(f"⚠️  源文件不存在: {src_path}")
            
            elif event_type == "deleted":
                # 文件删除 - 我们不自动删除另一边的文件，只是记录
                self.logger.info(f"🗑️  文件被删除: {src_path} (不会同步删除操作)")
                
        except Exception as e:
            self.logger.error(f"❌ 同步失败 {src_path}: {str(e)}")
    
    def on_created(self, event):
        """文件创建事件"""
        if not event.is_directory:
            self.sync_file(event.src_path, "created")
    
    def on_modified(self, event):
        """文件修改事件"""
        if not event.is_directory:
            self.sync_file(event.src_path, "modified")
    
    def on_moved(self, event):
        """文件移动事件"""
        if not event.is_directory:
            self.sync_file(event.dest_path, "moved")
    
    def on_deleted(self, event):
        """文件删除事件"""
        if not event.is_directory:
            self.sync_file(event.src_path, "deleted")

def setup_directories():
    """设置监控目录"""
    current_dir = Path.cwd()
    
    # 根据您的项目结构设置目录
    source_dir = current_dir / "Sources" / "AiCube"
    driver_dir = current_dir / "Sources" / "Drivers"
    
    # 确保目录存在
    for directory in [source_dir, driver_dir]:
        if not directory.exists():
            print(f"警告: 目录不存在: {directory}")
            directory.mkdir(parents=True, exist_ok=True)
            print(f"已创建目录: {directory}")
    
    return source_dir, driver_dir

def start_monitoring():
    """开始监控和同步"""
    source_dir, driver_dir = setup_directories()
    
    print("=" * 60)
    print("🔍 开始实时双向文件同步监控")
    print("=" * 60)
    print(f"监控目录 1 (Source): {source_dir}")
    print(f"监控目录 2 (Driver): {driver_dir}")
    print(f"监控文件类型: {('.c', '.h')}")
    print("=" * 60)
    print("操作说明:")
    print("  🔄 - 文件已同步")
    print("  ⚠️  - 跳过同步 (目标目录无匹配文件)")
    print("  🗑️  - 文件被删除 (仅记录)")
    print("  ❌ - 同步失败")
    print("=" * 60)
    print("按 Ctrl+C 停止监控")
    print("=" * 60)
    
    # 创建观察者
    observer = Observer()
    
    # 为每个目录创建事件处理器
    # 监控Source目录，同步到Driver
    source_handler = TwoWaySyncHandler(source_dir, driver_dir)
    observer.schedule(source_handler, str(source_dir), recursive=True)
    
    # 监控Driver目录，同步到Source
    driver_handler = TwoWaySyncHandler(driver_dir, source_dir)
    observer.schedule(driver_handler, str(driver_dir), recursive=True)
    
    # 开始监控
    observer.start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n🛑 停止监控...")
        observer.stop()
    
    observer.join()
    print("监控已停止")

def initial_sync():
    """初始同步 - 确保两边文件一致"""
    source_dir, driver_dir = setup_directories()
    
    print("执行初始同步检查...")
    
    # 同步Source → Driver
    sync_count = 0
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.endswith(('.c', '.h')):
                source_file = Path(root) / file
                target_files = find_matching_files(file, driver_dir)
                
                for target_file in target_files:
                    shutil.copy2(source_file, target_file)
                    print(f"🔁 初始同步: {source_file} -> {target_file}")
                    sync_count += 1
    
    # 同步Driver → Source
    for root, dirs, files in os.walk(driver_dir):
        for file in files:
            if file.endswith(('.c', '.h')):
                driver_file = Path(root) / file
                target_files = find_matching_files(file, source_dir)
                
                for target_file in target_files:
                    shutil.copy2(driver_file, target_file)
                    print(f"🔁 初始同步: {driver_file} -> {target_file}")
                    sync_count += 1
    
    print(f"初始同步完成，共同步 {sync_count} 个文件")

def find_matching_files(filename, search_dir):
    """查找匹配的文件"""
    matching_files = []
    for root, dirs, files in os.walk(search_dir):
        if filename in files:
            matching_files.append(Path(root) / filename)
    return matching_files

if __name__ == "__main__":
    # 安装依赖: pip install watchdog
    
    print("双向文件同步监控工具")
    print("1. 执行初始同步 + 开始监控")
    print("2. 仅开始监控")
    print("3. 仅执行初始同步")
    
    choice = input("请选择操作 (1-3, 默认1): ").strip() or "1"
    
    if choice in ["1", "3"]:
        initial_sync()
    
    if choice in ["1", "2"]:
        start_monitoring()