    .syntax unified
    .cpu cortex-m0
    .thumb

    .section .text.Reset_Handler
    .globl Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    bl SystemInit
    bl main
    b .

    .section .text.Default_Handler
    .globl Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b .

    .section .isr_vector
    .globl _vector_table
_vector_table:
    .word _estack
    .word Reset_Handler
    .word Default_Handler /* NMI */
    .word Default_Handler /* HardFault */
