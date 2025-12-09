#include <vm.h>
#include <types.h>
#include <xlog.h>
#include <vcpu.h>
#include <kalloc.h>
#include <layout.h>
#include <guest.h>
#include <utils.h>
#include <arch.h>
#include <xmalloc.h>
#include <vmm.h>
#include <printf.h>
#include <vmmio.h>
#include <gicv3.h>

int gic_max_lrs = 0;
int gic_max_spi = 0;

//初始化虚拟GICv3接口
void gic_context_init(struct gicv3_context *gic_context)
{
    read_sysreg(gic_context->vmcr, ICH_VMCR_EL2);
}

void restore_gic_context(struct gicv3_context *gic_context)
{
    u32 sre;
    write_sysreg(ICH_VMCR_EL2, gic_context->vmcr);
    read_sysreg(sre, ICC_SRE_EL1);
    write_sysreg(ICC_SRE_EL1, sre | gic_context->sre_el1);
}

//读取ICH_LR<n>_EL2寄存器的数据
u64 gic_read_list_reg(int n)
{
    u64 val;
    switch(n) {
        case 0:   read_sysreg(val, ICH_LR0_EL2); break;
        case 1:   read_sysreg(val, ICH_LR1_EL2); break;
        case 2:   read_sysreg(val, ICH_LR2_EL2); break;
        case 3:   read_sysreg(val, ICH_LR3_EL2); break;
        case 4:   read_sysreg(val, ICH_LR4_EL2); break;
        case 5:   read_sysreg(val, ICH_LR5_EL2); break;
        case 6:   read_sysreg(val, ICH_LR6_EL2); break;
        case 7:   read_sysreg(val, ICH_LR7_EL2); break;
        case 8:   read_sysreg(val, ICH_LR8_EL2); break;
        case 9:   read_sysreg(val, ICH_LR9_EL2); break;
        case 10:  read_sysreg(val, ICH_LR10_EL2); break;
        case 11:  read_sysreg(val, ICH_LR11_EL2); break;
        case 12:  read_sysreg(val, ICH_LR12_EL2); break;
        case 13:  read_sysreg(val, ICH_LR13_EL2); break;
        case 14:  read_sysreg(val, ICH_LR14_EL2); break;
        case 15:  read_sysreg(val, ICH_LR15_EL2); break;
        default:  abort("Unknow ICH LR number");
    }

    return val; 
}

//写入ICH_LR<n>_EL2寄存器的数据
void gic_write_list_reg(int n, u64 val)
{
    switch(n) {
        case 0:   write_sysreg(ICH_LR0_EL2, val); break;
        case 1:   write_sysreg(ICH_LR1_EL2, val); break;
        case 2:   write_sysreg(ICH_LR2_EL2, val); break;
        case 3:   write_sysreg(ICH_LR3_EL2, val); break;
        case 4:   write_sysreg(ICH_LR4_EL2, val); break;
        case 5:   write_sysreg(ICH_LR5_EL2, val); break;
        case 6:   write_sysreg(ICH_LR6_EL2, val); break;
        case 7:   write_sysreg(ICH_LR7_EL2, val); break;
        case 8:   write_sysreg(ICH_LR8_EL2, val); break;
        case 9:   write_sysreg(ICH_LR9_EL2, val); break;
        case 10:  write_sysreg(ICH_LR10_EL2, val); break;
        case 11:  write_sysreg(ICH_LR11_EL2, val); break;
        case 12:  write_sysreg(ICH_LR12_EL2, val); break;
        case 13:  write_sysreg(ICH_LR13_EL2, val); break;
        case 14:  write_sysreg(ICH_LR14_EL2, val); break;
        case 15:  write_sysreg(ICH_LR15_EL2, val); break;
        default:  abort("Unknow ICH LR number");
    }
}

