#ifndef __LIBS_H__
#define __LIBS_H__


#include <stddef.h>
#include "types.h"

u64 strlen(const char *s);
void *memset(void *dst, int c, u64 n);
void *memcpy(void *dst, const void *src, size_t count);

#endif