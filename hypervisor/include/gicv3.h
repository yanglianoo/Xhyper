#ifndef __GICV3_H__
#define __GICV3_H__

#include <types.h>
#include <layout.h>
#include <arch.h>

struct gic_irq_ops {
    const char *name;
    void (*mask)(u32 irq);
    void (*unmask)(u32 irq);
    void (*get_irq)(u32 *irq);
    void (*guest_eoi)(u32 irq);
    void (*hyp_eoi)(u32 irq);
    void (*dir)(u32 irq);
    void (*configure)(u32 irq, u32 type);
    void (*set_affinity)(u32 irq, u32 cpu_num);
    void (*set_priority)(u32 irq, u32 prio);
    void (*clear_pending)(u32 irq);
    void (*set_pending)(u32 irq);
    bool (*get_pending)(u32 irq);
    void (*set_active)(u32 irq);
};

struct gicv3_context {
    u64 gic_lr[16];
    u64 vmcr;
    u64 sre_el1;
};

#define GIC_NSGI              (16)
#define GIC_NPPI              (16)

#define GIC_MIN_SPI0          (32)   // SPI中断号开始位置
#define GIC_MIN_LPI           (8192) // LPI中断号开始位置

#define GIC_LEVEL_TRIGGER     (0) // 
#define GIC_EDGE_TRIGGER      (2) // 边沿触发

/* GIC Distributor block */
#define GICD_CTLR               (0x0)
#define GICD_TYPER              (0x4)
#define GICD_IIDR               (0x8)
#define GICD_TYPER2             (0xc)
// 这里的n代表第几个寄存器，每个寄存器32bit
#define GICD_IGROUPR(n)         (0x080  + (u64)(n) * 4) //中断分组寄存器，设置中断的组别（Group 0 用于 FIQ，Group 1 用于 IRQ）
#define GICD_ISENABLER(n)       (0x100  + (u64)(n) * 4) //中断使能寄存器，启用特定中断。
#define GICD_ICENABLER(n)       (0x180  + (u64)(n) * 4) //中断禁用寄存器，禁用特定中断
#define GICD_ISPENDR(n)         (0x200  + (u64)(n) * 4) //中断挂起寄存器，设置中断为挂起状态。
#define GICD_ICPENDR(n)         (0x280  + (u64)(n) * 4) //中断挂起清除寄存器，清除挂起状态。
#define GICD_ISACTIVER(n)       (0x300  + (u64)(n) * 4) //中断激活寄存器，设置中断为激活状态。
#define GICD_ICACTIVER(n)       (0x380  + (u64)(n) * 4) //中断激活清除寄存器，清除激活状态。
#define GICD_IPRIORITYR(n)      (0x400  + (u64)(n) * 4) //中断优先级寄存器，设置中断优先级。
#define GICD_ITARGETSR(n)       (0x800  + (u64)(n) * 4) //中断目标寄存器，指定中断的目标 CPU（GICv2 遗留，GICv3 部分支持）
#define GICD_ICFGR(n)           (0xc00  + (u64)(n) * 4) //中断配置寄存器，设置中断触发方式（边沿触发或电平触发）。
#define GICD_IROUTER(n)         (0x6000 + (u64)(n) * 8) //中断路由寄存器，指定中断的目标 CPU 或亲和性（Affinity）。

#define GICD_CTLR_ENABLE_G1A    (1U << 1) // 使能非安全 Group1 中断。
#define GICD_CTLR_ENABLE_G1     (1U << 0) // 使能 Group0 中断
#define GICD_CTLR_ARE_NA        (1U << 4)

#define GICD_TYPER_CPUNumber_SHIFT 5 // 目标 CPU 数量偏移

/* Redistributor GICR_* are percpu registers, the stride is GICR_STRIDE */
#define GICR_BASE_n(n)      (GICR_BASE + (n)*GICR_STRIDE)

//RD_base
#define GICR_CTLR           (0x0)
#define GICR_IIDR           (0x4)
#define GICR_TYPER          (0x8)
#define GICR_WAKER          (0x14)
#define GICR_PIDR2          (0xffe8)

/* SGI base is at 64K offset from Redistributor */
#define SGI_BASE            0x10000
#define GICR_IGROUPR0       (SGI_BASE+0x80)
#define GICR_ISENABLER0     (SGI_BASE+0x100)
#define GICR_ICENABLER0     (SGI_BASE+0x180)
#define GICR_ICPENDR0       (SGI_BASE+0x280)
#define GICR_ISACTIVER0     (SGI_BASE+0x300)
#define GICR_ICACTIVER0     (SGI_BASE+0x380)
#define GICR_IPRIORITYR(n)  (SGI_BASE+0x400+(n)*4)
#define GICR_ICFGR0         (SGI_BASE+0xc00)
#define GICR_ICFGR1         (SGI_BASE+0xc04)
#define GICR_IGRPMODR0      (SGI_BASE+0xd00)

