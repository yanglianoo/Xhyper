#include <vm.h>
#include <types.h>
#include <xlog.h>
#include <layout.h>
#include <guest.h>
#include <utils.h>
#include <arch.h>
#include <xmalloc.h>
#include <vmm.h>
#include <printf.h>
#include <vmmio.h>
#include <gicv3.h>
#include <vgicv3.h>
#include <spinlock.h>


/* Alloc and initialize a virtual gic cpu interface */
struct vgicv3_cpu *create_vgic_cpu(int vcpuid) 
{
    struct vgicv3_cpu *vgic_cpu = (struct vgicv3_cpu *)xmalloc(sizeof(struct vgicv3_cpu));
    if(vgic_cpu == NULL) {
        abort("Unable to alloc vgic_cpu, no memory");
    }

    vgic_cpu->used_lr = 0;

    for(struct vgicv3_irq_config *v = vgic_cpu->sgis; v < &vgic_cpu->sgis[GIC_NSGI]; v++) {
        v->enabled = 0;
        v->affinity = 1 << vcpuid;
        v->group = 1;
    }

    for(struct vgicv3_irq_config *v = vgic_cpu->ppis; v < &vgic_cpu->ppis[GIC_NSGI]; v++) {
        v->enabled = 0;
        v->affinity = 1 << vcpuid;
        v->group = 1;
    }

    return vgic_cpu;
}

static struct vgicv3_irq_config *vgic_irq_get(struct vcpu *vcpu, int irq_num)
{
    if(irq_num >= 0 && irq_num < 16) { // SGI中断
        return &vcpu->vgic_cpu->sgis[irq_num];
    } else if(irq_num >= 16 && irq_num < 32) { // PPI中断
        return &vcpu->vgic_cpu->ppis[irq_num];
    } else if(irq_num >=32 ) {
        return &vcpu->vm->vgic_dist->spis[irq_num-32]; // SPI中断
    } else {
        abort("Invalid irq number");
    }
    return NULL;
}

/* 该函数用于同步虚拟GIC的LR使用状态，确保used_lr位图与硬件LR寄存器的实际状态一致，
   避免重复分配或遗漏中断。适用于虚拟化场景下的中断管理
*/
void virq_enter(struct vcpu *vcpu)
{
    struct vgicv3_cpu *vgic_cpu = vcpu->vgic_cpu;
    for(int i = 0; i < gic_max_lrs; i++) {
        if((vgic_cpu->used_lr & (1 << i)) != 0) {
            u64 list_reg = gic_read_list_reg(i);
            if(LR_IS_INACTIVE(list_reg)) {
                vgic_cpu->used_lr &= ~(u16)(1 << i);
            }
        }
    }
}

//从used_lr这个lr的bitmap中分配一个lr
static int alloc_lr(struct vgicv3_cpu *vgic_cpu)
{
    for(int i = 0; i < gic_max_lrs; i++) {
        if((vgic_cpu->used_lr & (1 << i)) == 0) {
            vgic_cpu->used_lr |= (1 << i);
            return i;
        }
    }
    return -1;
}


int virq_inject(struct vcpu *vcpu, u32 pirq, u32 virq)
{
    struct vgicv3_cpu *vgic_cpu = vcpu->vgic_cpu;
    u64 lr = gic_create_lr(pirq, virq);
    int n  = alloc_lr(vgic_cpu);
    if(n >= 0) {
        gic_write_list_reg(n, lr);
    } else {
        abort("No List Register");
    }

    return 0;
}

//开启 irq_num 中断
static void vgic_irq_enable(struct vcpu *vcpu, int irq_num)
{
    gicv3_ops.unmask(irq_num);
}

//关闭 irq_num 中断
static void vgic_irq_disable(struct vcpu *vcpu, int irq_num)
{
    gicv3_ops.mask(irq_num);
}

static void vgic_target_set(struct vcpu *vcpu, int irq_num, u8 target)
{
    gicv3_ops.set_affinity(irq_num, target);
}

