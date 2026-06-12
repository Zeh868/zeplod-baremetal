$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Split-Path -Parent $ScriptDir
$BuildRoot = Join-Path (Join-Path $ScriptDir 'build') 'windows'
$Toolchain = (Join-Path $RootDir 'cmake/toolchain-arm-none-eabi.cmake').Replace('\', '/')
$Examples = Get-Content (Join-Path $ScriptDir 'examples.txt') |
    Where-Object { $_ -and -not $_.StartsWith('#') }
$Failed = @()

foreach ($Example in $Examples) {
    $SourceDir = Join-Path $ScriptDir $Example
    $BuildDir = Join-Path $BuildRoot $Example
    Write-Host "=== Building $Example ==="

    & cmake -G 'MinGW Makefiles' -S $SourceDir -B $BuildDir `
        "-DCMAKE_TOOLCHAIN_FILE=$Toolchain"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for $Example" }

    & cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "Build failed for $Example" }

    Write-Host "=== Running $Example in QEMU ==="
    $Elf = Join-Path $BuildDir "$Example.elf"
    $Stdout = Join-Path $BuildDir 'qemu.stdout'
    $Stderr = Join-Path $BuildDir 'qemu.stderr'
    Remove-Item $Stdout, $Stderr -ErrorAction SilentlyContinue

    $Arguments = "-machine microbit -cpu cortex-m0 -kernel `"$Elf`" " +
        '--semihosting -display none'
    $Process = Start-Process qemu-system-arm -ArgumentList $Arguments `
        -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr
    Start-Sleep -Seconds 8
    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
    }

    $Output = (Get-Content $Stdout, $Stderr -ErrorAction SilentlyContinue) -join "`n"
    ($Output -split "`n" | Select-Object -First 30) | ForEach-Object {
        Write-Host $_
    }

    if ($Output -match 'EXAMPLE_.*: PASS') {
        Write-Host "$Example ... PASS"
    } else {
        Write-Host "$Example ... FAIL"
        $Failed += $Example
    }
}

if ($Failed.Count -gt 0) {
    Write-Error "Failed examples: $($Failed -join ', ')"
    exit 1
}

Write-Host 'All examples passed.'
