#ifndef __XMALLOC_H__
#define __XMALLOC_H__


#include "spinlock.h"
#include "types.h"
#include "arch.h"
#include <stdint.h>
#include <stddef.h>

#define CONFIG_BLK_SIZE   8192

#define BLK_SLICE_BIT     10
// BLK_SLICE_SIZE = 1 << 10 = 1024（1KB）
#define BLK_SLICE_SIZE    (1 << BLK_SLICE_BIT) 
// BLK_SLICE_NUM = 8
#define BLK_SLICE_NUM     (CONFIG_BLK_SIZE/BLK_SLICE_SIZE)

#define ALIGN_BIT       3
#define ALIGN_SIZE      (1 << ALIGN_BIT) 
#define ALIGN_MASK      (ALIGN_SIZE - 1)
#define ALIGN_UP(a)     (((a) + ALIGN_MASK) & ~ALIGN_MASK)

static inline uint8_t clz32(uint32_t x)
{
    uint8_t n;
    // 计算前导零的个数
    n = __builtin_clz(x);
    return n;
}

// 将一个给定的块大小（size）转换为对应的“类型”或“阶”（type），
// 通常表示块大小的以2为底的对数（log2）级别
#define BLK_SIZE2TYPE(size) (32 - clz32((uint32_t)(size) - 1))

#define BLK_TYPE2SIZE(type) (1 << type)

/*
blk_list_t 描述一个特定大小的内存块列表，
管理一组固定大小的内存块（例如 16 字节、32 字节等）。
*/
typedef struct {
    size_t     blk_size;     // 块大小（如8B、16B）
    uint32_t   freelist_cnt; // 空闲块数量
    uint32_t   nofree_cnt;   // 已分配块数量
    uintptr_t  slice_cnt;    // 使用的切片数量
    uintptr_t  slice_addr;   // 当前切片起始地址
    size_t     slice_offset; // 当前切片分配偏移量
    uintptr_t  free_head;    // 空闲块链表头指针
} blk_list_t;

typedef struct {
    spinlock_t  blk_lock;
    const char *pool_name;
    uintptr_t   pool_start;
    uintptr_t   pool_end;
    uint32_t    slice_cnt;
    char        slice_type[BLK_SLICE_NUM];
    blk_list_t  blk_list[BLK_SLICE_BIT];
} blk_pool_t;

int   xmalloc_init(void);
void *xmalloc(uint32_t size);
int   xfree(void *ptr);


#endif