#ifndef __VM_H__
#define __VM_H__

#include <types.h>
#include <guest.h>
#include <spinlock.h>
#include <vcpu.h>

typedef struct vm_config {
    guest_t *guest_image;
    guest_t *guest_dtb;
    guest_t *guest_initrd;
    u64      entry_addr;
    u64      ram_size;
    int      ncpu;
} vm_config_t;

typedef struct vm {
    char       name[32];
    int        nvcpu;
    u64       *vttbr;
    spinlock_t vm_lock;
    struct vcpu *vcpus[NCPU];
} vm_t;

void create_guest_vm(vm_config_t *vm_config);

#endif