u64 gic_create_lr(u32 pirq, u32 virq)
{
    /*
     * The state of the interrupt is pending
     * The interrupt maps directly to a hardware interrupt
     * This is a Group 1 virtual interrupt
     */
    return LR_STATE(LR_PENDING) | LR_HW | LR_GROUP(1) | LR_PINTID(pirq) | LR_VINTID(virq);
}

/* 读取GICD_CTLR寄存器的bit[31]，确认GICD_CTLR寄存器写入是否有效*/
static inline void gic_dist_wait_for_rwp(void)
{
    while((GICD_READ32(GICD_CTLR) >> 31) & 0x1);
}

static void gic_dist_init(void)
{
    u32 gicd_type, gic_irqs;
    s32 i, nr_regs;

    /* Disable Distributor */
    GICD_WRITE32(GICD_CTLR, 0);
    gic_dist_wait_for_rwp();  
    
    gicd_type = GICD_READ32(GICD_TYPER);
    /* 0x1f= 0b11111 计算支持的最大SPI*/
    gic_irqs  = ((gicd_type & 0x1F) + 1) * 32;
    gic_max_spi = gic_irqs - 1;
    
    /* 用于计算控制所有中断所需的寄存器数量（每个寄存器控制 32 个中断），这里+31是向上取整。*/
    nr_regs = (gic_irqs + 31) / 32;

    for(i = 1; i < nr_regs; i++) {
        /* Set interrupt is Non-secure Group 1 */
        GICD_WRITE32(GICD_IGROUPR(i), ~((u32)(0)));   // 中断分组寄存器写入全 1 表示将这些 SPI 中断分配到 非安全 Group 1
        /* Disable all IRQs */
        GICD_WRITE32(GICD_ICENABLER(i), ~((u32)(0))); //中断禁用寄存器，写入全 1 表示禁用所有这些 SPI 中断（初始化阶段默认禁用，避免误触发）。
        /* Clear all active IRQs */
        GICD_WRITE32(GICD_ICACTIVER(i), ~((u32)(0))); //清除活跃状态寄存器，写入全 1 表示清除这些中断的 “活跃” 状态（确保初始化前无残留活跃标记）。
        /* Clear all active IRQs */
        GICD_WRITE32(GICD_ICPENDR(i), ~((u32)(0)));   //清除挂起状态寄存器，写入全 1 表示清除这些中断的 “挂起” 状态（确保初始化前无残留挂起标记）。
    }

    /* Set default trigger type for all spi as level triggered */
    /* 配置中断触发方式 */
    nr_regs = (gic_irqs + 15) / 16;
    for(i = 1; i < nr_regs; i++) {
        GICD_WRITE32(GICD_ICFGR(i), 0);
    }

    /* Before Enable Distributor, we need to wait RWP */
    gic_dist_wait_for_rwp();

    /* Enabel Distributor */
    GICD_WRITE32(GICD_CTLR, (GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1));
    gic_dist_wait_for_rwp();
}

/* 初始化Redistributor GICR_* */
static void gic_redist_init(void)
{
    u32 waker, sre;
    int cpu = coreid();

    /************ gic restributor configuration **********/

    /* Enable GIC Restributor */
    waker = GICR_READ32(cpu, GICR_WAKER);
    /*将 ProcessorSleep 位（bit [1]）清零，表示处理器（PE, Processing Element）不处于且不进入低功耗状态。*/
    GICR_WRITE32(cpu, GICR_WAKER, (waker & ~(1 << 1)));
    /* 轮询 GICR_WAKER 的 ChildrenAsleep 位（bit [2]），等待其变为 0。*/
    while(GICR_READ32(cpu, GICR_WAKER) & (1 << 2));

    /* Configure SIGs and PPIs as non-secure Group 1 */
    GICR_WRITE32(cpu, GICR_IGROUPR0, ~((u32)(0)));
    GICR_WRITE32(cpu, GICR_IGRPMODR0, 0);
    /* 禁用所有 SGI/PPI 中断 */
    GICR_WRITE32(cpu, GICR_ICENABLER0, ~((u32)(0)));
    /* 清除所有 SGI/PPI 中断的激活态 */
    GICR_WRITE32(cpu, GICR_ICACTIVER0, ~((u32)(0)));
    /* 清楚所有 SGI/PPI 中断的挂起态 */
    GICR_WRITE32(cpu, GICR_ICPENDR0, ~((u32)(0)));

    GICR_WRITE32(cpu, GICR_ICFGR1, 0);

    return;
}

