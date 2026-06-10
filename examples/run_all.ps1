$ErrorActionPreference = 'Stop'
$Examples = @('ultra_blink', 'core_sensor', 'full_system', 'interrupt_demo')
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Join-Path $ScriptDir '..'
$Failed = @()

function Invoke-External {
    param([scriptblock]$Command)
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $Command 2>&1 | Out-Null
    $code = $LASTEXITCODE
    $ErrorActionPreference = $prev
    return $code
}

Push-Location $ScriptDir

try {
    foreach ($ex in $Examples) {
        Write-Host "=== Building $ex ==="
        Set-Location $ex
        $toolchain = (Join-Path $RootDir 'cmake/toolchain-arm-none-eabi.cmake') -replace '\\', '/'
        if ((Invoke-External { cmake -G 'Unix Makefiles' -B build "-DCMAKE_TOOLCHAIN_FILE=$toolchain" . }) -ne 0) {
            throw "cmake configure failed for $ex"
        }
        if ((Invoke-External { cmake --build build }) -ne 0) {
            throw "cmake build failed for $ex"
        }

        Write-Host "=== Running $ex in QEMU ==="
        $elfPath = Join-Path (Get-Location) "build\$ex.elf"
        $job = Start-Job -ScriptBlock {
            param($elf)
            qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $elf --semihosting -display none 2>&1
        } -ArgumentList $elfPath

        $null = Wait-Job $job -Timeout 8
        Stop-Job $job -ErrorAction SilentlyContinue
        $prev = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $output = Receive-Job $job 2>&1
        $ErrorActionPreference = $prev
        Remove-Job $job -Force -ErrorAction SilentlyContinue

        Set-Location $ScriptDir

        # Print captured QEMU output so user can see main() logs
        Write-Host "--- QEMU output for $ex ---"
        $output | Select-Object -First 30 | ForEach-Object { Write-Host $_ }
        if ($output.Count -gt 30) { Write-Host "... (truncated)" }
        Write-Host "--- end of $ex output ---"

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
