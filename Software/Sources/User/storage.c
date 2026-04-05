#include "storage.h"
#include <string.h>

// 哈希相关常量
#define HASH_SIZE 127
#define HASH_SEED 37

// 地址维护常量
#define FLASH_ADDR_START 0xFC0000
#define FLASH_ADDR_END   0xFC37FF
#define FLASH_SECTOR_SIZE 512
#define FLASH_SECTOR_NUM ((FLASH_ADDR_END - FLASH_ADDR_START + 1) / FLASH_SECTOR_SIZE)

#define FLASH_DEFAULT_VALUE 0xFF

// 地址维护结构体
struct {
    uint32_t flash_addr_count;
    uint16_t dirty_num;
} flash_t;
//地址注册函数，利用哈希值进行映射？
//通过数据地址计算出哈希值，通过数据类型更新地址计数器
//哈希值作为索引来在数组中存闪存地址
//更改时只需要计算出正确的哈希值->通过哈希值得到正确的闪存地址->覆写数据

//哈希冲突怎么办
//将刚才的闪存地址数组更改为链表结构
//链表结构体包含数据如下
// 存储对象结构体
typedef struct {
    void far* ram_addr;
    uint32_t flash_addr;
    uint16_t next;           //这个next是对应下一个元素在storageVarPool中的索引，是解决哈希冲突用的
    uint16_t next_sector;    //这个对应下一个同扇区的元素在池子中的索引，用于处理保存时的逻辑
    uint16_t len;            // 数据长度（字节）
    uint8_t dirty_flags;     // 脏标记
    char name[10];           // 变量名（调试用）
} StorageObj_t;

#define STATIC_RAM_SIZE 50
uint16_t storageVarPoolAddr = 0;
static StorageObj_t xdata storageVarPool[STATIC_RAM_SIZE] = {0};
static uint16_t xdata storageTable[HASH_SIZE] = {0};
static uint16_t xdata storageWritedSector[FLASH_SECTOR_NUM];//这个数组的索引是对应的哈希值，其元素是链表头在池中的位置
uint8_t xdata storageSectorMark[FLASH_SECTOR_NUM]; //这个数组的索引是对应的扇区值，其元素是链表头在池中的位置

static uint8_t xdata storageWriteBuffer[FLASH_SECTOR_SIZE];

// 计算扇区索引
static uint16_t _get_Sector_Index(uint32_t flash_addr) {
    return (uint16_t)((flash_addr - FLASH_ADDR_START) / FLASH_SECTOR_SIZE);
}

// 从对象池分配索引
static StorageErr_t static_malloc(uint16_t* addr) {
    if (storageVarPoolAddr < STATIC_RAM_SIZE) {
        *addr = storageVarPoolAddr;
        storageVarPoolAddr++;
        return 0;
    }
    return OUT_OF_RAM;
}

// Flash 空间分配（带4字节对齐处理）
static StorageErr_t flash_malloc(uint32_t far* flash_addr_out, uint16_t len) {
    uint32_t current_addr = flash_t.flash_addr_count;
    uint32_t next_addr;
    uint16_t next_sec;

    // 长度 ≤4 时要求地址4字节对齐
    if (len <= 4) {
        if (current_addr & 0x03) {
            current_addr = (current_addr & 0xFFFFFFFC) + 4;
        }
    }

    next_addr = current_addr + len;
    if (next_addr > FLASH_ADDR_END + 1) {
        return OUT_OF_FLASH;
    }

    // 检查是否跨扇区，若跨则跳到下一扇区起始
    if (_get_Sector_Index(current_addr) != _get_Sector_Index(next_addr - 1)) {
        next_sec = _get_Sector_Index(current_addr) + 1;
        current_addr = FLASH_ADDR_START + (uint32_t)next_sec * FLASH_SECTOR_SIZE;
        if (current_addr + len > FLASH_ADDR_END + 1) {
            return OUT_OF_FLASH;
        }
    }

    *flash_addr_out = current_addr;
    flash_t.flash_addr_count = current_addr + len;
    return 0;
}

// 扇区操作
static void _flash_read_sector(uint16_t index) {
    uint32_t addr;
    uint16_t i;
    
    addr = index * FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    for (i = 0; i < FLASH_SECTOR_SIZE; i += 16) {
        IAP_Read_16Bytes(addr + i, &storageWriteBuffer[i]);
    }
}

static void _flash_write_sector(uint16_t index) {
    uint32_t addr;
    uint16_t i;
    
    addr = index * FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    for (i = 0; i < FLASH_SECTOR_SIZE; i += 16) {
        IAP_Write_16Bytes(addr + i, &storageWriteBuffer[i]);
    }
}

static void _flash_erase_sector(uint16_t index) {
    uint32_t addr = index * FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    IAP_EraseSector(addr);
}

// 通用Flash读取（按字节）
static void _flash_read(uint32_t flash_addr, void far* buf, uint16_t len) {
    uint8_t far* p = (uint8_t far*)buf;
    while (len--) {
        *p++ = IAP_ReadByte(flash_addr++);
    }
}

// 哈希计算（MurmurHash3 变体）
static uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

static uint32_t _fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

static uint16_t hash_cal(void far* addr) {
    uint32_t h1 = HASH_SEED;
    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;
    uint32_t k1 = (uint32_t)addr;

    k1 *= c1;
    k1 = rotl32(k1, 15);
    k1 *= c2;

    h1 ^= k1;
    h1 = rotl32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;

    h1 ^= 4;
    h1 = _fmix32(h1);

    return (uint16_t)(h1 % HASH_SIZE);
}

