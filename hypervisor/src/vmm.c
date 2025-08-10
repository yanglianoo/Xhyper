#include "printf.h"
#include "types.h"
#include "layout.h"
#include "arch.h"
#include "kalloc.h"
#include "vmm.h"
#include "xlog.h"

/*
遍历 Stage-2 页表（从 L0 到 L2），找到或创建 L3 页表项。
*/
u64 *page_walk(u64 *pgt, u64 va, bool alloc)
{
    /* page walk from L0 ~ L2 */
    for(int table_level = 0; table_level < 3; table_level ++) {
        // 每级通过 PINDEX(table_level, va)计算索引，从虚拟地址提取对应级的索引位。
        u64 *pte = &pgt[PINDEX(table_level, va)];

        /* Page table entry has been mapped to next-level Page table */
        if((*pte & PTE_VALID) && (*pte & PTE_TABLE)) {
            /* Next-Table address has been alloced
             * 63        47                       12          2   1   0
             * +--------+--------------------------+----------+---+---+
             * |        | Next-level table addr    | IGNORED  | 1 | 1 |
             * +--------+--------------------------+----------+---+---+
             */
            /* Get next-level page table address */
            pgt = (u64 *)PTE_PA(*pte);
        } else if(alloc == true) {
            /* Page table entry is not mapped, alloc one page from next-level page table */
            pgt = alloc_one_page();
            if(pgt == NULL) {
                abort("Unable to alloc one page for page_walk");
            }
            *pte = PTE_PA(pgt) | PTE_TABLE | PTE_VALID;
        } else {
            return NULL;
        }
    }

    /* L3 PTE */
    return &pgt[PINDEX(3, va)];
}

/*  为虚拟机的虚拟地址范围 [va, va + size) 创建 Stage-2 页面映射，映射到物理地址 [pa, pa + size)。
    pgt：Stage-2 页面表的基地址（通常由 VTTBR_EL2 提供）。
    va：虚拟机的虚拟地址（Virtual Address）。
    pa：物理地址（Physical Address），要映射到的目标地址。
    size：映射的大小（字节）。
    mattr：内存属性索引，指向 MAIR_EL2 的某个 Attr（例如 DEVICE_nGnRnE 或 NORMAL_NC）
*/
void create_guest_mapping(u64 *pgt, u64 va, u64 pa, u64 size, u64 mattr)
{
    /* va, pa, size shall be page alignment */
    if(va % PAGESIZE != 0 ||
       pa % PAGESIZE != 0 ||
       size % PAGESIZE != 0) {
        abort("Create_guest_mapping with invalid param");
    }

    for(u64 p = 0; p < size; p += PAGESIZE, va += PAGESIZE, pa += PAGESIZE) {
        u64 *pte = page_walk(pgt, va, true);
        if(*pte & PTE_AF) {
            abort("Page table entry has been used");
        }
        *pte = PTE_PA(pa) | PTE_AF | mattr | PTE_V;
    }
}

void page_unmap(u64 *pgt, u64 va, u64 size)
{
    if(va % PAGESIZE != 0 || size % PAGESIZE != 0) {
        abort("Page_unmap with invalid param");
    }
    
    for(u64 p = 0; p < size; p += PAGESIZE, va += PAGESIZE) {
        u64 *pte = page_walk(pgt, va, false);
        if(*pte == 0) {
            abort("Page already unmapped");
        }
        u64 pa = PTE_PA(*pte);
        free_one_page((void *)pa);
        *pte = 0;
    }
}

void stage2_mmu_init(void)
{
    LOG_INFO("Stage2 Translation MMU initialization ...\n");

    /* Physical Address range supported */
    u64 feature;
    read_sysreg(feature, id_aa64mmfr0_el1);
    LOG_INFO("PARange bits is %d\n", PA_RANGE(feature));

    /* 配置 VTCR_EL2 寄存器
     * T0SZ = 64 - 20 = 44 : (IPA range is 2^(64-20) = 2^44)
     * SL0  = 2 : starting level is leve-0
     * TG0  = 0 : 4K Granule size
     * PS   = 1 : Physical address Size is 36 bits
     */
    u64 vtcr = VTCR_T0SZ(20) | VTCR_SL0(2) |
               VTCR_SH0(0) | VTCR_TG0(0) | VTCR_NSW |
               VTCR_NSA | VTCR_PS(4);

    LOG_INFO("Setting vtcr_el2 to 0x%x\n", vtcr);
    write_sysreg(vtcr_el2, vtcr);

    /* 配置内存属性
        Attr0（索引 0）：对应 Device-nGnRnE 内存类型，值为 0x0。
        Attr1（索引 1）：对应 Normal Non-Cacheable 内存类型，值为 0x44。
    */ 
    u64 mair = (DEVICE_nGnRnE << (8 * DEVICE_nGnRnE_INDEX)) | (NORMAL_NC << (8 * NORMAL_NC_INDEX));
    LOG_INFO("Setting mair_el2 to 0x%x\n", mair);
    write_sysreg(mair_el2, mair);

    isb();

    return;
}

/* Provides configuration controls for virtualization */
extern void hyper_vector();
void hyper_setup()
{   
    /*
        HCR_TSC : 控制虚拟机的 SMC（Secure Monitor Call）指令是否陷入 Hypervisor
        HCR_RW  : 决定虚拟机的执行状态是 AArch64 还是 AArch32
        HCR_VM  : 开启或关闭 Stage-2 地址转换
    */
    u64 hcr = HCR_TSC | HCR_RW | HCR_VM;
    LOG_INFO("Setting hcr_el2 to 0x%x and enable stage 2 address translation\n");
    write_sysreg(hcr_el2, hcr);

    LOG_INFO("Setting Vector Base Address Register for EL2\n");
    write_sysreg(vbar_el2, (u64)hyper_vector);

    isb();
}

