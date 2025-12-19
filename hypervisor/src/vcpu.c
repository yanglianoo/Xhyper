#include <xlog.h>
#include <vcpu.h>
#include <types.h>
#include <arch.h>
#include <printf.h>
#include <vgicv3.h>

pcpu_t pcpus[NCPU];
vcpu_t vcpus[NCPU];
static spinlock_t vcpus_lock;

/* Acquire the pcpu according to current core id */
pcpu_t *cur_pcpu()
{
    int id = coreid();
    return &pcpus[id];
}

/* Initialize the physical cpu array
 * core 0 ---> pcpus[0]
 * core 1 ---> pcpus[1]
 * ...
 */
void pcpu_init()
{
    for(int i=0; i < NCPU; i++){
        pcpus[i].cpuid = i;
        pcpus[i].vcpu  = NULL;
    }
    return;
}

void vcpu_init()
{
    arch_spinlock_init(&vcpus_lock);
    for(int i=0; i < NCPU; i++){
        vcpus[i].state = VCPU_UNUSED;
    }
    return;
}

static vcpu_t *vcpu_alloc()
{
    arch_spin_lock(&vcpus_lock);
    for(vcpu_t *vcpu = vcpus; vcpu < &vcpu[NCPU]; vcpu++){
        if(vcpu->state == VCPU_UNUSED) {
            vcpu->state = VCPU_ALLOCED;
            arch_spin_unlock(&vcpus_lock);
            return vcpu;
        }
    }
    arch_spin_unlock(&vcpus_lock);
    return NULL;
}

vcpu_t *create_vcpu(vm_t *vm, int vcpuid, u64 entry)
{
    u64 cnt;
    vcpu_t *vcpu = vcpu_alloc();
    if(vcpu == NULL) {
        abort("Unable to alloc a vcpu");
    }

    vcpu->core_name = "Cortex-A72";
    vcpu->vm        = vm;
    vcpu->cpuid     = vcpuid;

    /*
        程序状态保存寄存器（SPSR）
        SPSR_M(5) : 5 = 0b0101 EL1 handler模式
        SPSR_DAIF : (0xf << 6) = 0b1111 << 6 = 0b1111000000 = 0x3C0
            - D（位 9）：Debug 异常（如断点、单步）屏蔽。
            - A（位 8）：SError（系统错误）屏蔽。
            - I（位 7）：IRQ（普通中断）屏蔽。
            - F（位 6）：FIQ（快速中断）屏蔽
    */
    vcpu->regs.spsr = SPSR_M(5) | SPSR_DAIF;    /* used to set the spsr_el2 */
    // elr 存放了异常返回地址
    vcpu->regs.elr  = entry;                    /* used to set the elr_el2 */
    // cpu ID判断寄存器
    vcpu->sys_regs.mpidr_el1 = vcpuid;          /* used to fake the mpidr_el1 */
    // cpu型号厂商等信息
    vcpu->sys_regs.midr_el1  = 0x410FD081;      /* used to fake the core to cortex-a72 */

    read_sysreg(cnt, cntfrq_el0);
    vcpu->sys_regs.cntfrq_el0 = cnt;

    u64 val;
    read_sysreg(val, cntfrq_el0);
    LOG_INFO("cntfrq_el0 is %p\n", val);
    
    if(vcpuid == 0) {  /* If it is the primary virtual cpu, set the dtb address (ipa) */
        vcpu->regs.x[0] = vm->dtb;
    }

    /* alloc vgic cpu interface */
    vcpu->vgic_cpu = create_vgic_cpu(vcpuid);

    gic_context_init(&vcpu->gic_context);
    return vcpu;
}

static void restore_sysreg(vcpu_t *vcpu)
{
    write_sysreg(spsr_el1, vcpu->sys_regs.spsr_el1);
    write_sysreg(elr_el1, vcpu->sys_regs.elr_el1);
    write_sysreg(vmpidr_el2, vcpu->sys_regs.mpidr_el1);
    write_sysreg(vpidr_el2, vcpu->sys_regs.midr_el1);
    write_sysreg(sp_el0, vcpu->sys_regs.sp_el0);
    write_sysreg(sp_el1, vcpu->sys_regs.sp_el1);
    write_sysreg(ttbr0_el1, vcpu->sys_regs.ttbr0_el1);
    write_sysreg(ttbr1_el1, vcpu->sys_regs.ttbr1_el1);
    write_sysreg(tcr_el1, vcpu->sys_regs.tcr_el1);
    write_sysreg(vbar_el1, vcpu->sys_regs.vbar_el1);
    write_sysreg(sctlr_el1, vcpu->sys_regs.sctlr_el1);
    write_sysreg(cntv_ctl_el0, vcpu->sys_regs.cntv_ctl_el0);
    write_sysreg(cntv_tval_el0, vcpu->sys_regs.cntv_tval_el0);
    write_sysreg(cntfrq_el0, vcpu->sys_regs.cntfrq_el0);
}

extern void switch_out(void);

static void switch_vcpu(vcpu_t *vcpu)
{
    cur_pcpu()->vcpu = vcpu;
    /* tpidr_el2保存当前执行的vcpu的地址，在上下文中可以用于获取vcpu */
    write_sysreg(tpidr_el2, vcpu);

    vcpu->state = VCPU_RUNNING;
    /* 设置stage2转换的页表基地址寄存器 */
    write_sysreg(vttbr_el2, vcpu->vm->vttbr);
    flush_tlb();
    /* 设置EL1/EL0系统寄存器的初始状态 */
    restore_sysreg(vcpu);
    /* 恢复gic上下文 */
    restore_gic_context(&vcpu->gic_context);
    isb();
    /* 切换到EL1 */
    switch_out();
}

void start_vcpu()
{
    int id = coreid();
    vcpu_t *vcpu = &vcpus[id];

    if(vcpu->state != VCPU_READY) {
        abort("vcpu is not ready");
    }

    LOG_INFO("pcpu %d: starting vcpu %d\n", id, vcpu->cpuid);

    switch_vcpu(vcpu);
}