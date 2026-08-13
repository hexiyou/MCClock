# ── MCClock deployment / packaging script ──
# Builds the Release binaries, runs windeployqt, bundles the sound resources
# and produces a distributable zip.
#
# Usage:
#   pwsh tools/deploy.ps1                # Release build + deploy package
#   pwsh tools/deploy.ps1 -SkipBuild     # reuse existing build output
#   pwsh tools/deploy.ps1 -Configuration Debug
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$root     = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $root "build"
# MSVC multi-config generators place binaries under bin\<Configuration>
$binDir   = Join-Path $buildDir "bin\$Configuration"
$deployDir = Join-Path $root "deploy"
# Try VS 2022 first, fall back to VS 2025 if available
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) {
    $cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}
$qtBin    = "C:\Qt\6.8.3\msvc2022_64\bin"
$windeployqt = Join-Path $qtBin "windeployqt.exe"

# ── 1. Build ──
if (-not $SkipBuild) {
    Write-Host "[1/4] Building $Configuration ..."
    if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
        # Use VS 2026 (Visual Studio 18)
        & $cmake -S $root -B $buildDir -G "Visual Studio 18 2026" -A x64
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    }
    # Ensure a running MCClock instance does not lock the executable
    Stop-Process -Name MCClock -Force -ErrorAction SilentlyContinue
    & $cmake --build $buildDir --config $Configuration --target MCClock
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    & $cmake --build $buildDir --config $Configuration --target MCClock-CLI
    if ($LASTEXITCODE -ne 0) { throw "Build failed (CLI)" }
} else {
    Write-Host "[1/4] Skipping build (-SkipBuild)"
}

$guiExe = Join-Path $binDir "MCClock.exe"
$cliExe = Join-Path $binDir "MCClock-CLI.exe"
if (-not (Test-Path $guiExe)) { throw "Missing $guiExe - run without -SkipBuild first" }

# ── 2. windeployqt (Qt runtime dlls + plugins) ──
Write-Host "[2/4] Running windeployqt into $deployDir ..."
New-Item -ItemType Directory -Path $deployDir -Force | Out-Null
Copy-Item $guiExe (Join-Path $deployDir "MCClock.exe") -Force
if (Test-Path $cliExe) {
    Copy-Item $cliExe (Join-Path $deployDir "MCClock-CLI.exe") -Force
}
& $windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw --no-ffmpeg --exclude-plugins ffmpegmediaplugin `
    --dir $deployDir (Join-Path $deployDir "MCClock.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# Remove FFmpeg libraries and plugin (no longer needed with --no-ffmpeg)
$ffmpegFiles = @("avcodec-*.dll", "avformat-*.dll", "avutil-*.dll", "swresample-*.dll", "swscale-*.dll")
foreach ($pattern in $ffmpegFiles) {
    Get-ChildItem -Path $deployDir -Filter $pattern -ErrorAction SilentlyContinue | Remove-Item -Force
}
$ffPlugin = Join-Path $deployDir "multimedia\ffmpegmediaplugin.dll"
if (Test-Path $ffPlugin) { Remove-Item $ffPlugin -Force }

# ── 3. Sound resources (alarm ringtones + hourly chime voice files) ──
Write-Host "[3/5] Copying sound resources ..."
$soundsSrc = Join-Path $root "resources\sounds"
if (-not (Test-Path $soundsSrc)) { throw "Missing sound resources: $soundsSrc" }
$soundsDst = Join-Path $deployDir "sounds"
if (Test-Path $soundsDst) { Remove-Item $soundsDst -Recurse -Force }
Copy-Item $soundsSrc $soundsDst -Recurse -Force

# ── 4. Qt Chinese translation (for QFontDialog and other standard dialogs) ──
Write-Host "[4/5] Copying Qt translation ..."
$transSrc = Join-Path $binDir "translations\qtbase_zh_CN.qm"
if (Test-Path $transSrc) {
    $transDstDir = Join-Path $deployDir "translations"
    if (-not (Test-Path $transDstDir)) { New-Item -ItemType Directory -Path $transDstDir -Force | Out-Null }
    Copy-Item $transSrc (Join-Path $transDstDir "qtbase_zh_CN.qm") -Force
} else {
    Write-Host "  Warning: $transSrc not found, skipping translation"
}

# ── 5. Zip package ──
$stamp = Get-Date -Format "yyyyMMdd-HHmm"
$zipPath = Join-Path $root "MCClock-deploy-$stamp.zip"
Write-Host "[5/5] Creating $zipPath ..."
Compress-Archive -Path (Join-Path $deployDir "*") -DestinationPath $zipPath -Force

Write-Host ""
Write-Host "Deployment ready:"
Write-Host "  Folder      : $deployDir"
Write-Host "  Zip         : $zipPath"
Write-Host "  Sounds      : $( (Get-ChildItem (Join-Path $deployDir 'sounds')).Count ) files bundled"
if (Test-Path (Join-Path $deployDir "translations")) {
    Write-Host "  Translation : $( (Get-ChildItem (Join-Path $deployDir 'translations')).Count ) files bundled"
}
