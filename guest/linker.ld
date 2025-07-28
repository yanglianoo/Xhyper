OUTPUT_ARCH("aarch64")
EXTERN(_start)
ENTRY(_start)


SECTIONS
{
    . = 0x80200000;
    .text : {
      *(.text) *(.text.*)
    }

    .rodata : {
      *(.rodata) *(.rodata.*)
    }

    .data : {
      *(.data) *(.data.*)
    }

    . = ALIGN(4096);
    .bss : {
      __bss_start = .;
      *(.bss .bss.*)
      __bss_end = .;
    }
}