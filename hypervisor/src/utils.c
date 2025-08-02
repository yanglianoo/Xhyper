#include <utils.h>

/*---------------------strlen-------------------------------*/
u64 strlen(const char *s)
{
    u64 len = 0;
    while (*s++ != '\0')
        len++;
    return len;
}

/*---------------------memset-------------------------------*/
void *memset(void *dst, int c, u64 n) {
    char *d = dst;

    while(n-- > 0) {
        *d++ = c;
    }

    return dst;
}

/*---------------------memcpy-------------------------------*/
void *memcpy(void *dst, const void *src, size_t count)
{
    char *pTo = (char *)dst;
    char *pFrom = (char *)src;
    while (count-- > 0)
    {
        *pTo++ = *pFrom++;
    }
    return dst;
}

/*---------------------strcpy-------------------------------*/
char *strcpy(char *dst, const char *src)
{
    char *r = dst;
    while((*dst++ = *src++) != 0);
    return r;
}