/* 初始化cpu interface */
static void gic_icc_init(void)
{
    u32 sre;
    int cpu = coreid();
    /************ cpu interface configuration ***********/

    /* Enable sysrem register */
    read_sysreg(sre, ICC_SRE_EL2);

    /* bit 3: EL1 accesses to ICC_SRE_EL1 do not trap to EL2.
     * bit 0: The System register interface to the ICH_* registers
     * and the EL1 and EL2 ICC_* registers is enabled for EL2
     */
    write_sysreg(ICC_SRE_EL2, sre | (1 << 3) | 1);

    /* make sure ICC_SRE_EL2.SRE already set to 1, must? */
    isb();

    read_sysreg(sre, ICC_SRE_EL1);

    /* The System register interface for the current Security state is enabled. */
    /* 使能EL1下对系统寄存器的访问*/
    write_sysreg(ICC_SRE_EL1, sre | 1);

    /* Set the idle priority as the priority mask to allow all unterrupts */
    /* 设置所有 ICC的中断都可触发*/
    write_sysreg(ICC_PMR_EL1, 0xFF);

    /* Set binary points, only for Group 1 */
    /* 设置所有中断可抢占 */
    write_sysreg(ICC_BPR1_EL1, 0x7);

    /* Set EOImode as split mode */
    write_sysreg(ICC_CTLR_EL1, (1 << 1));

    /* Enable SGIs/PPIs as Ns-Group 1 */
    write_sysreg(ICC_IGRPEN1_EL1, 1);

    return;
}

// 初始化虚拟cpu interface接口
static void gic_hyp_init(void)
{
    u64 vtr;
    /* Virtual Group 1 interrupts are enabled. */
    write_sysreg(ICH_VMCR_EL2, (1<<1));
    /* Virtual CPU interface operation enabled. */
    write_sysreg(ICH_HCR_EL2, (1<<0));
    /* The number of implemented List registers - 1 */
    read_sysreg(vtr, ICH_VTR_EL2);
    gic_max_lrs = ((vtr & 0x1F) + 1);

    return;
}

void gic_percpu_init(void)
{   
    // 初始化Redistributor
    gic_redist_init();
    // 初始化cpu interface
    gic_icc_init();
    // 初始化虚拟cpu interface
    gic_hyp_init();
}

/* Disable IRQ for SPIs/SGIs/PPIs */
static void gic_disable_int(u32 irq)
{
    u32 reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_LPI) {
        LOG_WARN("X-Hyper don't support LPIs\n");
        return;
    }

    if(irq >= GIC_MIN_SPI0) {
        /* Configure for SPIs */
        reg = GICD_READ32(GICD_ICENABLER(irq / 32));
        reg |= 1U << (irq % 32);
        GICD_WRITE32(GICD_ICENABLER(irq / 32), reg);
    } else {
        /* Configure for SGIs/PPIs */
        reg = GICR_READ32(cpu, GICR_ICENABLER0);
        reg |= 1U << (irq %32);
        GICR_WRITE32(cpu, GICR_ICENABLER0, reg);
    }
}

