#include <printf.h>
#include <layout.h>
#include <pl011.h>
#include <xmalloc.h>
#include <kalloc.h>

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

int hyper_init_primary()
{
    pl011_init();
    print_logo();

    /* xmalloc init */
    xmalloc_init();
    /* kalloc init */
    kalloc_init();

    printf("xmalloc and kalloc have been initialized\n");

    while(1) {}

    return 0;
}