/* EL1访问GICD 会触发地址异常访问陷入到EL2通过mmio读*/
static int vgicd_read(struct vcpu *vcpu, u64 offset, u64 *val, struct vmmio_access *vmmio)
{
    int irq_num;
    struct vgicv3_irq_config *irq;
    struct vgicv3_dist *vgic_dist = vcpu->vm->vgic_dist;

    arch_spin_lock(&vgic_dist->lock);

    switch(offset) {
        /* simulate GICD_CTLR */
        case GICD_CTLR: {
            u32 reg = GICD_CTLR_ARE_NA;
            if(vgic_dist->enabled) {
                reg |= GICD_CTLR_ENABLE_G1A; // 使能非安全 Group1 中断。
            }
            *val = reg;
            goto finished;
        }

        /* simulate GICD_TYPER */
        case GICD_TYPER: {
            /* ITLinesNumber */ 
            u32 reg = ((vgic_dist->nspis + 32) >> 5) - 1; // 计算支持的最大SPI
            /* CPUNumber */
            reg |= (8 - 1) << GICD_TYPER_CPUNumber_SHIFT; // 假设系统有 8 个 CPU
            *val = reg;
            goto finished;
        }

        /* simulate GICD_IIDR */
        case GICD_IIDR: {
            *val = GICD_READ32(GICD_IIDR);
            goto finished;
        }

        /* simulate GICD_TYPER2 */
        case GICD_TYPER2:
            *val = 0;   /* Linux read this reg to the feature of gicv3, fake it */
            goto finished;

        case GICD_IGROUPR(0) ... GICD_IGROUPR(31) + 3: {
            u32 igrp = 0;
            irq_num = (offset - GICD_IGROUPR(0)) / sizeof(u32) * 32; // 计算中断号
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i); // 获取中断配置
                igrp |= (u32)irq->group << i; // 设置中断组
            }
            *val = igrp;
            goto finished;
        }

        case GICD_ISENABLER(0) ... GICD_ISENABLER(31) + 3: {
            u32 isen;
            irq_num = (offset - GICD_ISENABLER(0)) / sizeof(u32) * 32; // 计算中断号
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                isen |= (u32)irq->enabled << i;
            }
            *val = isen;
            goto finished;
        }

        case GICD_IPRIORITYR(0) ... GICD_IPRIORITYR(254) + 3: {
            u32 iprio = 0;
            irq_num = offset - GICD_IPRIORITYR(0);
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                iprio |= (u32)irq->priority << (i * 8);
            }
            *val = iprio;
            goto finished;
        }

        case GICD_ITARGETSR(0) ... GICD_ITARGETSR(254) + 3: {
            u32 itar = 0;
            irq_num = offset - GICD_ITARGETSR(0);
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num+i);
                itar |= (u32)irq->affinity << (i * 8);
            }
            *val = itar;
            goto finished;
        }

        case GICD_ISPENDR(0) ... GICD_ISPENDR(31) + 3:
        case GICD_ICPENDR(0) ... GICD_ICPENDR(31) + 3:
        case GICD_ISACTIVER(0) ... GICD_ISACTIVER(31) + 3:
        case GICD_ICACTIVER(0) ... GICD_ICACTIVER(31) + 3:
        case GICD_ICFGR(0) ... GICD_ICFGR(63) + 3:
        case GICD_IROUTER(0) ... GICD_IROUTER(31) + 3:
        case GICD_IROUTER(32) ... GICD_IROUTER(1019) + 3:
            *val = 0;
            goto finished;
        case GICD_PIDR2:
            *val = GICD_READ32(GICD_PIDR2);
            goto finished;
    }

    LOG_WARN("[vgicd_read] Unable to handle the GICD_* request\n");
    arch_spin_unlock(&vgic_dist->lock);
    return -1;

finished:
    arch_spin_unlock(&vgic_dist->lock);

    return 0;
}

