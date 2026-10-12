<#
.SYNOPSIS
    OWTF2: builds the TF2 mod and prints a short error summary.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/build.ps1
    powershell -ExecutionPolicy Bypass -File tools/build.ps1 -Regenerate
    powershell -ExecutionPolicy Bypass -File tools/build.ps1 -Full -Config Debug
#>
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Config = 'Release',
    [switch]$Regenerate,   # rerun VPC first (needed after editing .vpc/.vgc files)
    [switch]$Full          # build everything.sln (all tools) instead of game_tf.sln
)

$ErrorActionPreference = 'Stop'
$root   = Split-Path -Parent $PSScriptRoot
$src    = Join-Path $root 'src'
$logDir = Join-Path $PSScriptRoot 'logs'
$log    = Join-Path $logDir 'last-build.log'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

if ($Full) {
    $sln = Join-Path $src 'everything.sln'
    $gen = Join-Path $src 'createallprojects.bat'
} else {
    $sln = Join-Path $src 'game_tf.sln'
    $gen = Join-Path $src 'creategameprojects.bat'
}

# 1. Generate the solution with VPC if asked, or if it doesn't exist yet.
if ($Regenerate -or -not (Test-Path $sln)) {
    Write-Host "Generating projects with $(Split-Path -Leaf $gen)..."
    Push-Location $src
    try { & cmd.exe /c "`"$gen`"" } finally { Pop-Location }
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $sln)) {
        Write-Host "Project generation failed (exit code $LASTEXITCODE)." -ForegroundColor Red
        exit 1
    }
}

# 2. Find MSBuild from Visual Studio 2022.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Write-Host 'vswhere.exe not found. Is Visual Studio 2022 installed?' -ForegroundColor Red
    exit 1
}
$msbuild = & $vswhere -version '[17.0,18.0)' -products * -requires Microsoft.Component.MSBuild `
    -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    Write-Host 'MSBuild for Visual Studio 2022 not found.' -ForegroundColor Red
    exit 1
}

# 3. Pick the platform the solution defines for this configuration (prefer x64).
$slnText  = Get-Content $sln -Raw
$platform = if ($slnText -match [regex]::Escape("$Config|x64")) { 'x64' } else { 'Win32' }

# 4. Build.
Write-Host "Building $(Split-Path -Leaf $sln) ($Config|$platform)..."
& $msbuild $sln /m /nologo /v:minimal `
    "/p:Configuration=$Config" "/p:Platform=$platform" `
    /fl "/flp:LogFile=$log;Verbosity=normal"
$code = $LASTEXITCODE

# 5. Summarize.
$errors   = Select-String -Path $log -Pattern ': (fatal )?error ' -ErrorAction SilentlyContinue | ForEach-Object { $_.Line.Trim() } | Sort-Object -Unique
$warnings = (Select-String -Path $log -Pattern ': warning ' -ErrorAction SilentlyContinue | Measure-Object).Count

Write-Host ''
if ($code -eq 0) {
    Write-Host "BUILD SUCCEEDED ($warnings warning lines)." -ForegroundColor Green
} else {
    Write-Host "BUILD FAILED: $(@($errors).Count) unique error lines. First 40:" -ForegroundColor Red
    $errors | Select-Object -First 40 | ForEach-Object { Write-Host "  $_" }
}
Write-Host "Full log: $log"
exit $code
