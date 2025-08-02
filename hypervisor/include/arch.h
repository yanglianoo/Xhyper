#ifndef __ARCH_H__
#define __ARCH_H__


/* 将reg的值读取到val中 */
#define __read_sysreg(val, reg) \
    asm volatile("mrs %0, " #reg : "=r"(val))

#define read_sysreg(val, reg)   __read_sysreg(val, reg)

/* 将val的值写入到reg中 */
#define __write_sysreg(reg, val)  \
  asm volatile("msr " #reg ", %0" : : "r"(val))
#define write_sysreg(reg, val)  \
  do { u64 x = (u64)(val); __write_sysreg(reg, x); } while(0)

// 指令同步屏障
#define isb()   asm volatile("isb")
// 数据同步屏障
#define dsb(sy) asm volatile("dsb " #sy);

/* SPSR_EL2 */
#define SPSR_M(n)    (n & 0xf)
#define SPSR_DAIF    (0xf << 6)

// 读取当前核心id
static inline int coreid()
{
    int val;
    read_sysreg(val, mpidr_el1);
    return val & 0xf;
}

static inline void flush_tlb()
{
    //在刷新 TLB 之前，
    //确保所有之前的内存写操作（如页表更新）对其他核可见。
    dsb(ishst);
    /*
    vmalls12e1 是 ARMv8-A 的虚拟化扩展指令，表示：
     - vmall：使所有虚拟机的 TLB 条目失效。
     - s12：针对 Stage-1 和 Stage-2 地址转换（虚拟机管理器的两阶段地址转换）。
     - e1：针对 EL1（Exception Level 1，客户机通常运行的级别）。
    */ 
    asm volatile("tlbi vmalls12e1");
    dsb(ish);
    isb();
}

#endif