// 开启中断
static void gic_enable_int(u32 irq)
{
    u32 reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_LPI) {
        LOG_WARN("X-Hyper don't support LPIs\n");
        return;
    }

    if(irq >= GIC_MIN_SPI0) {  /* Configure for SPIs */
        reg = GICD_READ32(GICD_ISENABLER(irq / 32));
        reg |= 1U << (irq % 32);
        GICD_WRITE32(GICD_ISENABLER(irq / 32), reg);
    } else {  /* Configure for SGIs/PPIs */
        reg = GICR_READ32(cpu, GICR_ISENABLER0);
        reg |= 1U << (irq %32);
        GICR_WRITE32(cpu, GICR_ISENABLER0, reg);
    }
}

/*从 cpu interface 接口获取当前最高优先级中断的ID */
static void gic_get_iar(u32 *irq)
{
    if(irq == NULL) {
        return;
    } else {
        read_sysreg(*irq, ICC_IAR1_EL1);
    }
}

static void gic_dir(u32 irq)
{
    write_sysreg(ICC_DIR_EL1, irq);
}

static void gic_hyp_eoi(u32 irq)
{
    /* EOI = end of interrupt */
    /* 完成对irq中断的处理，写ICC_EOIR1_EL1寄存器，标志着结束*/
    write_sysreg(ICC_EOIR1_EL1, irq);
    /* 清楚irq中断的Active 状态 */
    write_sysreg(ICC_DIR_EL1, irq);
}

// 设置irq被路由到哪个(Processing Element)。通过target来设置
static void gic_set_target(u32 irq, u32 target)
{
    u32 reg;
    int cpu = coreid();

    if(irq < GIC_MIN_SPI0) {
        return;
    }

    if(irq >= GIC_MIN_LPI) {
        LOG_WARN("X-Hyper don't support LPIs\n");
        return;
    }

    reg = GICD_READ32(GICD_ITARGETSR(irq / 4));
    reg &= ((u32)0xFF << (irq % 4 * 8));
    GICD_WRITE32(GICD_ITARGETSR(irq / 4), reg | (target << (irq % 4 * 8)));

}

/* 
配置irq中断号对应的中断触发类型：
     - 电平触发
     - 边沿触发 
*/
static void gic_set_config(u32 irq, u32 config)
{
    u32 shift, reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_SPI0) { // 配置SPI中断
        shift = (irq % 16) * 2;
        reg = GICD_READ32(GICD_ICFGR(irq / 16));
        reg &= ~( ( (u32)(0x03) ) << shift );
        reg |= ( ( (u32)config ) << shift );
        GICD_WRITE32(GICD_ICFGR(irq / 16), reg);
    } else { // 配置 PPI中断
        shift = (irq % 16) * 2;
        reg = GICR_READ32(cpu, GICR_ICFGR1);
        reg &= ~( ( (u32)(0x03) ) << shift );
        reg |= ( ( (u32)config ) << shift );
        GICR_WRITE32(cpu, GICR_ICFGR1, reg);
    }
}

/* 
 * 处理EL1下的中断结束，写入ICC_EOIR1_EL1寄存器，标志着结束
 * 这个函数在EL2下被调用
 */
static void gic_guest_eoi(u32 irq)
{
    write_sysreg(ICC_EOIR1_EL1, irq);
}

struct gic_irq_ops gicv3_ops =  {
    .name         = "ARM GICv3",
    .mask         = gic_disable_int,
    .unmask       = gic_enable_int,
    .get_irq      = gic_get_iar,
    .hyp_eoi      = gic_hyp_eoi,
    .guest_eoi    = gic_guest_eoi, 
    .dir          = gic_dir,
    .set_affinity = gic_set_target,
    .configure    = gic_set_config,
};

void gic_v3_init(void)
{
    /* 使能distributor */ 
    gic_dist_init();

    gic_percpu_init();
}

void hyper_spi_config(u32 irq, u32 type)
{
    // 设置 irq 被路由到 0 号PE 
    gicv3_ops.set_affinity(irq, 0);
    // 配置 irq 的中断触发类型
    gicv3_ops.configure(irq, type);
    // Enable irq中断
    gicv3_ops.unmask(irq);
}