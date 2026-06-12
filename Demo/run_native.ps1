param([Parameter(Mandatory = $true)][string]$Example)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$SourceDir = Join-Path $ScriptDir $Example
$BuildDir = Join-Path (Join-Path (Join-Path $ScriptDir 'build') 'native') $Example

if (-not (Test-Path $SourceDir)) {
    Write-Error "Unknown example '$Example'"
    exit 1
}

cmake -S $SourceDir -B $BuildDir -DNATIVE_SIM_BUILD=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $BuildDir --config Debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$Exe = Join-Path $BuildDir "Debug\$Example.exe"
if (-not (Test-Path $Exe)) {
    $Exe = Join-Path $BuildDir "$Example.exe"
}
$out = & $Exe 2>&1 | Out-String
Write-Host $out
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($out -notmatch ': PASS') { Write-Error 'Example did not report PASS'; exit 1 }
