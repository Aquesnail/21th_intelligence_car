#include "storage.h"




//哈希相关常量
#define HASH_SIZE 127
#define HASH_SEED 37
//哈希维护结构体


//地址维护常量
#define FLASH_ADDR_START 0xFC0000
#define FLASH_ADDR_END   0xFC37FF
#define FLASH_SECTOR_SIZE 512
#define FLASH_SECTOR_NUM (FLASH_ADDR_END - FLASH_ADDR_START + 1)/FLASH_SECTOR_SIZE

#define FLASH_DEFAULT_VALUE 0xFF
//地址维护结构体
struct {
    uint32_t flash_addr_count;
    uint16_t dirty_num;
   // uint16_t flash_sector;
}flash_t;
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
   // uint8_t sector; //对齐用的空位,现在换成扇区
    uint16_t next; //这个next是对应下一个元素在storageVarPool中的索引，是解决哈希冲突用的
    uint16_t next_sector;//这个对应下一个同扇区的元素在池子中的索引，用于处理保存时的逻辑
    VarType_t type : 4;
    uint8_t dirty_flags : 1; //这个脏标记flag是为了应对哈希冲突时，无法辨认修改变量的问题
    char name[10];
   // uint8_t dirty_sector : 1; //标记脏扇区
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
uint16_t storageVarPoolAddr = 0; //资源池维护
static StorageObj_t xdata storageVarPool[STATIC_RAM_SIZE]={0};
static uint16_t xdata storageTable[HASH_SIZE]={0};//这个数组的索引是对应的哈希值，其元素是链表头在池中的位置
static uint16_t xdata storageWritedSector[FLASH_SECTOR_NUM]; //这个数组的索引是对应的扇区值，其元素是链表头在池中的位置
uint8_t xdata storageSectorMark[FLASH_SECTOR_NUM];


//写入缓存
static uint8_t xdata storageWriteBuffer[FLASH_SECTOR_SIZE];


static uint8_t _getTypeSize(VarType_t type) {
    switch(type) {
        case DBG_UINT8: case DBG_INT8: return 1;
        case DBG_UINT16: case DBG_INT16: return 2;
        case DBG_UINT32: case DBG_INT32: case DBG_FLOAT: return 4;
        default: return 0;
    }
}
uint16_t _get_Sector_Index(uint32_t flash_addr){
    return(uint16_t)((flash_addr-FLASH_ADDR_START)/FLASH_SECTOR_SIZE);
}
static StorageErr_t static_malloc(uint16_t far* addr){
    if(storageVarPoolAddr<50){
        *addr = storageVarPoolAddr; 
        storageVarPoolAddr++; 
        return 0;
    }
       
    return OUT_OF_RAM;
}

//这里的flash_malloc不仅需要扇区对齐，还要4字节对齐
// 修改后的 flash_malloc，增加地址对齐逻辑
static StorageErr_t flash_malloc(uint32_t far* flash_addr_out, VarType_t type){
    uint8_t t = _getTypeSize(type);
    uint32_t current_addr = flash_t.flash_addr_count;
    uint32_t next_addr;
    
    // 【新增】地址对齐处理 (Padding)
    // STC32 是大端机，为了配合 IAP_Read_4Bytes等硬件指令，必须对齐
   if (t <= 4) {
        // 如果是少于 4 字节的变量，地址必须是 4 的倍数
        if (current_addr & 0x03) { // 如果低两位不为0
            current_addr = (current_addr & 0xFFFFFFFC) + 4;  //位运算小魔法 :)
        }
        // while (current_addr % 4 != 0) {
        //     current_addr++; 
        // }
    }

    // 计算占用后的下一个地址
    next_addr = current_addr + t;
    
    // 检查是否溢出总空间
    if(next_addr > FLASH_ADDR_END + 1) return OUT_OF_FLASH;

    // 检查是否跨扇区
    // 这里的逻辑也需要微调：如果对齐后的变量跨扇区了，直接跳到下一扇区起始
    if(_get_Sector_Index(current_addr) != _get_Sector_Index(next_addr - 1)){
        // 计算下一个扇区的物理起始地址
        uint16_t next_sec_idx = _get_Sector_Index(current_addr) + 1;
        current_addr = FLASH_ADDR_START + ((uint32_t)next_sec_idx * FLASH_SECTOR_SIZE);
        
        // 跨扇区后，新地址肯定是 512 的倍数，天然满足 4 字节对齐，不需要再对齐了
        
        // 重新检查溢出
        if(current_addr + t > FLASH_ADDR_END + 1) return OUT_OF_FLASH; //溢出直接退出，没退出才能执行对flash_addr_out的赋值，完美的剪枝，AI自己看第二遍都看迷糊说自己写错了...
    }

    *flash_addr_out = current_addr;
    flash_t.flash_addr_count = current_addr + t; // 更新全局计数器
    return 0;
}

