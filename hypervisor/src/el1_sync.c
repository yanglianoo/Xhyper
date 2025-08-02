#include <arch.h>
#include <printf.h>
#include <types.h>
#include <xlog.h>
#include <vcpu.h>
#include <vpsci.h>  


static void vpsci_handler(vcpu_t *vcpu)
{
    /*
     * x0 - function id
     * x1 - target cpu
     * x2 - entry addr (the entry addr for guest vm, not for vmm)
     */

    u64 ret = vpsci_trap_smc(vcpu, vcpu->regs.x[0], vcpu->regs.x[1], vcpu->regs.x[2]);
    vcpu->regs.x[0] = ret;
}

static int hvc_smc_handler(vcpu_t *vcpu, int imm)
{
    switch(imm) {
        case 0:
            vpsci_handler(vcpu);
            return 0;
        default:
            return -1;
    } 
}

void el1_sync_proc()
{
    vcpu_t *vcpu;
    /* which vcpu has been trapped into EL2 */
    read_sysreg(vcpu, tpidr_el2);

    u64 esr, elr, far;
    /* Exception Syndrome Register */
    read_sysreg(esr, esr_el2);
    /* Exception Link Register */
    read_sysreg(elr, elr_el2);
    /* Holds the faulting Virtual Address */
    read_sysreg(far, far_el2);

    /* Exception Class  取esr_el2寄存器的 26-31位，即EC字段*/
    u64 esr_ec  = (esr >> 26) & 0x3F;
    /* Instruction Specific Syndrome  取esr_el2寄存器0-24位，即ISS字段*/
    u64 esr_iss = esr & 0x1FFFFFF;
    /* Instruction Length 取esr_el2寄存器的25位，即IL字段 */
    u64 esr_il  = (esr >> 25) & 0x1;

    switch(esr_ec) {

        /* HVC instruction execution in AArch64 state, when HVC is not disabled. */
        /* 64位环境下执行HVC（Hypervisor Call）指令触发的异常。用于虚拟机监控模式（EL2），虚拟机通过此指令与Hypervisor交互。*/
        case 0x16:
            LOG_INFO("\033[32m [el1_sync_proc] hvc trap from EL1\033[0m\n");
            if(hvc_smc_handler(vcpu, esr_iss) != 0) {
                abort("Unknown HVC call #%d", esr_iss);
            }
            /* hvc from EL1 will set the preferred exception return address to pc+4 */
            vcpu->regs.elr += 0;
            break;
        /* 64位环境下执行SMC（Secure Monitor Call）指令触发的异常。用于安全监控模式（EL3），实现安全世界与非安全世界的切换*/
        case 0x17:
            LOG_INFO("\033[32m[el1_sync_proc] smc trap from EL1\033[0m\n");
            /* on smc call, iss is the imm of a smc */
            if(hvc_smc_handler(vcpu, esr_iss) != 0) {
                abort("Unknown SMC call #%d", esr_iss);
            }
            /* smc trapped from EL1 will set preferred execption return address to pc
             * so we need to +4 return to the next instruction.
             */
            vcpu->regs.elr += 4;
            break;
        default:
            abort("Unknown el1 sync: esr_ec %p, esr_iss %p, elr %p, far %p", esr_ec, esr_iss, elr, far);
            break;
    }

    return;
}