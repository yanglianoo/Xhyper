.section ".text"

.global vectable
.balign 0x800
vectable:

  b .
.balign 0x80
  b .
.balign 0x80
  b .
.balign 0x80
  b .

.balign 0x80
  b el1_sync
.balign 0x80
  b el1_irq
.balign 0x80
  b .
.balign 0x80
  b .

.balign 0x80
  b .
.balign 0x80
  b .
.balign 0x80
  b .
.balign 0x80
  b .

.balign 0x80
  b .
.balign 0x80
  b .
.balign 0x80
  b .
.balign 0x80
  b .

el1_irq:
  bl el1_sync
  eret