// uint8_t _flash_write_single(uint32_t flash_addr,union Converter dat,VarType_t type){
    
//     return 0;
// }
// uint8_t _flash_read_single(uint32_t flash_addr,union Converter far* dat,VarType_t type){

//     return 0;
// }
// uint8_t _flash_write_interface(uint32_t flash_addr,uint8_t far* dat,uint16_t len){
//     return 0;
// }

// uint8_t _flash_write_interface(uint32_t flash_addr,uint8_t far*dar,uint16_t len){
//     return 0;
// }
static void _flash_write_sector(uint16_t index){
    uint32_t addr=index*FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    uint16_t i;
    for(i=0;i<FLASH_SECTOR_SIZE;i+=16){
        IAP_Write_16Bytes(addr+i,&storageWriteBuffer[i]);
    }
    return;
}
static void _flash_erase_sector(uint16_t index){
    uint32_t addr=index*FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    IAP_EraseSector(addr);
		return;
}
static void _flash_read_sector(uint16_t index){
    uint32_t addr=index*FLASH_SECTOR_SIZE + FLASH_ADDR_START;
    uint16_t i;
    for(i=0;i<FLASH_SECTOR_SIZE;i+=16){
        IAP_Read_16Bytes(addr+i,&storageWriteBuffer[i]);
    }
    return;
}

