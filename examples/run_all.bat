@echo off
setlocal enabledelayedexpansion

rem Get the directory where this batch file resides
set "EXAMPLES_DIR=%~dp0"
set "ROOT_DIR=%EXAMPLES_DIR%..\"

cd /d "%EXAMPLES_DIR%"

set EXAMPLES=ultra_blink core_sensor full_system
set FAILED=

for %%e in (%EXAMPLES%) do (
    echo === Building %%e ===
    cd "%%e"
    cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE="%ROOT_DIR%cmake\toolchain-arm-none-eabi.cmake" . >nul 2>&1
    cmake --build build >nul 2>&1

    echo === Running %%e in QEMU ===
    start /B qemu-system-arm -machine microbit -cpu cortex-m0 -kernel "build\%%e.elf" --semihosting -nographic -serial stdio > "..\%%e.log" 2>&1
    timeout /t 10 >nul
    taskkill /F /IM qemu-system-arm.exe >nul 2>&1

    findstr /C:"EXAMPLE_" "..\%%e.log" | findstr /C:"PASS" >nul
    if !errorlevel! == 0 (
        echo %%e ... PASS
    ) else (
        echo %%e ... FAIL
        set FAILED=!FAILED! %%e
    )
    cd ..
)

if "!FAILED!"=="" (
    echo All examples passed.
    exit /b 0
) else (
    echo Failed:!FAILED!
    exit /b 1
)
