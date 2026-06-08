MEMORY {
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .text : {
        KEEP(*(.isr_vector))
        *(.text*)
    } > FLASH

    .data : {
        *(.data*)
    } > RAM AT > FLASH

    .bss : {
        *(.bss*)
    } > RAM
}
