param(
    [string]$Version = (Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue | Select-Object -First 1).Trim(),
    [string]$OutputDir = (Join-Path $PSScriptRoot "..\release")
)

$ErrorActionPreference = 'Stop'
if (-not $Version) { throw "Version konnte nicht ermittelt werden. Bitte VERSION pflegen oder -Version angeben." }

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# Windows artifacts (MSVC Release)
$winBin = Join-Path $root "build-msvc\Release\themis_server.exe"
if (Test-Path $winBin) {
    $dst = Join-Path $OutputDir ("themisdb-v$Version-windows-x64.exe")
    Copy-Item $winBin $dst -Force
    Write-Host "Windows artifact collected: $dst" -ForegroundColor Green

    $winDll = Join-Path $root "build-msvc\Release\themis_core.dll"
    if (Test-Path $winDll) {
        $dllDst = Join-Path $OutputDir ("themisdb-v$Version-windows-x64-themis_core.dll")
        Copy-Item $winDll $dllDst -Force
        Write-Host "Windows DLL collected: $dllDst" -ForegroundColor Green
    }
}

# Linux artifacts (prefer WSL build directory)
$linuxCandidates = @(
    "build-wsl/Release/themis_server",
    "build-linux/Release/themis_server",
    "build-linux/themis_server"
)

$linuxBin = $null
foreach ($c in $linuxCandidates) {
    $full = Join-Path $root $c
    if (Test-Path $full) { $linuxBin = $full; break }
}

if ($linuxBin) {
    $dst = Join-Path $OutputDir ("themisdb-v$Version-linux-x64")
    Copy-Item $linuxBin $dst -Force
    Write-Host "Linux artifact collected: $dst" -ForegroundColor Green

    $linuxSoCandidates = @(
        "build-wsl/Release/libthemis_core.so",
        "build-linux/Release/libthemis_core.so",
        "build-linux/libthemis_core.so"
    )

    foreach ($soPath in $linuxSoCandidates) {
        $fullSo = Join-Path $root $soPath
        if (Test-Path $fullSo) {
            $soDst = Join-Path $OutputDir ("themisdb-v$Version-linux-x64-libthemis_core.so")
            Copy-Item $fullSo $soDst -Force
            Write-Host "Linux .so collected: $soDst" -ForegroundColor Green
            break
        }
    }
}