static int vgicd_write(struct vcpu *vcpu, u64 offset, u64 val, struct vmmio_access *vmmio)
{
    int irq_num;
    struct vgicv3_irq_config *irq;
    struct vgicv3_dist *vgic_dist = vcpu->vm->vgic_dist;

    arch_spin_lock(&vgic_dist->lock);

    switch(offset) {
        /* simulate GICD_CTLR */
        case GICD_CTLR: {
            if(val & GICD_CTLR_ENABLE_G1A) {
                vgic_dist->enabled = 1;
            } else {
                vgic_dist->enabled = 0;
            }
            goto finished;
        }
        /* simulate GICD_TYPER */
        case GICD_TYPER:
        /* simulate GICD_IIDR */
        case GICD_IIDR:
            /* Read only register */
            goto finished;

        case GICD_IGROUPR(0) ... GICD_IGROUPR(31) + 3:
            goto finished;
        
        case GICD_ISENABLER(0) ... GICD_ISENABLER(31) + 3: {
            irq_num = (offset - GICD_ISENABLER(0)) / sizeof(u32) * 32;
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                if((val >> i) & 0x1) {
                    irq->enabled = 1;
                    vgic_irq_enable(vcpu, irq_num + i);
                }
            }
            goto finished;
        }

        case GICD_ICENABLER(0) ... GICD_ICENABLER(31) + 3: {
            irq_num = (offset - GICD_ISENABLER(0)) / sizeof(u32) * 32;
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                if((val >> i) & 0x1) {
                    irq->enabled = 0;
                    vgic_irq_disable(vcpu, irq_num + i);
                }
            }
            goto finished;
        }

        case GICD_IPRIORITYR(0) ... GICD_IPRIORITYR(254) + 3: {
            irq_num = (offset - GICD_IPRIORITYR(0)) / sizeof(u32) * 4;
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                irq->priority = (val >> (i * 8)) & 0xff;
            }
            goto finished;
        }

        case GICD_ITARGETSR(0) ... GICD_ITARGETSR(254) + 3: {
            irq_num = (offset - GICD_ITARGETSR(0)) / sizeof(u32) * 4;
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                irq->affinity = (val >> (i * 8)) & 0xff;
                vgic_target_set(vcpu, irq_num + i, irq->affinity);
            }
            goto finished;
        }

        case GICD_ISPENDR(0) ... GICD_ISPENDR(31) + 3:
        case GICD_ICPENDR(0) ... GICD_ICPENDR(31) + 3:
        case GICD_ISACTIVER(0) ... GICD_ISACTIVER(31) + 3:
        case GICD_ICACTIVER(0) ... GICD_ICACTIVER(31) + 3:
        case GICD_ICFGR(0) ... GICD_ICFGR(63) + 3:
        case GICD_IROUTER(0) ... GICD_IROUTER(31) + 3:
        case GICD_IROUTER(32) ... GICD_IROUTER(1019) + 3:
            goto finished;
    }

    LOG_WARN("[vgicd_write] Unable to handle the GICD_* request\n");
    arch_spin_unlock(&vgic_dist->lock);
    return -1;

finished:
    arch_spin_unlock(&vgic_dist->lock);
    return 0;
}

