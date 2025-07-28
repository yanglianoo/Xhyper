.section .text, "ax"
.global  _start
.type    _start, function
.align 4

_start:
    mrs  x0, CurrentEL

    /* Print 0 to PL011 */
    mov  x3, 0x09000000
    mov  x2, 0x30
    str  w2, [x3]

    /* spin here */
    b       .