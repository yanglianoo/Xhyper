#include <vm.h>
#include <types.h>
#include <xlog.h>
#include <vcpu.h>
#include <kalloc.h>
#include <layout.h>
#include <guest.h>
#include <utils.h>
#include <arch.h>
#include <xmalloc.h>
#include <vmm.h>
#include <printf.h>


static void vm_init(vm_t *vm, vm_config_t *vm_config)
{
    memset(vm, 0, sizeof(vm_t));
    strcpy(vm->name, vm_config->guest_image->guest_name);
    arch_spinlock_init(&vm->vm_lock);
    vm->nvcpu = vm_config->ncpu;

    /* set entry addr for primary core */
    vm->vcpus[0] = create_vcpu(vm, 0, vm_config->entry_addr);

    /* slave core entry addr will be set through psci call from Guest OS */
    for(int cpu = 1; cpu < vm_config->ncpu; cpu++) {
        vm->vcpus[cpu] = create_vcpu(vm, cpu, 0);
    }
}

static void do_memory_mapping(u64 *pgt, vm_config_t *vm_config)
{
    /* create normal range mapping for guest */
    u64   p;
    char *page;

    /* create guest image mapping */
    LOG_INFO("-->Create guest image mapping\n");
    
    u64 copy_size;
    for(p = 0; p < vm_config->guest_image->image_size; p += PAGESIZE) {
        char *page = alloc_one_page();
        if(page == NULL) {
            abort("Unable to alloc a page");
        }

        if(vm_config->guest_image->image_size - p > PAGESIZE){
            copy_size = PAGESIZE;
        } else {
            copy_size = vm_config->guest_image->image_size - p;
        }
        /* copy the guest image content from X-Hyper image to pages */
        memcpy(page, (char *)vm_config->guest_image->start_addr + p, copy_size);
        /* create the stage2 mapping for guest image */
        create_guest_mapping(pgt, vm_config->entry_addr + p, (u64)page, PAGESIZE, S2PTE_NORMAL | S2PTE_RW);
    }

    LOG_INFO("-->Create normal range mapping for guest\n");
    for( ; p < vm_config->ram_size; p += PAGESIZE) {
        /* Alloc a page size physical memory */
        page = alloc_one_page();
        if(page == NULL) {
            abort("Unable to alloc a page");
        }
        create_guest_mapping(pgt, vm_config->entry_addr + p, (u64)page, PAGESIZE, S2PTE_NORMAL | S2PTE_RW);
    }

}

static void do_device_mapping(u64 *pgt, vm_config_t *vm_config)
{
    create_guest_mapping(pgt, PL011BASE, PL011BASE, PAGESIZE, S2PTE_DEVICE | S2PTE_RW);
}

void create_guest_vm(vm_config_t *vm_config)
{
    guest_t *guest_img = vm_config->guest_image;
    guest_t *guest_dtb = vm_config->guest_dtb;
    guest_t *guest_initrd = vm_config->guest_initrd;

    if(guest_img == NULL) {
        abort("Guest vm are required when creating geust vm");
    }

    if(vm_config->ncpu > NCPU) {
        abort("The number of VCPUs cannot exceed %d", NCPU);
    }

    LOG_INFO("Creating guest vm ...\n");
    LOG_INFO("-->guest image name  is %s\n", guest_img->guest_name);
    LOG_INFO("-->guest image start is %x\n", guest_img->start_addr);
    LOG_INFO("-->guest image end   is %x\n", guest_img->end_addr);
    LOG_INFO("-->guest image size  is %x\n", guest_img->image_size);
    LOG_INFO("-->guest vm entry    is %x\n", vm_config->entry_addr);

    if(guest_dtb != NULL) {
        LOG_INFO("-->guest dtb name  is %s\n", guest_dtb->guest_name);
        LOG_INFO("-->guest dtb start is %x\n", guest_dtb->start_addr);
        LOG_INFO("-->guest dtb end   is %x\n", guest_dtb->end_addr);
        LOG_INFO("-->guest dtb size  is %x\n", guest_dtb->image_size);
    }

    if(guest_initrd != NULL) {
        LOG_INFO("-->guest initrd name  is %s\n", guest_initrd->guest_name);
        LOG_INFO("-->guest initrd start is %x\n", guest_initrd->start_addr);
        LOG_INFO("-->guest initrd end   is %x\n", guest_initrd->end_addr);
        LOG_INFO("-->guest initrd size  is %x\n", guest_initrd->image_size);
    }

    LOG_INFO("-->guest vcpus is    %d\n", vm_config->ncpu);
    LOG_INFO("-->guest ram size is %x\n", vm_config->ram_size);

    /* alloc a vm */
    vm_t *vm = (vm_t *)xmalloc(sizeof(vm_t));
    if (vm == NULL) {
        abort("Unable to alloc a vm, no memory");
    }

    vm_init(vm, vm_config);

    /* alloc the translation table base page */
    u64 *vttbr = alloc_one_page();
    if(vttbr == NULL) {
        abort("Unable to alloc the vttbr");
    }

    vm->vttbr = vttbr;

    /* map the normal memory and image */
    do_memory_mapping(vttbr, vm_config);
    /* map the device memory */
    do_device_mapping(vttbr, vm_config);

    /* set vcpu[0] ready */
    LOG_INFO("-->Set Guest vm vcpu[0] as ready\n");

    vm->vcpus[0]->state = VCPU_READY;

    return;
}