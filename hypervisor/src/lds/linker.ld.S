#include "layout.h"

EXTERN(_start)
ENTRY(_start)

SECTIONS
{
    /* hyper image link virtual addr, here equal to phys addr */
    . = HIMAGE_VADDR;

    .text.boot : {
      KEEP(*(.text.boot))
    }

    . = ALIGN(4096);

    .text : {
      *(.text) *(.text.*)
    }

    .rodata : {
      *(.rodata) *(.rodata.*)
    }

    .data : {
      __data_start = .;
      *(.data) *(.data.*)
      __data_end = .;
    }
    . = ALIGN(4096);

    .bss : {
      __bss_start = .;
      *(.bss .bss.*)
      __bss_end = .;
    }
    . = ALIGN(4096);

    blk_pool_start = .;
    . += (SZ_4K * 3);
    blk_pool_end   = .;
    . = ALIGN(4096);
    
    HIMAGE_END = .;
}