#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned long  u64;
typedef long           s64;
typedef unsigned int   u32;
typedef signed int     s32;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned char  u8;
typedef signed char    s8;

#define NULL ((void *)0)

typedef _Bool bool;
#define true  1
#define false 0

#define ALIGN_PAGE(address) ((address + (SZ_4K - 1)) & ~(SZ_4K - 1))

#endif
