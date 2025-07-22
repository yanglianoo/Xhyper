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