param([Parameter(Mandatory=$true)][string]$Example)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Join-Path $ScriptDir '..'

function Invoke-External {
    param([scriptblock]$Command)
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $Command 2>&1 | Out-Null
    $code = $LASTEXITCODE
    $ErrorActionPreference = $prev
    if ($code -ne 0) { exit $code }
}

$ExampleDir = Join-Path $ScriptDir $Example
if (-not (Test-Path $ExampleDir)) {
    Write-Error "Example '$Example' not found in $ScriptDir"
    exit 1
}

Set-Location $ExampleDir

Write-Host "=== Building $Example ==="
$toolchain = (Join-Path $RootDir 'cmake/toolchain-arm-none-eabi.cmake') -replace '\\', '/'
Invoke-External { cmake -G 'Unix Makefiles' -B build "-DCMAKE_TOOLCHAIN_FILE=$toolchain" . }
Invoke-External { cmake --build build }

Write-Host "=== Running $Example in QEMU (interactive) ==="
Write-Host "Press Ctrl+C to quit QEMU"
Write-Host ""

$elfPath = Join-Path (Get-Location) "build\$Example.elf"
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $elfPath --semihosting -display none
