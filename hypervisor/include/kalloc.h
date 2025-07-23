#ifndef __KALLOC_H__
#define __KALLOC_H__

void  kalloc_init();
void *alloc_one_page();
void  free_one_page(void *p);

#endif