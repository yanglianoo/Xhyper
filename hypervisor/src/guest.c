#include <types.h>
#include <guest.h>

/* Guest vm image are packaged in X-Hyper Image with aarch64-linux-gnu--ld */
extern char _binary_Image_start[];
extern char _binary_Image_end[];
extern char _binary_Image_size[];

/* linux virt dtb */
extern char _binary_virt_dtb_start[];
extern char _binary_virt_dtb_end[];
extern char _binary_virt_dtb_size[];

#if 0
guest_t guest_vm_image = {
    .guest_name  = "guest_vm",
    .start_addr  = (u64)_binary_Guest_VM_start,
    .end_addr    = (u64)_binary_Guest_VM_end,
    .image_size  = (u64)_binary_Guest_VM_size,
};
#endif

guest_t guest_vm_image = {
    .guest_name  = "guest_vm",
    .start_addr  = (u64)_binary_Image_start,
    .end_addr    = (u64)_binary_Image_end,
    .image_size  = (u64)_binary_Image_size,
};

guest_t guest_virt_dtb = {
    .guest_name  = "virt_dtb",
    .start_addr  = (u64)_binary_virt_dtb_start,
    .end_addr    = (u64)_binary_virt_dtb_end,
    .image_size  = (u64)_binary_virt_dtb_size,
};