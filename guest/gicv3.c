#include <types.h>
#include <printf.h>
#include <gicv3.h>
#include <arch.h>


#include "types.h"
#include "printf.h"
#include "gicv3.h"
#include "arch.h"

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
    gic_irqs  = ((gicd_type & 0x1F) + 1) * 32;

    nr_regs = (gic_irqs + 31) / 32;
    /* irq 0 ~ 31 for SGIs and PPIs */
    for(i = 1; i < nr_regs; i++) {
        /* Set interrupt is Non-secure Group 1 */
        GICD_WRITE32(GICD_IGROUPR(i), ~((u32)(0)));
        /* Disable all IRQs */
        GICD_WRITE32(GICD_ICENABLER(i), ~((u32)(0)));
        /* Clear all active IRQs */
        GICD_WRITE32(GICD_ICACTIVER(i), ~((u32)(0)));
        /* Clear all active IRQs */
        GICD_WRITE32(GICD_ICPENDR(i), ~((u32)(0)));
    }

    /* Set default trigger type for all spi as level triggered */
    nr_regs = (gic_irqs + 15) / 16;
    for(i = 1; i < nr_regs; i++) {
        GICD_WRITE32(GICD_ICFGR(i), 0);
    }

    /* Before Enable Distributor, we need to wait RWP */
    gic_dist_wait_for_rwp();

    /* Enabel Distributor */
    GICD_WRITE32(GICD_CTLR, (GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1));
    gic_dist_wait_for_rwp();

    return;
}

static void gic_redist_init(void)
{
    u32 waker;
    int cpu = coreid();

    /************ gic restributor configuration **********/

    /* Enable GIC Restributor */
    waker = GICR_READ32(cpu, GICR_WAKER);
    /* This PE is not in, and is not entering, a low power state. */
    GICR_WRITE32(cpu, GICR_WAKER, (waker & ~(1 << 1)));
    /* Wait until An interface to the connected PE might be active */
    while(GICR_READ32(cpu, GICR_WAKER) & (1 << 2));

    /* Configure SIGs and PPIs as non-secure Group 1 */
    GICR_WRITE32(cpu, GICR_IGROUPR0, ~((u32)(0)));
    GICR_WRITE32(cpu, GICR_IGRPMODR0, 0);

    /* Disable all SGIs/PPIs */
    GICR_WRITE32(cpu, GICR_ICENABLER0, ~((u32)(0)));
    /* Clear all active SGIs/PPIs */
    GICR_WRITE32(cpu, GICR_ICACTIVER0, ~((u32)(0)));
    /* Clear all pending SGIs/PPIs */
    GICR_WRITE32(cpu, GICR_ICPENDR0, ~((u32)(0)));

    /* Configure PPIs are as level triggered type */
    /* SGIs are set by hardware to be edge-triggered only */
    GICR_WRITE32(cpu, GICR_ICFGR1, 0);

    return;
}

static void gic_icc_init(void)
{
    u32 sre;

    /************ cpu interface configuration ***********/

    /* Enable sysrem register */
    read_sysreg(sre, ICC_SRE_EL1);
    write_sysreg(ICC_SRE_EL1, sre | 1);

    isb();

    /* Set the idle priority as the priority mask to allow all unterrupts */
    write_sysreg(ICC_PMR_EL1, 0xFF);
    /* Set binary points, only for Group 1 */
    write_sysreg(ICC_BPR1_EL1, 0x7);
    /* Set EOImode as split mode */
    write_sysreg(ICC_CTLR_EL1, (1 << 1));
    /* Enable SGIs/PPIs as Ns-Group 1 */
    write_sysreg(ICC_IGRPEN1_EL1, 1);

    return;
}

/* Initialize GIC Restributor and CPU interface */
void gic_percpu_init(void)
{
    gic_redist_init();
    gic_icc_init();
}

