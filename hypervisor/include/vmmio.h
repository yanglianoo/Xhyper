#ifndef __VMMIO_H__
#define __VMMIO_H__

#include <types.h>
#include <vm.h>
#include <vcpu.h>

struct vcpu;
struct vm;

enum access_size {
    VMMIO_BYTE       = 1,
    VMMIO_HALFWORD   = 2,
    VMMIO_WORD       = 4,
    VMMIO_DOUBLEWORD = 8,
};

struct vmmio_access {
    u64 ipa;    /* fault ipa */
    u64 pc;     /* fault pc */
    int wnr;    /* write or read */
    enum access_size accsize;  /* access size */
};

struct vmmio_info {
    struct vmmio_info *next;
    u64 base;
    u64 size;

    int (*vmmio_read)(struct vcpu *, u64, u64 *, struct vmmio_access *);
    int (*vmmio_write)(struct vcpu *, u64, u64, struct vmmio_access *);
};

int vmmio_handler(struct vcpu *vcpu, int reg_num, struct vmmio_access *vmmio);
int vmmio_handler_register(struct vm *vm, u64 ipa, u64 size,
                           int (*vmmio_read)(struct vcpu *, u64, u64 *, struct vmmio_access *),
                           int (*vmmio_write)(struct vcpu *, u64, u64, struct vmmio_access *));

#endif