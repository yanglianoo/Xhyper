.section ".text"


.macro save_hyp_regs
    sub sp, sp, #256
    stp x0, x1, [sp, #8 * 0]
    stp x2, x3, [sp, #8 * 2]
    stp x4, x5, [sp, #8 * 4]
    stp x6, x7, [sp, #8 * 6]
    stp x8, x9, [sp, #8 * 8]
    stp x10, x11, [sp, #8 * 10]
    stp x12, x13, [sp, #8 * 12]
    stp x14, x15, [sp, #8 * 14]
    stp x16, x17, [sp, #8 * 16]
    stp x18, x19, [sp, #8 * 18]
    stp x20, x21, [sp, #8 * 20]
    stp x22, x23, [sp, #8 * 22]
    stp x24, x25, [sp, #8 * 24]
    stp x26, x27, [sp, #8 * 26]
    stp x28, x29, [sp, #8 * 28]
    str x30, [sp, #8 * 30]
.endm


.macro restore_hyp_regs
    ldp x0, x1, [sp, #8 * 0]
    ldp x2, x3, [sp, #8 * 2]
    ldp x4, x5, [sp, #8 * 4]
    ldp x6, x7, [sp, #8 * 6]
    ldp x8, x9, [sp, #8 * 8]
    ldp x10, x11, [sp, #8 * 10]
    ldp x12, x13, [sp, #8 * 12]
    ldp x14, x15, [sp, #8 * 14]
    ldp x16, x17, [sp, #8 * 16]
    ldp x18, x19, [sp, #8 * 18]
    ldp x20, x21, [sp, #8 * 20]
    ldp x22, x23, [sp, #8 * 22]
    ldp x24, x25, [sp, #8 * 24]
    ldp x26, x27, [sp, #8 * 26]
    ldp x28, x29, [sp, #8 * 28]
    ldr x30, [sp, #8 * 30]
    add sp, sp, #256
.endm

.macro save_vm_regs
    stp x0, x1, [sp, #-16]!     /* save x0, x1 on stack */
    mrs x0, tpidr_el2           /* x0 = &vcpu->regs */

    // 保存通用寄存器 X2-X29
    stp x2, x3, [x0, #8 * 2]
    stp x4, x5, [x0, #8 * 4]
    stp x6, x7, [x0, #8 * 6]
    stp x8, x9, [x0, #8 * 8]
    stp x10, x11, [x0, #8 * 10]
    stp x12, x13, [x0, #8 * 12]
    stp x14, x15, [x0, #8 * 14]
    stp x16, x17, [x0, #8 * 16]
    stp x18, x19, [x0, #8 * 18]
    stp x20, x21, [x0, #8 * 20]
    stp x22, x23, [x0, #8 * 22]
    stp x24, x25, [x0, #8 * 24]
    stp x26, x27, [x0, #8 * 26]
    stp x28, x29, [x0, #8 * 28]

    mrs x1, spsr_el2 /* x1 = spsr_el2 */
    mrs x2, elr_el2  /* x2 = elr_el2 */
    ldp x3, x4, [sp], #16   /* x3 = x0, x4 = x1  从栈加载之前保存的 X0 和 X1 到 X3 和 X4*/
    stp x30, x1, [x0, #8 *30] /* x30 = #8 *30   x1=spsr_el2 = #8 *31 保存spsr_el2*/
    str x2, [x0, #8 * 32]  /* 保存 elr_el2 */ 
    stp x3, x4, [x0, #8 * 0] /* 保存 x0 x1 */
.endm   


.macro restore_vm_regs
    mrs x0,  tpidr_el2          /* x0 = &vcpu->regs */
    ldp x30, x1, [x0, #8 * 30]  /* x1 = spsr_el2 */
    ldr x2, [x0, #8 * 32]       /* x2 = elr_el2 */
    msr spsr_el2, x1            /* spsr_el2 存放程序状态 */
    msr elr_el2,  x2            /* elr 存放异常返回地址 */

    ldp x3, x4, [x0, #8 * 0]    /* x3 = x0, x4 = x1 */
    stp x3, x4, [sp, #-16]!     /* 将 X3 和 X4（Guest VM 的 X0 和 X1）压入栈，栈指针 SP 减 16 字节。 */
    /* 恢复 X2-X29*/
    ldp x2, x3, [x0, #8 * 2]
    ldp x4, x5, [x0, #8 * 4]
    ldp x6, x7, [x0, #8 * 6]
    ldp x8, x9, [x0, #8 * 8]
    ldp x10, x11, [x0, #8 * 10]
    ldp x12, x13, [x0, #8 * 12]
    ldp x14, x15, [x0, #8 * 14]
    ldp x16, x17, [x0, #8 * 16]
    ldp x18, x19, [x0, #8 * 18]
    ldp x20, x21, [x0, #8 * 20]
    ldp x22, x23, [x0, #8 * 22]
    ldp x24, x25, [x0, #8 * 24]
    ldp x26, x27, [x0, #8 * 26]
    ldp x28, x29, [x0, #8 * 28]
    /* 恢复 X0 和 X1 */
    ldp x0, x1, [sp], #16
.endm

.global hyper_vector
hyper_vector:
.balign 0x800                   //确保向量表地址按 2048 字节（2KB）对齐
.set  vector_base, hyper_vector //定义符号 vector_base，指向 hyper_vector 地址
.org  vector_base               // 设置汇编起始地址为 vector_base。

/* Current EL with SP_EL0 */
.org (vector_base + 0x00)
    b .                     // sync
.org (vector_base + 0x80)
    b .                     // IRQ
.org (vector_base + 0x100)
    b .                     // FIQ
.org (vector_base + 0x180)
    b .                     // error

/* Current EL with SP_ELx */
.org (vector_base + 0x200)
    b vector_el2_sync       // sync  暂时为空
.org (vector_base + 0x280)
    b vector_el2_irq        // IRQ 处理来自EL2的irq异常
.org (vector_base + 0x300)
    b .                     // FIQ
.org (vector_base + 0x380)
    b .                     // error

/* Lower EL using aarch64 */
.org (vector_base + 0x400)
    b vector_el1_sync       // sync 处理来自EL1的sync异常
.org (vector_base + 0x480)
    b vector_el1_irq        // IRQ  处理来自EL1的irq异常
.org (vector_base + 0x500)
    b .                     // FIQ
.org (vector_base + 0x580)
    b .                     // error

/* Lower EL using aarch32 */
.org (vector_base + 0x600)
    b .                     // sync
.org (vector_base + 0x680)
    b .                     // IRQ
.org (vector_base + 0x700)
    b .                     // FIQ
.org (vector_base + 0x780)
    b .                     // error

.global  switch_out
.type    switch_out, function
switch_out:
    restore_vm_regs
    eret

vector_el2_sync:
    b .

vector_el2_irq:
    save_hyp_regs
    bl el2_irq_proc
    restore_hyp_regs
    eret

vector_el1_sync:
    save_vm_regs
    bl el1_sync_proc
    restore_vm_regs
    eret

vector_el1_irq:
    save_vm_regs
    bl el1_irq_proc
    restore_vm_regs
    eret
