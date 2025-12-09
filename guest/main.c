#include "types.h"
#include "pl011.h"
#include "printf.h"
#include "config.h"
#include "gicv3.h"
#include "arch.h"

__attribute__((aligned(SZ_4K))) char sp_stack[SZ_4K * NCPU] = {0};

extern void _start();
extern u64 smc_call(u64 funid, u64 target_cpu, u64 entrypoint);
extern void vectable();

void el1_sync() {
    u32 iar, irq;
    gicv3_ops.get_irq(&iar);
    irq = iar & 0x3ff;

    printf("Guest EL1 IRQ Handler, irq is %d\n", irq);

    switch(irq) {
        case UART_IRQ_LINE:
            pl011_irq_handler();
            break;
        default:
            printf("Guest EL1 IRQ unknow irq number\n");
            break;
    }

    gicv3_ops.guest_eoi(irq);
}

void print_logo(void)
{
    printf(" V     V     M     M      II  \n");
    printf("  V   V      MM   MM      II  \n");
    printf("   V V       M M M M      II  \n");
    printf("    V        M  M  M      II  \n");
    printf("    V        M     M      II  \n");
    printf("\n");
}

int vm_secondary_init()
{
    write_sysreg(vbar_el1, (unsigned long)vectable);
    gic_percpu_init();
    irq_enable;

    while(1) {
        printf("I am vm 1 on core %d\n", coreid());
        for(int i=0; i < 100000000; i++);
    }
}

int vm_primary_init()
{
    pl011_init();
    print_logo();

    write_sysreg(vbar_el1, (unsigned long)vectable);

    /* test code for wakeup vcore 1 */
    smc_call((u64)0xc4000003, (u64)1, (u64)_start);

    gic_v3_init();
    guest_spi_config(UART_IRQ_LINE, GIC_EDGE_TRIGGER);
    irq_enable;

    while(1) {
        printf("I am vm 1 on core %d\n", coreid());
        for(int i=0; i < 100000000; i++);
    }


    return 0;
}