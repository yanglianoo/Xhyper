#include <arch.h>
#include <printf.h>
#include <types.h>
#include <xlog.h>
#include <vcpu.h>
#include <vpsci.h>  
#include <vgicv3.h>

#define SYSREG_OPCODE(op0, op1, crn, crm, op2) \
     ((op0 << 20) | (op2 << 17) | (op1 << 14) | (crn << 10) | (crm << 1))

#define VSYSREG_ICC_SGI1R_EL1   SYSREG_OPCODE(3, 0, 12, 11, 5)

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

static u64 get_fault_ipa(u64 vaddr)
{
    u64 hpfar;
    read_sysreg(hpfar, hpfar_el2);
    // 取出hpfar_el2寄存器的低40位，然后左移8位
    u64 ipa_page = (hpfar & 0xfffffffffff) << 8;
    // 加上页面偏移地址，就是实际的ipa fault地址
    return ipa_page | (vaddr & (PAGESIZE - 1));
}

static int data_abort_handler(vcpu_t *vcpu, u64 esr_iss, u64 far)
{
    u64 ipa = get_fault_ipa(far);

    int wnr = (esr_iss >> 6) & 0x1;   /* write or read */
    int sas = (esr_iss >> 22) & 0x3;  /* Syndrome Access Size */
    int srt = (esr_iss >> 16) & 0x1f; /* The register number of the Wt/Xt/Rt */
    int fnv = (esr_iss >> 10) & 0x1;  /* far is valid or not valid */

    if(fnv != 0) {
        abort("FAR is not valid");
    }  

    enum access_size accesz;

    switch(sas) {
        case 0: accesz = VMMIO_BYTE; break;       //字节
        case 1: accesz = VMMIO_HALFWORD; break;   //半字
        case 2: accesz = VMMIO_WORD; break;       //字
        case 3: accesz = VMMIO_DOUBLEWORD; break; //双字
        default: abort("Unknow SAS from esr_el2");
    }

    struct vmmio_access vmmio_acs;

    vmmio_acs.ipa = ipa;
    vmmio_acs.accsize = accesz;
    vmmio_acs.wnr = wnr;

    if(vmmio_handler(vcpu, srt, &vmmio_acs) < 0) {
        LOG_WARN("VMMIO handler failed: ipa %p, va %p\n", ipa, far);
        return -1;
    }

    return 0;
}

int vsysreg_handler(vcpu_t *vcpu, u64 iss)
{
    int write_not_read = !(iss & 1);
    /* The Rt value from the issued instruction, the general-purpose register used for the transfer. */
    int rt = (iss >> 5) & 0x1F;
    iss = iss & ~(0x1F << 5);

    switch(iss) {
        case VSYSREG_ICC_SGI1R_EL1:
            vgicv3_generate_sgi(vcpu, rt, write_not_read);
        return 0;
    }

    LOG_WARN("Unable to handler a system register\n");
    return -1;
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

        /* trapped by read/write system register */
        case 0x18:
            if(vsysreg_handler(vcpu, esr_iss) < 0) {
                abort("Unknow system register trap");
            }
            vcpu->regs.elr += 4;
            break;

        /* data abort 内存访问异常*/
        case 0x24:
            //LOG_INFO("\033[32m[el1_sync_proc] data abort from EL0/1\033[0m\n");
            data_abort_handler(vcpu, esr_iss, far);
            vcpu->regs.elr += 4;
            break;
        default:
            abort("Unknown el1 sync: esr_ec %p, esr_iss %p, elr %p, far %p", esr_ec, esr_iss, elr, far);
            break;
    }

    return;
}

/* 处理来自EL1的中断 */
void el1_irq_proc(void)
{
    u32 iar, irq;
    struct vcpu *vcpu;
    read_sysreg(vcpu, tpidr_el2);
    virq_enter(vcpu);

    gicv3_ops.get_irq(&iar);
    irq = iar & 0x3FF;

    //LOG_INFO("el1 irq proc\n");
    //完成优先级降权
    gicv3_ops.guest_eoi(irq);

    /* set pirq equal to virq */
    virq_inject(vcpu, irq, irq);
}

int vgicv3_generate_sgi(struct vcpu *vcpu, int rt, int wr)
{
    u64  regs_sgi = vcpu->regs.x[rt];
    u16  target   = regs_sgi & 0xFFFF;
    u8   intid    = (regs_sgi >> 24) & 0xF;
    bool irm      = (regs_sgi >> 40) & 0x1;

    write_sysreg(ICC_SGI1R_EL1, regs_sgi);
    return 1;
}