#include <types.h>
#include <arch.h>
#include <vmmio.h>
#include <layout.h>
#include <vcpu.h>
#include <vm.h>
#include <printf.h>
#include <xlog.h>
#include <spinlock.h>
#include <xmalloc.h>

int vmmio_handler(struct vcpu *vcpu, int reg_num, struct vmmio_access *vmmio)
{
    /* The header of all the vmmios */
    struct vmmio_info *vmmios = vcpu->vm->vmmios;
    if(vmmios == NULL) {
        return -1;
    }

    u64 ipa = vmmio->ipa;
    u64 *reg = NULL;
    u64 val;
    /* 根据故障地址（vmmio->ipa）查找匹配的 MMIO 设备，
       调用相应的 vmmio_read 或 vmmio_write 函数。*/
    /* when use WZR or XZR, the srt is 31 */
    if(reg_num == 31) {
        val = 0;
    } else {
        /* use other common registers */
        reg = &vcpu->regs.x[reg_num];
        val = *reg;
    }

    /* 遍历MMIO设备链表 */
    for(struct vmmio_info *m = vmmios; m != NULL; m = m->next) {
        /* the fault ipa belong to this vmmio */
        if(m->base <= ipa && ipa < m->base + m->size) {
            if(vmmio->wnr) {
                if(m->vmmio_write) {
                    //LOG_INFO("[VMMIO WRITE]: device base: %p, offset is %p, write value %p, to reg %p\n", m->base, ipa - m->base, val, reg_num);
                    return m->vmmio_write(vcpu, ipa - m->base, val, vmmio);
                }
            } else {
                if(m->vmmio_read) {
                    //LOG_INFO("[VMMIO READ]: device base: %p, offset is %p, read to reg %p\n", m->base, ipa - m->base, reg_num);
                    return m->vmmio_read(vcpu, ipa - m->base, reg, vmmio);
                }
            }
        }
    }

    return 0;
}

/*
    注册新的 MMIO 设备到虚拟机的 vmmios 链表。
*/
int vmmio_handler_register(struct vm *vm, u64 ipa, u64 size,
                           int (*vmmio_read)(struct vcpu *, u64, u64 *, struct vmmio_access *),
                           int (*vmmio_write)(struct vcpu *, u64, u64, struct vmmio_access *))
{

    if(size == 0 || vm == NULL) {
        return -1;
    }

    arch_spin_lock(&vm->vm_lock);

    struct vmmio_info *new = (struct vmmio_info *)xmalloc(sizeof(struct vmmio_info));
    new->next = vm->vmmios;
    vm->vmmios = new;
    arch_spin_unlock(&vm->vm_lock);

    new->base = ipa;
    new->size = size;
    new->vmmio_read  = vmmio_read;
    new->vmmio_write = vmmio_write;

    return 0;
}