static void _flash_read_single(uint8_t far* ram_addr,uint32_t flash_addr,VarType_t type){
  //  uint8_t i;
    switch(type){
        case DBG_FLOAT:
        case DBG_INT32:
        case DBG_UINT32:
            IAP_Read_4Bytes(flash_addr,ram_addr);
            break;
        case DBG_INT16:
        case DBG_UINT16:
            IAP_Read_2Bytes(flash_addr,ram_addr);

            break;
        case DBG_UINT8:
        case DBG_INT8:
            *ram_addr = IAP_ReadByte(flash_addr);
            break;
    }
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
StorageErr_t Storage_Register(void far * addr,VarType_t type,char* name){
    uint16_t index;
    uint16_t table;
    StorageObj_t far* obj;
    StorageErr_t e;
    
    e=static_malloc(&index);
    if(e){
        return e;
    }
    table = hash_cal(addr);

    if(storageTable[table]==0xFFFF){ //头插法
        storageTable[table] = index;
        obj=&storageVarPool[index];
        obj->next = 0xFFFF;
        obj->next_sector=0xFFFF;
    }
    else{  
        storageVarPool[index].next = storageTable[table];
        storageTable[table]=index;
    }
    //obj->next=index;
    obj=&(storageVarPool[index]);
    obj->ram_addr = addr;
    e = flash_malloc(&(obj->flash_addr),type);
    if(e){
        return e;
    }
    obj->type = type;
    obj->dirty_flags = 0; //之前初始化为0xFF时这玩意儿也被置1了
    memcpy(obj->name,name,strlen(name));
    table = _get_Sector_Index(obj->flash_addr);
    if(storageWritedSector[table]==0xFFFF){
        storageWritedSector[table]=index;
        storageVarPool[index].next_sector = 0xFFFF;
    }else{
        obj->next_sector=storageWritedSector[table];
        storageWritedSector[table]=index;
    }

    
    return 0;
}
static StorageErr_t hash_search(void far* addr,StorageObj_t far*far* obj){
    uint16_t h1 = hash_cal(addr);
    uint16_t curr_index;
    curr_index = storageTable[h1];
    while(curr_index!=0xFFFF){
        *obj=&storageVarPool[curr_index];
        if(addr==(*obj)->ram_addr){
            return 0;
        }
        curr_index = (*obj)->next;
    }
    return NOT_REGISTERED;
}
StorageErr_t Storage_Load(void far* addr){
    StorageObj_t far* obj;
    StorageErr_t e;
    e = hash_search(addr,&obj);
    if(e){
        return e;
    }
    _flash_read_single(addr,obj->flash_addr,obj->type);
    return 0;
}

//标记脏函数
StorageErr_t Storage_Mark_Data(void far* addr){
    uint16_t table; 
    StorageObj_t far* obj;
    table = hash_cal(addr);
    table = storageTable[table];
    while(table!=0xFFFF){
        obj = &storageVarPool[table];
        if(obj->ram_addr == (addr)){
            obj->dirty_flags = 1;
            flash_t.dirty_num++;
            storageSectorMark[_get_Sector_Index(obj->flash_addr)] = 1;
            return 0;
        }
        table = obj->next;
    }
    return NOT_REGISTERED;
}
uint8_t xdata temp_flash_buf[16];
StorageErr_t Storage_Data_Writed(void far* addr){
    StorageObj_t far* obj;
    StorageErr_t e;
    uint8_t i,j;
    e = hash_search(addr,&obj);
    if(e){
        return e;
    }
    _flash_read_single(temp_flash_buf,obj->flash_addr,obj->type);
    e=_getTypeSize(obj->type);
    j=0;
    for(i=0;i<e;i++){
        if(temp_flash_buf[i]==0xFF)
            j++;
    }
    if(j==e)return NOT_IN_FLASH;
    return 0;
}

void Storage_Save_Data(){
    uint16_t i;
    uint16_t curr_index;
    StorageObj_t far* obj;


    for(i=0;i<FLASH_SECTOR_NUM;i++){
        if(storageSectorMark[i]==1){
           // obj = &(storageVarPool[(storageWritedSector[i])]);
            _flash_read_sector(i);
            curr_index=storageWritedSector[i];
            // if(obj->dirty_flags){
            //     _memcpy(
            //         (void far*)obj->ram_addr,
            //         (void far*)storageWriteBuffer[obj->flash_addr%FLASH_SECTOR_SIZE],
            //         _getTypeSize(obj->type)
            //     );
            //     obj->dirty_flags=0;
            //     //memcpy(&(storageWriteBuffer[obj->flash_addr%FLASH_SECTOR_SIZE]),obj->ram_addr,
					//     //这里用库函数会变成通用指针,理论上不影响
            // }

            while(curr_index!=0xFFFF){
                obj=&(storageVarPool[curr_index]); 
                if(obj->dirty_flags){
                    memcpy(
                        (void far*)&(storageWriteBuffer[(obj->flash_addr)%FLASH_SECTOR_SIZE]),
                        (void far*)obj->ram_addr,
                        _getTypeSize(obj->type)
                    );


                    
                    obj->dirty_flags=0;
                }
                curr_index=obj->next_sector;
            }
            _flash_erase_sector(i);
            _flash_write_sector(i);
             storageSectorMark[i]=0;
        }
    }
}



void Storage_Init(){
    memset(storageVarPool,0xFF,sizeof(StorageObj_t)*STATIC_RAM_SIZE);
    memset(storageTable,0xFF,sizeof(uint16_t)*HASH_SIZE);
    memset(storageWritedSector,0xFF,sizeof(uint16_t)*FLASH_SECTOR_NUM);
    memset(storageSectorMark,0,sizeof(uint8_t)*FLASH_SECTOR_NUM);
    flash_t.flash_addr_count= FLASH_ADDR_START;
    flash_t.dirty_num = 0;
}
