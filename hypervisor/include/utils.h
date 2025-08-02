#ifndef __UTILS_H__
#define __UTILS_H__

#include <types.h>
#include <stddef.h>

u64 strlen(const char *s);
void *memset(void *dst, int c, u64 n);
void *memcpy(void *dst, const void *src, size_t count);
char *strcpy(char *dst, const char *src);

#endif