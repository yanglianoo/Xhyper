#ifndef __VCPU_H__
#define __VCPU_H__

#include "types.h"
#include "layout.h"
#include "vm.h"

enum vcpu_state {
    VCPU_UNUSED,
    VCPU_ALLOCED,
    VCPU_READY,
    VCPU_RUNNING,
};

typedef struct vcpu {
    struct {
        u64 x[31];
        u64 spsr;
        u64 elr;
    } regs;

    struct {
        u64 spsr_el1;
        u64 elr_el1;
        u64 mpidr_el1;
        u64 midr_el1;
        u64 sp_el0;
        u64 sp_el1;
        u64 ttbr0_el1;
        u64 ttbr1_el1;
        u64 tcr_el1;
        u64 vbar_el1;
        u64 sctlr_el1;
        u64 cntv_ctl_el0;
        u64 cntv_tval_el0;
        u64 cntfrq_el0;
    } sys_regs;
    
    const char *core_name;
    struct vm  *vm;
    int        cpuid;
    enum vcpu_state state;
} vcpu_t;

// 物理cpu
typedef struct pcpu {
    int     cpuid;
    vcpu_t *vcpu;
} pcpu_t;

pcpu_t *cur_pcpu(void);
void    pcpu_init(void);
void    vcpu_init(void);
vcpu_t *create_vcpu(struct vm *vm, int vcpuid, u64 entry);
void    start_vcpu(void);
#endif