/* Disable IRQ for SPIs/SGIs/PPIs */
static void gic_disable_int(u32 irq)
{
    u32 reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_LPI) {
        printf("Guest don't support LPIs\n");
        return;
    }

    if(irq >= GIC_MIN_SPI0) {  /* Configure for SPIs */
        reg = GICD_READ32(GICD_ICENABLER(irq / 32));
        reg |= 1U << (irq % 32);
        GICD_WRITE32(GICD_ICENABLER(irq / 32), reg);
    } else {  /* Configure for SGIs/PPIs */
        reg = GICR_READ32(cpu, GICR_ICENABLER0);
        reg |= 1U << (irq %32);
        GICR_WRITE32(cpu, GICR_ICENABLER0, reg);
    }
}

/* Enable IRQ for SPIs/SGIs/PPIs */
static void gic_enable_int(u32 irq)
{
    u32 reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_LPI) {
        printf("Guest don't support LPIs\n");
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

/* Get irq number */
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

static void gic_set_target(u32 irq, u32 target)
{
    u32 reg;

    if(irq < GIC_MIN_SPI0) {
        return;
    }

    if(irq >= GIC_MIN_LPI) {
        printf("Guest don't support LPIs\n");
        return;
    }

    reg = GICD_READ32(GICD_ITARGETSR(irq / 4));
    reg &= ((u32)0xFF << (irq % 4 * 8));
    GICD_WRITE32(GICD_ITARGETSR(irq / 4), reg | (target << (irq % 4 * 8)));
}

static void gic_set_config(u32 irq, u32 config)
{
    u32 shift, reg;
    int cpu = coreid();
    if(irq >= GIC_MIN_SPI0) {
        shift = (irq % 16) * 2;
        reg = GICD_READ32(GICD_ICFGR(irq / 16));
        reg &= ~( ( (u32)(0x03) ) << shift );
        reg |= ( ( (u32)config ) << shift );
        GICD_WRITE32(GICD_ICFGR(irq / 16), reg);
    } else {
        shift = (irq % 16) * 2;
        reg = GICR_READ32(cpu, GICR_ICFGR1);
        reg &= ~( ( (u32)(0x03) ) << shift );
        reg |= ( ( (u32)config ) << shift );
        GICR_WRITE32(cpu, GICR_ICFGR1, reg);
    }
}

static void gic_set_priority(u32 irq, u32 prio)
{
    u32 reg;
    int cpu = coreid();
    (void)prio;
    if(irq >= GIC_MIN_LPI) {
        printf("Guest don't support LPIs\n");
        return;
    }

    if(irq >= GIC_MIN_SPI0) {
        reg = GICD_READ32(GICD_IPRIORITYR(irq / 4));
        reg &= ~((u32)0xff << (irq % 4 * 8));
        GICD_WRITE32(GICD_IPRIORITYR(irq / 4), reg);
    } else {
        reg = GICR_READ32(cpu, GICR_IPRIORITYR(irq / 4));
        reg &= ~((u32)0xff << (irq % 4 * 8));
        GICR_WRITE32(cpu, GICR_IPRIORITYR(irq / 4), reg);
    }
}

static void gic_guest_eoi(u32 irq)
{
    write_sysreg(ICC_EOIR1_EL1, irq);
    write_sysreg(ICC_DIR_EL1, irq);
}

struct gic_irq_ops gicv3_ops = {
    .name         = "ARM GICv3",
    .mask         = gic_disable_int,
    .unmask       = gic_enable_int,
    .get_irq      = gic_get_iar,
    .guest_eoi    = gic_guest_eoi,
    .dir          = gic_dir,
    .set_affinity = gic_set_target,
    .configure    = gic_set_config,
    .set_priority = gic_set_priority,
};

void gic_v3_init(void)
{
    gic_dist_init();
    gic_percpu_init();
}

void guest_spi_config(u32 irq, u32 type)
{
    gicv3_ops.set_affinity(irq, 0);
    gicv3_ops.set_priority(irq, 0);
    gicv3_ops.configure(irq, type);
    gicv3_ops.unmask(irq);
}


