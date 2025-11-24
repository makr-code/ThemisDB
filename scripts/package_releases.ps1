param(
    [switch]$SkipStrip
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Package ThemisDB Release Artefakte ===" -ForegroundColor Cyan

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $root
Set-Location $repo

# Ensure dist folder
$dist = Join-Path $repo 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null

function Sha256($path) {
    (Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLower()
}

# Helper: run a bash command in the qnap builder (for linux strip/tar)
function Run-Docker([string]$command) {
    & docker run --rm -v "${repo}:/src" -w /src themisdb-qnap-builder:latest bash -lc $command
    if ($LASTEXITCODE -ne 0) { throw "Docker command failed: $command" }
}

# Package QNAP Linux (build-qnap)
$binQnap = Join-Path $repo 'build-qnap/themis_server'
if (Test-Path $binQnap) {
    Write-Host "[QNAP] Packe build-qnap/themis_server" -ForegroundColor Yellow
    $outTar = Join-Path $dist 'themis_server_qnap_x64-linux.tar.gz'
    $cmd = @()
    $cmd += 'set -e'
    $cmd += 'mkdir -p /src/dist/stage-qnap'
    $cmd += 'cp /src/build-qnap/themis_server /src/dist/stage-qnap/'
    if (-not $SkipStrip) { $cmd += 'strip /src/dist/stage-qnap/themis_server' }
    $cmd += 'tar -C /src/dist/stage-qnap -czf /src/dist/themis_server_qnap_x64-linux.tar.gz themis_server'
    $cmd += 'rm -rf /src/dist/stage-qnap'
    Run-Docker ("$($cmd -join '; ')")
    $hash = Sha256 $outTar
    "$hash  $(Split-Path -Leaf $outTar)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
}

# Package Linux GCC Release (build-linux-gcc-release) – optional
$binLinuxGcc = Join-Path $repo 'build-linux-gcc-release/themis_server'
if (Test-Path $binLinuxGcc) {
    Write-Host "[Linux GCC] Packe build-linux-gcc-release/themis_server" -ForegroundColor Yellow
    $outTar2 = Join-Path $dist 'themis_server_linux_gcc_x64.tar.gz'
    $cmd = @()
    $cmd += 'set -e'
    $cmd += 'mkdir -p /src/dist/stage-gcc'
    $cmd += 'cp /src/build-linux-gcc-release/themis_server /src/dist/stage-gcc/'
    if (-not $SkipStrip) { $cmd += 'strip /src/dist/stage-gcc/themis_server' }
    $cmd += 'tar -C /src/dist/stage-gcc -czf /src/dist/themis_server_linux_gcc_x64.tar.gz themis_server'
    $cmd += 'rm -rf /src/dist/stage-gcc'
    Run-Docker ("$($cmd -join '; ')")
    $hash = Sha256 $outTar2
    "$hash  $(Split-Path -Leaf $outTar2)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
}

# Package Windows MSVC Release (build-msvc-ninja-release)
$binWin = Join-Path $repo 'build-msvc-ninja-release/themis_server.exe'
if (-not (Test-Path $binWin)) {
    # Fallback für Multi-Config VS Generator (Release Unterordner)
    $binWinRelease = Join-Path $repo 'build-msvc-ninja-release/Release/themis_server.exe'
    if (Test-Path $binWinRelease) {
        Write-Host "[Windows] Fallback: kopiere Release/themis_server.exe ins Root" -ForegroundColor Yellow
        Copy-Item $binWinRelease $binWin -Force
    }
}
if (Test-Path $binWin) {
    Write-Host "[Windows] Packe build-msvc-ninja-release/themis_server.exe" -ForegroundColor Yellow
    $outZip = Join-Path $dist 'themis_server_windows_x64.zip'
    $stage = Join-Path $dist 'stage-win'
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    Copy-Item $binWin -Destination $stage -Force
    if (Test-Path (Join-Path $repo 'README.md')) { Copy-Item (Join-Path $repo 'README.md') -Destination $stage -Force }
    if (Test-Path (Join-Path $repo 'LICENSE')) { Copy-Item (Join-Path $repo 'LICENSE') -Destination $stage -Force }
    if (Test-Path $outZip) { Remove-Item $outZip -Force }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $outZip -Force
    Remove-Item $stage -Recurse -Force
    $hash = Sha256 $outZip
    "$hash  $(Split-Path -Leaf $outZip)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
} else {
    Write-Warning "Windows Binary nicht gefunden (erwartet build-msvc-ninja-release). Überspringe Packaging."
}

Write-Host "Fertig. Artefakte in: $dist" -ForegroundColor Green
