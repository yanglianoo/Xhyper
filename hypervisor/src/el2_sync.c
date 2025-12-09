#include <arch.h>
#include <printf.h>
#include <types.h>
#include <xlog.h>
#include <vcpu.h>
#include <vpsci.h>
#include <vmmio.h>
#include <gicv3.h>
#include <pl011.h>

void el2_irq_proc(void)
{
    u32 iar, irq;
    /* 获取ICC_IAR1_EL1寄存器的值 */
    gicv3_ops.get_irq(&iar);
    /* 取出中断ID的值 0x3ff =0b 11 1111 1111 */
    irq = iar & 0x3FF;

    switch(irq) {
        case UART_IRQ_LINE:
            pl011_irq_handler();
        break;
        default:
            LOG_WARN("Unknow IRQ under EL2, irq is %d\n", irq);
            break;
    }

    gicv3_ops.hyp_eoi(irq);
}