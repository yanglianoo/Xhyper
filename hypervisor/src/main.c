#include <printf.h>
#include <layout.h>
#include <pl011.h>
#include <xmalloc.h>
#include <kalloc.h>
#include <xlog.h>
#include <vmm.h>
#include <guest.h>
#include <vm.h>

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
extern guest_t guest_vm_image;

int hyper_init_secondary()
{
    LOG_INFO("core %d is activated\n", coreid());
    stage2_mmu_init();
    hyper_setup();
    start_vcpu();
    
    while(1) {}
    return 0;
}


int hyper_init_primary()
{
    pl011_init();
    print_logo();

    /* xmalloc init */
    xmalloc_init();
    /* kalloc init */
    kalloc_init();

    stage2_mmu_init();
    hyper_setup();

    pcpu_init();
    vcpu_init();
    LOG_INFO("Pcpu/vcpu arrays have been initialized\n");

    vm_config_t guest_vm_cfg = {
        .guest_image  = &guest_vm_image,
        .guest_dtb    = NULL,
        .guest_initrd = NULL,
        .entry_addr   = 0x80200000,
        .ram_size     = 0x80000,
        .ncpu         = 2,
    };

    create_guest_vm(&guest_vm_cfg);

    start_vcpu();

    while(1) {}

    return 0;
}