static int vgicr_read(struct vcpu *vcpu, u64 offset, u64 *val, struct vmmio_access *vmmio)
{
    u32 gicr_index = offset / GICR_STRIDE;
    u32 gicr_offset = offset % GICR_STRIDE;
    int irq_num;
    struct vgicv3_irq_config *irq;

    if(gicr_index > (u32)vcpu->vm->nvcpu) {
        LOG_WARN("Invalid gic redistributor access\n");
        return -1;
    }

    vcpu = vcpu->vm->vcpus[gicr_index];

    switch(gicr_offset) {
        case GICR_CTLR:
        case GICR_WAKER:
        case GICR_IGROUPR0:
            *val = 0;
            return 0;
        case GICR_IIDR:
            *val = GICR_READ32(vcpu->cpuid, GICR_IIDR);
            return 0;
        case GICR_TYPER:
            *val = GICR_READ64(vcpu->cpuid, GICR_TYPER);
            return 0;
        case GICR_PIDR2:
            *val = GICR_READ32(vcpu->cpuid, GICR_PIDR2);
            return 0;
        case GICR_ISENABLER0: {
            u32 isen = 0;
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, i);
                isen |= irq->enabled << i;
            }
            *val = isen;
            return 0;
        }
        case GICR_ICENABLER0:
            *val = 0;
            return 0;
        case GICR_ICPENDR0:
            *val = 0;
            return 0;
        case GICR_ISACTIVER0:
        case GICR_ICACTIVER0:
            *val = 0;
            return 0;
        case GICR_IPRIORITYR(0) ... GICR_IPRIORITYR(7): {
            u32 iprio = 0;
            irq_num = (offset - GICR_IPRIORITYR(0)) / sizeof(u32) * 4;
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                iprio |= irq->priority << (i * 8);
            }
            *val = iprio;
            return 0;
        }
        case GICR_ICFGR0:
        case GICR_ICFGR1:
        case GICR_IGRPMODR0:
            *val = 0;
            return 0;
    }

    LOG_WARN("[vgicr_read] Unable to handle the GICR_* request\n");
    return 0;
}

static int vgicr_write(struct vcpu *vcpu, u64 offset, u64 val, struct vmmio_access *vmmio)
{
    u32 gicr_index = offset / GICR_STRIDE;
    u32 gicr_offset = offset % GICR_STRIDE;
    int irq_num;
    struct vgicv3_irq_config *irq;

    if(gicr_index > (u32)vcpu->vm->nvcpu) {
        LOG_WARN("Invalid gic redistributor access\n");
        return -1;
    }

    vcpu = vcpu->vm->vcpus[gicr_index];

    switch(gicr_offset) {
        case GICR_CTLR:
        case GICR_WAKER:
        case GICR_IGROUPR0:
        case GICR_TYPER:
        case GICR_PIDR2:
            return 0;
        case GICR_ISENABLER0:
            for(int i = 0; i < 32; i++) {
                irq = vgic_irq_get(vcpu, i);
                if((val >> i) & 0x1) {
                    irq->enabled = 1;
                    vgic_irq_enable(vcpu, i);
                }
            }
            return 0;
        case GICR_ICENABLER0:
        case GICR_ICPENDR0:
        case GICR_ISACTIVER0:
        case GICR_ICACTIVER0:
            return 0;
        case GICR_IPRIORITYR(0) ... GICR_IPRIORITYR(7):
            irq_num = (offset - GICR_IPRIORITYR(0)) / sizeof(u32) * 4;
            for(int i = 0; i < 4; i++) {
                irq = vgic_irq_get(vcpu, irq_num + i);
                irq->priority = (val >> (i * 8)) & 0xff;
            }
            return 0;
        case GICR_ICFGR0:
        case GICR_ICFGR1:
        case GICR_IGRPMODR0:
            return 0;
    }

    LOG_WARN("[vgicr_write] Unable to handle the GICR_* request\n");
    return -1;
}

struct vgicv3_dist *create_vgic_dist(struct vm *vm)
{
    struct vgicv3_dist *vgic_dist = (struct vgicv3_dist *)xmalloc(sizeof(struct vgicv3_dist));
    if(vgic_dist == NULL) {
        abort("Unable to alloc vgic_dist, no memory");
    }

    /* 0 ~ 31 for SGIs and PPIs */
    vgic_dist->nspis = gic_max_spi - 31;
    vgic_dist->enabled = 0;
    vgic_dist->spis = (struct vgicv3_irq_config *)xmalloc(vgic_dist->nspis);
    if(vgic_dist->spis == NULL) {
        abort("Unable to alloc vgic_dist->spis, no memory");
    }

    create_mmio_trap(vm, GICD_BASE, GICD_SIZE, vgicd_read, vgicd_write);
    create_mmio_trap(vm, GICR_BASE, GICR_SIZE, vgicr_read, vgicr_write);

    return vgic_dist;
}
