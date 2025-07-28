#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#define NCPU            4

/* 4K size */
#define SZ_4K           0x00001000
#define PAGESIZE        SZ_4K

/* 虚拟管理器的物理内存加载地址 */
#define HIMAGE_VADDR    0x40200000

#define PHYBASE         0x40000000
#define PHYSIZE         (128 * 1024 * 1024)
#define PHYEND          (PHYBASE + PHYSIZE)

/* PL011设备寄存器的基地址 */
#define PL011BASE   0x09000000

#endif