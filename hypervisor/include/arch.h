#ifndef __ARCH_H__
#define __ARCH_H__


/* 将reg的值读取到val中 */
#define __read_sysreg(val, reg) \
    asm volatile("mrs %0, " #reg : "=r"(val))

#define read_sysreg(val, reg)   __read_sysreg(val, reg)

// 指令同步屏障
#define isb()   asm volatile("isb")

// 读取当前核心id
static inline int coreid()
{
    int val;
    read_sysreg(val, mpidr_el1);
    return val & 0xf;
}
#endif