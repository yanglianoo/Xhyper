#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#define NCPU            4
/* 4K size */
#define SZ_4K           0x00001000
/* 虚拟管理器的物理内存加载地址 */
#define HIMAGE_VADDR    0x40200000

#define PHYBASE         0x40000000
#define PHYSIZE         (512 * 1024 * 1024)
#define PHYEND          (PHYBASE + PHYSIZE)

#endif