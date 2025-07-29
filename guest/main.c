#include "types.h"
#include "pl011.h"
#include "printf.h"
#include "config.h"

__attribute__((aligned(SZ_4K))) char sp_stack[SZ_4K * NCPU] = {0};

void print_logo(void)
{
    printf(" V     V     M     M      II  \n");
    printf("  V   V      MM   MM      II  \n");
    printf("   V V       M M M M      II  \n");
    printf("    V        M  M  M      II  \n");
    printf("    V        M     M      II  \n");
    printf("\n");
}

int vm_primary_init()
{
    pl011_init();
    print_logo();

    while(1) {}

    return 0;
}