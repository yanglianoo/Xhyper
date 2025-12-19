#ifndef __VM_H__
#define __VM_H__

#include <types.h>
#include <guest.h>
#include <spinlock.h>
#include <vcpu.h>
#include <vmmio.h>
#include <vgicv3.h>

struct vmmio_access;

typedef struct vm_config {
    guest_t *guest_image;
    guest_t *guest_dtb;
    guest_t *guest_initrd;
    u64      entry_addr;
    u64      ram_size;
    int      ncpu;
    u64      dtb_addr;
} vm_config_t;

typedef struct vm {
    char       name[32];
    int        nvcpu;
    u64       *vttbr;
    spinlock_t vm_lock;
    struct vcpu *vcpus[NCPU];
    struct vgicv3_dist *vgic_dist;
    struct vmmio_info *vmmios;
    u64        dtb;
} vm_t;

void create_guest_vm(vm_config_t *vm_config);
void create_mmio_trap(struct vm *vm, u64 ipa, u64 size,
                      int (*vmmio_read)(struct vcpu *, u64, u64 *, struct vmmio_access *),
                      int (*vmmio_write)(struct vcpu *, u64, u64, struct vmmio_access *));

#endif