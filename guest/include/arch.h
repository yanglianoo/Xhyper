#ifndef __ARCH_H__
#define __ARCH_H__

#include "types.h"

/* Read a sys_reg to val */
#define __read_sysreg(val, reg) \
    asm volatile("mrs %0, " #reg : "=r"(val))
#define read_sysreg(val, reg)   __read_sysreg(val, reg)

#define __write_sysreg(reg, val)  \
  asm volatile("msr " #reg ", %0" : : "r"(val))
#define write_sysreg(reg, val)  \
  do { u64 x = (u64)(val); __write_sysreg(reg, x); } while(0)

#define isb()   asm volatile("isb");
#define dsb(sy) asm volatile("dsb " #sy);

#define irq_enable  asm volatile("msr daifclr, #2" ::: "memory")
#define irq_disable asm volatile("msr daifset, #2" ::: "memory")

static inline int coreid()
{
    int val;
    read_sysreg(val, mpidr_el1);
    return val & 0xf;
}

#endif

