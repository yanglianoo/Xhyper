#ifndef __GUEST_H__
#define __GUEST_H__


#include <types.h>

typedef struct guest {
    char *guest_name;
    u64   start_addr;
    u64   image_size;
} guest_t;

#endif