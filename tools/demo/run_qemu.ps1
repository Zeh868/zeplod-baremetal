param(
    [Parameter(Mandatory = $true)][string]$Example,
    [int]$TimeoutSec = 60
)

$ErrorActionPreference = 'Stop'
$DemoToolsDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = (Resolve-Path (Join-Path $DemoToolsDir '../..')).Path
$DemoDir = Join-Path $RootDir 'Demo'
$SourceDir = Join-Path $DemoDir $Example
$BuildDir = Join-Path (Join-Path (Join-Path $RootDir 'build/demo') 'qemu') $Example
$Toolchain = (Join-Path $RootDir 'cmake/toolchain-arm-none-eabi.cmake').Replace('\', '/')
$KnownExamples = Get-Content (Join-Path $DemoToolsDir 'examples.txt')
$Qemu = (Get-Command qemu-system-arm -ErrorAction SilentlyContinue).Source

if ($Example -notin $KnownExamples -or -not (Test-Path $SourceDir)) {
    Write-Error "Unknown example '$Example'"
    exit 1
}
if (-not $Qemu) {
    Write-Error 'qemu-system-arm not found in PATH'
}

cmake -G 'MinGW Makefiles' -S $SourceDir -B $BuildDir `
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$Elf = Join-Path $BuildDir "$Example.elf"
$outFile = Join-Path $env:TEMP "zeplod_qemu_$Example.txt"
$p = Start-Process -FilePath $Qemu -ArgumentList @(
    '-machine', 'microbit',
    '-cpu', 'cortex-m0',
    '-kernel', $Elf,
    '--semihosting',
    '-display', 'none'
) -PassThru -NoNewWindow -RedirectStandardOutput $outFile

$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSec)
while (-not $p.HasExited -and [DateTime]::UtcNow -lt $deadline) {
    Start-Sleep -Milliseconds 200
    $partial = Get-Content $outFile -Raw -ErrorAction SilentlyContinue
    if ($partial -match ': PASS' -or $partial -match ': FAIL') {
        break
    }
}
$out = Get-Content $outFile -Raw -ErrorAction SilentlyContinue
if (-not $p.HasExited) {
    $p.Kill()
}
Write-Host $out
if ($out -notmatch ': PASS' -and $out -notmatch ': FAIL') {
    Write-Error "QEMU example '$Example' timed out without result"
    exit 1
}
if ($out -notmatch ': PASS') {
    exit 1
}
