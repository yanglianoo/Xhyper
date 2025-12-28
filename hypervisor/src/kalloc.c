#include <stddef.h>
#include "types.h"
#include "utils.h"
#include "spinlock.h"
#include "layout.h"
#include "xlog.h"
extern char HIMAGE_END[];

struct header {
    struct header *next;
};

struct {
    spinlock_t lock;
    struct header *freelist;
} pages;

void *alloc_one_page()
{
    arch_spin_lock(&pages.lock);
    struct header *page = pages.freelist;

    if(page == NULL) {
        return NULL;
    }

    pages.freelist = page->next;
    arch_spin_unlock(&pages.lock);

    memset((char *)page, 0, SZ_4K);
    return (void *)page;
}

void free_one_page(void *p)
{
    if(p == NULL) {
        return;
    }

    memset(p, 0, SZ_4K);
    struct header *fp = (struct header *)p;
    arch_spin_lock(&pages.lock);

    fp->next = pages.freelist;
    pages.freelist = fp;

    arch_spin_unlock(&pages.lock);
}

void kalloc_init()
{
    arch_spinlock_init(&pages.lock);
    for(u64 page = (u64)HIMAGE_END; page < PHYEND;  page +=  SZ_4K) {
        free_one_page((void *)page);
    }

    LOG_INFO("Kalloc have been initialized: \n");
    LOG_INFO("Kalloc: page start addr : %x\n", (u64)HIMAGE_END);
    LOG_INFO("Kalloc: page numbers    : %x\n", (PHYEND - (u64)HIMAGE_END)/PAGESIZE);
}

