#include <pl011.h>
#include <types.h>
#include <utils.h>

#define va_list         __builtin_va_list
#define va_start(v, l)  __builtin_va_start(v, l)
#define va_arg(v, l)    __builtin_va_arg(v, l)
#define va_end(v)       __builtin_va_end(v)
#define va_copy(d, s)   __builtin_va_copy(d, s)

enum printopt {
    PRINT_0X      = 1 << 0,
    ZERO_PADDING  = 1 << 1,
};

static void print64(s64 num, int base, bool sign, int digit, enum printopt opt)
{
    char buf[sizeof(num) * 8 + 1] = {0};
    char *end = buf + sizeof(buf);
    char *cur = end - 1;
    u64 unum;
    bool neg = false;

    if(sign && num < 0) {
        unum = (u64)(-(num + 1)) + 1;
        neg = true;
    } else {
        unum = (u64)num;
    }

    do {
        *--cur = "0123456789abcdef"[unum % base];
    } while(unum /= base);

    if(opt & PRINT_0X) {
        *--cur = 'x';
        *--cur = '0';
    }

    if(neg)
        *--cur = '-';

    int len = strlen(cur);
    if(digit > 0) {
        while(digit-- > len)
        pl011_putc(' ');
    }
    pl011_puts(cur);
    if(digit < 0) {
        digit = -digit;
        while(digit-- > len)
            pl011_putc(' ');
    }
}

static bool isdigit(char c)
{
    return '0' <= c && c <= '9';
}

static const char *fetch_digit(const char *fmt, int *digit) {
    int n = 0, neg = 0;

    if(*fmt == '-') {
        fmt++;
        neg = 1;
    }

    while(isdigit(*fmt)) {
        n = n * 10 + *fmt++ - '0';
    }

    *digit = neg? -n : n;
    return fmt;
}

static int vprintf(const char *fmt, va_list ap)
{
    char *s;
    void *p;
    int digit = 0;

    for(; *fmt; fmt++) {
        char c = *fmt;
        if(c == '%') {
            fmt++;
            fmt = fetch_digit(fmt, &digit);
            switch(c = *fmt) {
            case 'd':
                print64(va_arg(ap, s32), 10, true, digit, 0);
                break;
            case 'u':
                print64(va_arg(ap, u32), 10, false, digit, 0);
                break;
            case 'x':
                print64(va_arg(ap, u64), 16, false, digit, 0);
                break;
            case 'p':
                p = va_arg(ap, void *);
                print64((u64)p, 16, false, digit, PRINT_0X);
            break;
                case 'c':
                pl011_putc(va_arg(ap, int));
                break;
            case 's':
                s = va_arg(ap, char *);
                if(!s)
                s = "(null)";
                pl011_puts(s);
                break;
            case '%':
                pl011_putc('%');
                break;
            default:
                pl011_putc('%');
                pl011_putc(c);
                break;
            }
        } else {
            pl011_putc(c);
        }
    }

    return 0;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return 0;
}

void abort(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    printf("[X-Hyper abort]: ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);

    for(;;)
        asm volatile("wfi");
}

