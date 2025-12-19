#include <vpsci.h>
#include <types.h>
#include <printf.h>
#include <xlog.h>
#include <vcpu.h>

extern void _start();

static u32 vpsci_version()
{
    return smc_call(PSCI_VERSION, 0, 0);
}

static s32 vpsci_migrate_info_type()
{
    return smc_call(PSCI_MIGRATE_INFO_TYPE, 0, 0);
}

static s32 vpsci_cpu_on(vcpu_t *vcpu, u64 funid, u64 target_cpu, u64 entry_addr)
{
    LOG_INFO("Vpsci cpu on call for vcpu %d on entrypoint %p\n", target_cpu, entry_addr);
    
    if(target_cpu >= (u64)vcpu->vm->nvcpu) {
        LOG_WARN("Vpsci failed to wakeup vcpu\n");
    }

    vcpu_t *target = vcpu->vm->vcpus[target_cpu];
    target->regs.elr = entry_addr;
    target->state = VCPU_READY;
    /* wakeup the physical cpu */
    return smc_call(PSCI_SYSTEM_CPUON, target_cpu, (u64)_start);
}

u64 vpsci_trap_smc(vcpu_t *vcpu, u64 funid, u64 target_cpu, u64 entry_addr)
{
    if(vcpu == NULL) {
        abort("vpsci_trap_smc with NULL vcpu");
    }

    switch(funid) {
        case PSCI_VERSION:
            return vpsci_version();
        case PSCI_MIGRATE_INFO_TYPE:
            return (s64)vpsci_migrate_info_type();     
        case PSCI_SYSTEM_OFF:
            LOG_WARN("Unsupported PSCI CPU OFF\n");
            break;
        case PSCI_SYSTEM_RESET:
            LOG_WARN("Unsupported PSCI CPU RESET\n");
            break;
        case PSCI_SYSTEM_CPUON:
            return (s64)vpsci_cpu_on(vcpu, funid, target_cpu, entry_addr);
        case PSCI_FEATURE:    /* Linux will use this funid to get the PSCI FEATURE */
            /* fake it */
            return 0;
        case 0x80000000:
            return 0;
        default:
            abort("Unknown function id : %p from hvc/smc call", funid);
            return -1;
    }
    return -1;
}