#ifndef __VM_H__
#define __VM_H__

#include <types.h>
#include <guest.h>

typedef struct vm_config {
    guest_t *guest_image;
    guest_t *guest_dtb;
    guest_t *guest_initrd;
    u64      entry_addr;
    u64      ram_size;
} vm_config_t;

#endif