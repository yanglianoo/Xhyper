#ifndef __VSPACE_H__
#define __VSPACE_H__

#include "types.h"

/*
 *  40 bit Virtual Address
 *
 *     40    39 38    30 29    21 20    12 11       0
 *    +--------+--------+--------+--------+----------+
 *    | level0 | level1 | level2 | level3 | page off |
 *    +--------+--------+--------+--------+----------+
 *    从给定的 40 位虚拟地址中提取指定级别的页表索引（Page Table Index）
 */

#define PINDEX(level, ipa)    (((ipa) >> (39 - (level * 9))) & 0x1ff)

/* bit[1]: 该页表项的类型
 * 0 - block descriptor
 * 1 - table descriptor
 *
 * bit[0]: 该表项是否有效
 * 0 - 该表项无效
 * 1 - 该表项有效
 */

#define PTE_VALID   0x1 /* Table/Page Description Valid  0b0*/
#define PTE_TABLE   0x2 /* Level 0,1,2 Table Description 0b10*/
#define PTE_V       0x3 /* Page Description and valid 0b11*/
#define PTE_AF      (1 << 10) /* Access Flag  跟踪页面是否被 CPU 访问（读或写）*/

/* Get the next-level table address [47:12] */
/* 提取页表项的 [47:12] 位 */
#define PTE_PA(pte) ((u64)(pte) & 0xFFFFFFFFF000)

/* AArch64 Memory Model Feature Register 0 */
/* 提取低四位 */
#define PA_RANGE(n)   ((n) & 0xF)

/* VTCR_EL2: Virtuaization Translation Control Register */
#define VTCR_T0SZ(n)  ((n) & 0x3F) /* IPA region size is 2(64-T0SZ) bytes */
#define VTCR_SL0(n)   (((n) & 0x3) << 6)  /* Starting level of the stage 2 translation lookup */
#define VTCR_SH0(n)   (((n) & 0x3) << 12) /* Shareability attribute */
#define VTCR_TG0(n)   (((n) & 0x3) << 14) /* Granule size */
#define VTCR_PS(n)    (((n) & 0x7) << 16) /* Physical address Size for the second stage of translation */
#define VTCR_NSW      (1 << 29)  /* Non-Secure */
#define VTCR_NSA      (1 << 30)  /* Non-Secure Access */

/* Memory Attribute  MAIR_EL2 寄存器*/
#define DEVICE_nGnRnE_INDEX 0x0  //Device-nGnRnE 内存类型。
#define NORMAL_NC_INDEX     0x1  //Normal Non-Cacheable 内存类型

#define DEVICE_nGnRnE       0x0    /* Device-nGnRnE memory */
#define NORMAL_NC           0x44   /* Normal memory, Inner/Outer Non-cacheable */

/* Stage 2 attribute */
#define S2PTE_S2AP(ap)      (((ap) & 3) << 6) // 设置PTE AP位
#define S2PTE_RO            S2PTE_S2AP(1)     // 只读（Read-Only）
#define S2PTE_WO            S2PTE_S2AP(2)     // 只写（Write-Only）
#define S2PTE_RW            S2PTE_S2AP(3)     // 读写（Read-Write）

/*  设置或提取 Stage-2 页表条目的 AttrIndx 字段（bit[4:2]，3 位），
    选择 MAIR_EL2 的内存属性索引。
*/
#define S2PTE_ATTR(attr)    (((attr) & 7) << 2)
#define S2PTE_NORMAL        S2PTE_ATTR(NORMAL_NC_INDEX)
#define S2PTE_DEVICE        S2PTE_ATTR(DEVICE_nGnRnE_INDEX)

/* Hypervisor Configuration Register  HCR_EL2 */
#define HCR_VM              (1 << 0)   /* HCR_EL2.VM（bit[0]），启用 EL1 和 EL0 的 Stage-2 地址转换。. */
#define HCR_SWIO            (1 << 1)   /* HCR_EL2.SWIO（bit[1]），控制 EL1 执行的缓存失效指令是否需要陷阱到 EL2 */
#define HCR_FMO             (1 << 3)   /* HCR_EL2.FMO（bit[3]），控制物理 FIQ（Fast Interrupt Request）路由 */
#define HCR_IMO             (1 << 4)   /* HCR_EL2.IMO（bit[4]），控制物理 IRQ（Interrupt Request）路由 */
#define HCR_RW              (1 << 31)  /* HCR_EL2.RW（bit[31]），指定 EL1 的执行状态 */
#define HCR_TSC             (1 << 19)  /* HCR_EL2.TSC（bit[19]），控制 EL1 的 SMC（Secure Monitor Call）指令是否陷阱到 EL2 */

void stage2_mmu_init(void);
void hyper_setup();
void create_guest_mapping(u64 *pgt, u64 va, u64 pa, u64 size, u64 mattr);
void page_unmap(u64 *pgt, u64 va, u64 size);
u64 *page_walk(u64 *pgt, u64 va, bool alloc);
void copy_to_ipa(u64 *pgt, u64 to_ipa, char *from, u64 len);
#endif