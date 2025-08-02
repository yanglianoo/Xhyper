#include "types.h"
#include "pl011.h"
#include "printf.h"
#include "config.h"

__attribute__((aligned(SZ_4K))) char sp_stack[SZ_4K * NCPU] = {0};

extern void _start();
extern u64 smc_call(u64 funid, u64 target_cpu, u64 entrypoint);

void print_logo(void)
{
    printf(" V     V     M     M      II  \n");
    printf("  V   V      MM   MM      II  \n");
    printf("   V V       M M M M      II  \n");
    printf("    V        M  M  M      II  \n");
    printf("    V        M     M      II  \n");
    printf("\n");
}

int vm_secondary_init()
{
    printf("I am guest secondary cpu\n");
    while(1) {
        printf("I am vm 1 on core 1\n");
        for(int i=0; i < 100000000; i++);
    }
}

int vm_primary_init()
{
    pl011_init();
    print_logo();

    /* test code for wakeup vcore 1 */
    smc_call((u64)0xc4000003, (u64)1, (u64)_start);

    while(1) {
        printf("I am vm 1 on core 0\n");
        for(int i=0; i < 100000000; i++);
    }


    return 0;
}