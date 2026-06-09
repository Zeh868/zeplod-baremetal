$ErrorActionPreference = 'Stop'
$Examples = @('ultra_blink', 'core_sensor', 'full_system')
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Join-Path $ScriptDir '..'
$Failed = @()

Push-Location $ScriptDir

try {
    foreach ($ex in $Examples) {
        Write-Host "=== Building $ex ==="
        Set-Location $ex
        $null = cmake -G 'Unix Makefiles' -B build -DCMAKE_TOOLCHAIN_FILE="$RootDir\cmake\toolchain-arm-none-eabi.cmake" . 2>&1
        $null = cmake --build build 2>&1

        Write-Host "=== Running $ex in QEMU ==="
        $elfPath = Join-Path (Get-Location) "build\$ex.elf"
        $job = Start-Job -ScriptBlock {
            param($elf)
            qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $elf --semihosting -nographic -serial stdio 2>&1
        } -ArgumentList $elfPath

        Start-Sleep -Seconds 8
        Stop-Job $job -ErrorAction SilentlyContinue
        $output = Receive-Job $job
        Remove-Job $job

        Set-Location $ScriptDir
        if ($output -match 'EXAMPLE_.*PASS') {
            Write-Host "$ex ... PASS"
        } else {
            Write-Host "$ex ... FAIL"
            $Failed += $ex
        }
    }

    if ($Failed.Count -eq 0) {
        Write-Host "All examples passed."
        exit 0
    } else {
        Write-Host "Failed: $($Failed -join ' ')"
        exit 1
    }
} finally {
    Pop-Location
}
