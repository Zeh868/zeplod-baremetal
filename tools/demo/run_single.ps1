param([Parameter(Mandatory = $true)][string]$Example)

$ErrorActionPreference = 'Stop'
$DemoToolsDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = (Resolve-Path (Join-Path $DemoToolsDir '../..')).Path
$DemoDir = Join-Path $RootDir 'Demo'
$BuildDir = Join-Path (Join-Path (Join-Path $RootDir 'build/demo') 'windows') $Example
$Toolchain = (Join-Path $RootDir 'cmake/toolchain-arm-none-eabi.cmake').Replace('\', '/')
$KnownExamples = Get-Content (Join-Path $DemoToolsDir 'examples.txt')
$SourceDir = Join-Path $DemoDir $Example

if ($Example -notin $KnownExamples -or -not (Test-Path $SourceDir)) {
    Write-Error "Unknown example '$Example'"
    exit 1
}

Write-Host "=== Building $Example ==="
& cmake -G 'MinGW Makefiles' -S $SourceDir -B $BuildDir `
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "=== Running $Example in QEMU ==="
Write-Host 'Press Ctrl+C to stop.'
$Elf = Join-Path $BuildDir "$Example.elf"
& qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $Elf `
    --semihosting -display none
