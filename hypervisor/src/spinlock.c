#include "spinlock.h"
#include "printf.h"

void arch_spin_lock(spinlock_t *spinlock)
{
    if(spin_check(spinlock)) {
        abort("spinlock - %s is alredy held by core: %d", spinlock->name, spinlock->coreid);
    }

    // 锁被获取时，spinlock->lock设为1
    asm volatile(
        /* 获取spinlock->lock的地址   (锁变量地址)*/
        "mov x1, %0\n"
        /* w2 = 1 将立即数1加载到寄存器w2 */
        "mov w2, #1\n"
        /* 独占访问 - 从内存位置x1加载值到w3 */
        "1: ldxr w3, [x1]\n"
        /* 如果w3的值不为零, 则跳转到标签 1 */
        "cbnz w3, 1b\n"
        /* 使用stxr指令将w2的值尝试写入spinlock->lock
         * 它将返回的状态存储在w3中。
         * 如果spinlock->lock被其他线程改变了, stxr将失败, 并将w3设置为非零值。
        */
       "stxr w3, w2, [x1]\n"
        /* 如果上述stxr失败, 则继续跳转到标签1尝试获取锁 */
        "cbnz w3, 1b\n"
        :: "r" (&spinlock->lock) : "memory"
    );
    spinlock->coreid = coreid();
    isb();
}

/*
与 arch_spin_lock 不同，这里不需要原子操作（如 LDXR/STXR），
因为锁是由当前核心持有，假设其他核心不会同时修改 spinlock->lock（由锁的语义保证）。
*/
void arch_spin_unlock(spinlock_t *spinlock)
{
    if(!spin_check(spinlock)) {
        abort("spinlock - %s is unlocked by invalid coreid", spinlock->name);
    }
    /* 直接将锁的值改为0就可以 */
    asm volatile("str wzr, %0" : "=m"(spinlock->lock) :: "memory");
    isb();
}