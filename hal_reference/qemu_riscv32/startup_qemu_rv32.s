    .section .text.reset
    .globl _start
_start:
    la sp, _estack
    call main
    j .

    .section .bss
    .globl _estack
_estack:
    .space 4096
