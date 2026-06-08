set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(CMAKE_C_COMPILER riscv-none-elf-gcc)
set(CMAKE_ASM_COMPILER riscv-none-elf-gcc)
set(CMAKE_OBJCOPY riscv-none-elf-objcopy)

set(CMAKE_C_FLAGS "-march=rv32imac -mabi=ilp32 -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
