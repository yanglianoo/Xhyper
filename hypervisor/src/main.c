#include <printf.h>
#include <layout.h>
#include <pl011.h>
#include <xmalloc.h>
#include <kalloc.h>
#include <xlog.h>
#include <vmm.h>

__attribute__((aligned(SZ_4K))) char sp_stack[SZ_4K * NCPU] = {0};

void print_logo(void)
{
    printf(" X   X        H   H  Y   Y  PPPP    EEEEE  RRRR  \n");
    printf("  X X         H   H   Y Y   P   P   E      R   R \n");
    printf("   X   =====  HHHHH    Y    PPPP    EEEE   RRRR  \n");
    printf("  X X         H   H    Y    P       E      R R   \n");
    printf(" X   X        H   H    Y    P       EEEEE  R  RR \n");
    printf("\n");
}

extern void test_create_vm_mapping(void);

int hyper_init_primary()
{
    pl011_init();
    print_logo();

    /* xmalloc init */
    xmalloc_init();
    /* kalloc init */
    kalloc_init();

    LOG_INFO("xmalloc and kalloc have been initialized\n");

    stage2_mmu_init();
    hyper_setup();

    test_create_vm_mapping();

    while(1) {}

    return 0;
}