// 通过RAM地址查找对象
static StorageErr_t hash_search(void far* addr, StorageObj_t far** obj) {
    uint16_t h = hash_cal(addr);
    uint16_t idx = storageTable[h];
    while (idx != 0xFFFF) {
        *obj = &storageVarPool[idx];
        if (addr == (*obj)->ram_addr) {
            return 0;
        }
        idx = (*obj)->next;
    }
    return NOT_REGISTERED;
}

// 注册变量（基于长度）
StorageErr_t Storage_Register(void far* addr, uint16_t len, char* name) {
    uint16_t index;
    uint16_t table;
    uint16_t sector;
    StorageObj_t* obj;
    StorageErr_t e;
    
    e = static_malloc(&index);
    if (e) return e;

    table = hash_cal(addr);
    obj = &storageVarPool[index];

    // 插入哈希表（头插法）
    if (storageTable[table] == 0xFFFF) {
        storageTable[table] = index;
        obj->next = 0xFFFF;
    } else {
        obj->next = storageTable[table];
        storageTable[table] = index;
    }

    obj->ram_addr = addr;
    e = flash_malloc(&obj->flash_addr, len);
    if (e) return e;

    obj->len = len;
    obj->dirty_flags = 0;
    strncpy(obj->name, name, sizeof(obj->name) - 1);
    obj->name[sizeof(obj->name) - 1] = '\0';

    // 插入扇区链表
    sector = _get_Sector_Index(obj->flash_addr);
    if (storageWritedSector[sector] == 0xFFFF) {
        storageWritedSector[sector] = index;
        obj->next_sector = 0xFFFF;
    } else {
        obj->next_sector = storageWritedSector[sector];
        storageWritedSector[sector] = index;
    }

    return 0;
}

// 从Flash加载到RAM
StorageErr_t Storage_Load(void far* addr) {
    StorageObj_t far* obj;
    StorageErr_t e;
    
    e = hash_search(addr, &obj);
    if (e) return e;
    _flash_read(obj->flash_addr, addr, obj->len);
    return 0;
}

// 标记数据为脏
StorageErr_t Storage_Mark_Data(void far* addr) {
    uint16_t h;
    uint16_t idx;
    StorageObj_t far* obj;
    
    h = hash_cal(addr);
    idx = storageTable[h];
    while (idx != 0xFFFF) {
        obj = &storageVarPool[idx];
        if (obj->ram_addr == addr) {
            obj->dirty_flags = 1;
            flash_t.dirty_num++;
            storageSectorMark[_get_Sector_Index(obj->flash_addr)] = 1;
            return 0;
        }
        idx = obj->next;
    }
    return NOT_REGISTERED;
}

// 检查数据是否已在Flash中写入（非全0xFF）
StorageErr_t Storage_Data_Writed(void far* addr) {
    StorageObj_t far* obj;
    uint16_t i;
    StorageErr_t e;
    
    e = hash_search(addr, &obj);
    if (e) return e;
    for (i = 0; i < obj->len; i++) {
        if (IAP_ReadByte(obj->flash_addr + i) != 0xFF) {
            return 0;  // 已写入
        }
    }
    return NOT_IN_FLASH;
}

// 保存所有脏数据到Flash
void Storage_Save_Data(void) {
    uint16_t i;
    uint16_t idx;
    uint16_t offset;
    StorageObj_t far* obj;
    
    for (i = 0; i < FLASH_SECTOR_NUM; i++) {
        if (storageSectorMark[i] == 1) {
            _flash_read_sector(i);
            idx = storageWritedSector[i];
            while (idx != 0xFFFF) {
                obj = &storageVarPool[idx];
                if (obj->dirty_flags) {
                    offset = obj->flash_addr % FLASH_SECTOR_SIZE;
                    memcpy((void far*)&storageWriteBuffer[offset],
                           (void far*)obj->ram_addr,
                           obj->len);
                    obj->dirty_flags = 0;
                }
                idx = obj->next_sector;
            }
            _flash_erase_sector(i);
            _flash_write_sector(i);
            storageSectorMark[i] = 0;
        }
    }
    flash_t.dirty_num = 0;
}

// 初始化
void Storage_Init(void) {
    memset(storageVarPool, 0xFF, sizeof(storageVarPool));
    memset(storageTable, 0xFF, sizeof(storageTable));
    memset(storageWritedSector, 0xFF, sizeof(storageWritedSector));
    memset(storageSectorMark, 0, sizeof(storageSectorMark));
    flash_t.flash_addr_count = FLASH_ADDR_START;
    flash_t.dirty_num = 0;
}

// 通过名称查找变量
StorageErr_t Storage_FindByName(const char* name, void far** addr_out, uint16_t* len_out) {
    uint16_t i;
    for (i = 0; i < storageVarPoolAddr; i++) {
        if (strcmp(storageVarPool[i].name, name) == 0) {
            if (addr_out) *addr_out = storageVarPool[i].ram_addr;
            if (len_out)  *len_out  = storageVarPool[i].len;
            return 0;
        }
    }
    return NOT_REGISTERED;
}

// 获取已注册变量数量
uint16_t Storage_GetRegCount(void) {
    return storageVarPoolAddr;
}

// 获取指定索引的变量名
const char* Storage_GetRegName(uint16_t idx) {
    if (idx < storageVarPoolAddr) return storageVarPool[idx].name;
    return NULL;
}

// 获取指定索引的变量RAM地址
void far* Storage_GetRegAddr(uint16_t idx) {
    if (idx < storageVarPoolAddr) return storageVarPool[idx].ram_addr;
    return NULL;
}

// 获取指定索引的变量长度
uint16_t Storage_GetRegLen(uint16_t idx) {
    if (idx < storageVarPoolAddr) return storageVarPool[idx].len;
    return 0;
}