/* ICH system register */
#define ICH_HCR_EL2         S3_4_C12_C11_0
#define ICH_VTR_EL2         S3_4_C12_C11_1
#define ICH_VMCR_EL2        S3_4_C12_C11_7
#define ICH_LR0_EL2         S3_4_C12_C12_0
#define ICH_LR1_EL2         S3_4_C12_C12_1
#define ICH_LR2_EL2         S3_4_C12_C12_2
#define ICH_LR3_EL2         S3_4_C12_C12_3
#define ICH_LR4_EL2         S3_4_C12_C12_4
#define ICH_LR5_EL2         S3_4_C12_C12_5
#define ICH_LR6_EL2         S3_4_C12_C12_6
#define ICH_LR7_EL2         S3_4_C12_C12_7
#define ICH_LR8_EL2         S3_4_C12_C13_0
#define ICH_LR9_EL2         S3_4_C12_C13_1
#define ICH_LR10_EL2        S3_4_C12_C13_2
#define ICH_LR11_EL2        S3_4_C12_C13_3
#define ICH_LR12_EL2        S3_4_C12_C13_4
#define ICH_LR13_EL2        S3_4_C12_C13_5
#define ICH_LR14_EL2        S3_4_C12_C13_6
#define ICH_LR15_EL2        S3_4_C12_C13_7

/* ICC system register */
#define ICC_IAR0_EL1        S3_0_C12_C8_0
#define ICC_IAR1_EL1        S3_0_C12_C12_0
#define ICC_EOIR0_EL1       S3_0_C12_C8_1
#define ICC_EOIR1_EL1       S3_0_C12_C12_1
#define ICC_HPPIR0_EL1      S3_0_C12_C8_2
#define ICC_HPPIR1_EL1      S3_0_C12_C12_2
#define ICC_BPR0_EL1        S3_0_C12_C8_3
#define ICC_BPR1_EL1        S3_0_C12_C12_3
#define ICC_DIR_EL1         S3_0_C12_C11_1
#define ICC_PMR_EL1         S3_0_C4_C6_0
#define ICC_RPR_EL1         S3_0_C12_C11_3
#define ICC_CTLR_EL1        S3_0_C12_C12_4
#define ICC_CTLR_EL3        S3_6_C12_C12_4
#define ICC_SRE_EL1         S3_0_C12_C12_5
#define ICC_SRE_EL2         S3_4_C12_C9_5
#define ICC_SRE_EL3         S3_6_C12_C12_5
#define ICC_IGRPEN0_EL1     S3_0_C12_C12_6
#define ICC_IGRPEN1_EL1     S3_0_C12_C12_7
#define ICC_SEIEN_EL1       S3_0_C12_C13_0
#define ICC_SGI0R_EL1       S3_0_C12_C11_7
#define ICC_SGI1R_EL1       S3_0_C12_C11_5
#define ICC_ASGI1R_EL1      S3_0_C12_C11_6

#define ICC_SRE_EL2         S3_4_C12_C9_5

#define LR_STATE(n)         (((n) & 0x3L) << 62)
#define LR_HW               (1L << 61)
#define LR_GROUP(n)         (((n) & 0x1L) << 60)
#define LR_PINTID(n)        (((n) & 0x3ffL) << 32) //物理中断ID
#define LR_VINTID(n)        ((n) & 0xffffffffL)    //虚拟中断ID

#define LR_PENDING          (1L)
#define LR_IS_INACTIVE(lr)  (((lr >> 62) & 0x3) == 0L)

/* GICD_* are all 32 bits register */
static inline u32 GICD_READ32(u32 offset)
{
    return *(volatile u32 *)(u64)(GICD_BASE + offset);
}

static inline void GICD_WRITE32(u32 offset, u32 val)
{
    *(volatile u32 *)(u64)(GICD_BASE + offset) = val;
}

static inline u32 GICR_READ32(int coreid, u32 offset)
{
    return *(volatile u32 *)(u64)(GICR_BASE_n(coreid) + offset);
}

static inline void GICR_WRITE32(int coreid, u32 offset, u32 val)
{
    *(volatile u32 *)(u64)(GICR_BASE_n(coreid) + offset) = val;
}

/* GICR_* have some 64 bits register */
static inline u64 GICR_READ64(int coreid, u32 offset)
{
    return *(volatile u64 *)(u64)(GICR_BASE_n(coreid) + offset);
}

static inline void GICR_WRITE64(int coreid, u32 offset, u32 val)
{
    *(volatile u64 *)(u64)(GICR_BASE_n(coreid) + offset) = val;
}

extern struct gic_irq_ops gicv3_ops;
extern int gic_max_lrs;
extern int gic_max_spi;

void gic_v3_init(void);
void gic_percpu_init(void);
void hyper_spi_config(u32 irq, u32 type);
void gic_context_init(struct gicv3_context *gic_context);
void restore_gic_context(struct gicv3_context *gic_context);
u64  gic_read_list_reg(int n);
void gic_write_list_reg(int n, u64 val);
u64  gic_create_lr(u32 pirq, u32 virq);

#endif