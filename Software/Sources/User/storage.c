#include "storage.h"




//哈希相关常量
#define HASH_SIZE 127
#define HASH_SEED 37
//哈希维护结构体


//地址维护常量
#define FLASH_ADDR_START 0xFFFFFFFF
#define FLASH_ADDR_END  0xFFFFFFFF
#define FLASH_SECTOR_SIZE 512
#define FLASH_DEFAULT_VALUE 0xFE
//地址维护结构体
struct addr_maintainece{
    uint32_t flash_addr;

};
//地址注册函数，利用哈希值进行映射？
//通过数据地址计算出哈希值，通过数据类型更新地址计数器
//哈希值作为索引来在数组中存闪存地址
//更改时只需要计算出正确的哈希值->通过哈希值得到正确的闪存地址->覆写数据

//哈希冲突怎么办
//将刚才的闪存地址数组更改为链表结构
//链表结构体包含数据如下
typedef struct{
    void far* ram_addr; 
    uint32_t flash_addr;
    VarType_t type;
    uint8_t sector; //对齐用的空位,现在换成扇区
    uint16_t next; //这个next是对应下一个元素在storageVarPool中的索引
}StorageObj_t;
//维护一个单向链表头数组，每一个数组元素代表这个哈希值对应的链表头
//如果哈希冲突了，就在对应的链表头后面添加一个元素
//当访问哈希冲突数据时，就顺序遍历链表，比较地址，直到找到闪存地址对应的链表元素


//如何写入闪存？
//由于我们已经把地址保存进了池中，接下来直接修改，并标记我们已经修改的数据就可以，最后再保存
//如何保存?
//由于写操作的单位是一个扇区，所以我们会先遍历所有扇区，按照扇区筛选所需数据
//每处理完一个扇区，就读取这个扇区的所有数据，并在内存中重新写入标记的数据
//然后再覆写整个扇区

//如何读取？
//直接传入地址，直接通过已经保存的数据类型直接修改数据

#define STATIC_RAM_SIZE 50
uint16_t strorageVarPoolAddr = 0; //资源池维护
static StorageObj_t xdata storageVarPool[STATIC_RAM_SIZE]={0};
static uint16_t xdata storageTable[HASH_SIZE]={0};//这个数组的索引是对应的哈希值，其元素是链表头在池中的位置

static StorageErr_t static_malloc(uint16_t far* addr){
    if(strorageVarPoolAddr<50){*addr = strorageVarPoolAddr;return 0;}
    strorageVarPoolAddr++;
    return OUT_OF_RAM;
}





//哈希模块
/* 哈希算法函数
基于MurmurHash3_x86_32
*/
static uint32_t rotl32 ( uint32_t x,int8_t r){
    return (x << r) | (x >> (32 - r)); // 将 x 左移 r 位，并将 x 右移 (32-r) 位（填充零），然后进行位或操作。这是位旋转的典型实现。
}

#define ROTL32(x,y) rotl32(x,y)
static uint32_t _fmix32(uint32_t h){
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}
static uint16_t hash_cal(void far* addr){
   // uint8_t i;
    uint32_t h1 = HASH_SEED;
    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;
    uint32_t k1 = 0;
    k1 = (uint32_t)addr;

    k1 *= c1;
    k1=rotl32(k1,15);
    k1*=c2;

    h1^=k1;
    h1=rotl32(h1,13);
    h1=h1*5+0xe6546b64;

    h1 ^= 4;
    h1 = _fmix32(h1);

    return (uint16_t)(h1%HASH_SIZE);
}

/* 哈希注册函数
输入地址，完成静态分配(得到在storageVarPool的下标)并
返回错误代码//在storageTable中的下标（哈希值）。
*/
static StorageErr_t hash_register(void far * addr){
    uint16_t i;
    StorageErr_t e;
    e=static_malloc(&i);
    if(e){
        return e;
    }
    storageTable[hash_cal(addr)] = i;
    return 0;
}
static StorageObj_t far* hash_search(void far* addr){
    uint16_t h1 = hash_cal(addr);
    StorageObj_t far* obj;
    obj = &storageVarPool[storageTable[h1]];
    if(obj->ram_addr == addr)return obj;
    while(obj->next!=0){
        if(obj->ram_addr==addr)return obj;
        obj = &storageVarPool[obj->next];
    }
    return NULL;
}
//写入缓存
static uint8_t xdata storageWriteBuffer[FLASH_SECTOR_SIZE];
static uint8_t xdata storageWritedDataTable[256]={0}; //标记修改的数据
//写入读出封装函数
static void Storage_Write_Buffer(void far* addr){
    uint16_t table = hash_cal(addr);
    storageWritedDataTable[table]=1;
    return;
}
static void Storage_Save_Data(){

}

static void Storage_Read_Flash(StorageObj_t far*  dat){

    return;
}
static void _storage_Read_Sector(){
    
}
uint16_t _get_Sector_Index(uint32_t flash_addr){
    return(flash_addr-FLASH_ADDR_START)/512;
}


static uint8_t GetTypeSize(uint8_t type) {
    switch(type) {
        case DBG_UINT8: case DBG_INT8: return 1;
        case DBG_UINT16: case DBG_INT16: return 2;
        case DBG_UINT32: case DBG_INT32: case DBG_FLOAT: return 4;
        default: return 0;
    }
}