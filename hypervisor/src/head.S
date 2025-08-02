#include <layout.h>

.section .text, "ax"
.global  _start
.type    _start, function
.align 4

# 设置栈指针，如果core_id = 0 , 则跳转到 hyper_init_primary
_start:
    /* Set stack for c code */
    adrp    x0, sp_stack
    /* Get Current code id */
    mrs     x1, mpidr_el1
    and     x1, x1, #0x0f
    add     x2, x1, 1
    mov     x3, #SZ_4K
    mul     x3, x3, x2
    add     x0, x0, x3
    mov     sp, x0
    cbz     x1, hyper_init_primary
    bl      hyper_init_secondary
    /* spin here */
    b       .


# #0 表示立即数值为 0，通常意味着不直接指定特定的功能 ID（Function ID），
# 而是依赖通用寄存器（通常是 X0）传递功能 ID。
.global smc_call
.type   smc_call, function
smc_call:
    smc #0
    ret
