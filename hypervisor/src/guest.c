#include <types.h>
#include <guest.h>

/* Guest vm image are packaged in X-Hyper Image with aarch64-linux-gnu--ld */
extern char _binary_build_Guest_VM_start[];
extern char _binary_build_Guest_VM_end[];
extern char _binary_build_Guest_VM_size[];

guest_t guest_vm_image = {
    .guest_name  = "guest_vm",
    .start_addr  = (u64)_binary_build_Guest_VM_start,
    .image_size  = (u64)_binary_build_Guest_VM_size,
};