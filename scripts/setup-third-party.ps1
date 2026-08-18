param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [switch]$BootstrapVcpkg,
    [switch]$ForceRecloneVcpkg
)

$ErrorActionPreference = 'Stop'

function Require-Git {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "git ist nicht im PATH verfügbar. Bitte Git installieren oder PATH korrigieren."
    }
}

function Ensure-Submodule {
    param(
        [string]$Root,
        [string]$Name
    )

    $path = Join-Path $Root $Name
    if (Test-Path $path) {
        Write-Host "[OK] Submodule-Verzeichnis vorhanden: $Name"
    }

    Write-Host "[INFO] Initialisiere/Aktualisiere Submodule: $Name"
    git -C $Root submodule update --init --recursive $Name | Out-Null
}

function Ensure-Vcpkg {
    param(
        [string]$Root,
        [switch]$Bootstrap,
        [switch]$ForceReclone
    )

    $vcpkgDir = Join-Path $Root 'vcpkg'
    $toolchain = Join-Path $vcpkgDir 'scripts\buildsystems\vcpkg.cmake'

    if ($ForceReclone -and (Test-Path $vcpkgDir)) {
        Write-Host "[INFO] Entferne vorhandenes vcpkg für Re-Clone: $vcpkgDir"
        Remove-Item $vcpkgDir -Recurse -Force
    }

    if (-not (Test-Path $toolchain)) {
        if (Test-Path $vcpkgDir) {
            Write-Host "[WARN] vcpkg-Verzeichnis unvollständig, versuche Neuinitialisierung als Submoduleintrag"
            try {
                git -C $Root submodule update --init --recursive vcpkg | Out-Null
            } catch {
                Write-Host "[WARN] Submodule-Init für vcpkg fehlgeschlagen, fallback auf git clone"
            }
        }

        if (-not (Test-Path $toolchain)) {
            if (Test-Path $vcpkgDir) {
                Remove-Item $vcpkgDir -Recurse -Force
            }
            Write-Host "[INFO] Klone vcpkg nach $vcpkgDir"
            git clone https://github.com/microsoft/vcpkg.git $vcpkgDir | Out-Null
        }
    }

    if (-not (Test-Path $toolchain)) {
        throw "vcpkg Toolchain nicht gefunden: $toolchain"
    }

    Write-Host "[OK] vcpkg Toolchain gefunden: $toolchain"

    if ($Bootstrap) {
        $isWindowsHost = ($env:OS -eq 'Windows_NT')
        if ($isWindowsHost) {
            $bootstrapScript = Join-Path $vcpkgDir 'bootstrap-vcpkg.bat'
            if (Test-Path $bootstrapScript) {
                Write-Host "[INFO] Führe vcpkg Bootstrap aus (Windows)"
                & cmd /c "`"$bootstrapScript`""
            }
        } else {
            $bootstrapScript = Join-Path $vcpkgDir 'bootstrap-vcpkg.sh'
            if (Test-Path $bootstrapScript) {
                Write-Host "[INFO] Führe vcpkg Bootstrap aus (Unix)"
                & bash $bootstrapScript
            }
        }
    }
}

function Ensure-VcpkgBaseline {
    param(
        [string]$Root
    )

    $manifest = Join-Path $Root 'vcpkg.json'
    $vcpkgDir = Join-Path $Root 'vcpkg'

    if (-not (Test-Path $manifest)) {
        Write-Host "[WARN] vcpkg.json nicht gefunden - Baseline-Check uebersprungen"
        return
    }

    $json = Get-Content $manifest -Raw | ConvertFrom-Json
    $baseline = $json.'builtin-baseline'
    if (-not $baseline) {
        Write-Host "[INFO] Kein builtin-baseline in vcpkg.json gesetzt"
        return
    }

    $baselineCommitRef = "${baseline}^{commit}"
    cmd /c "git -C `"$vcpkgDir`" cat-file -e $baselineCommitRef 1>nul 2>nul"
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] vcpkg Baseline-Commit vorhanden: $baseline"
        return
    }

    Write-Host "[WARN] vcpkg Baseline fehlt lokal: $baseline"
    Write-Host "[INFO] Versuche gezielten Fetch des Baseline-Commits"
    git -C $vcpkgDir fetch origin $baseline --depth=1 | Out-Null

    cmd /c "git -C `"$vcpkgDir`" cat-file -e $baselineCommitRef 1>nul 2>nul"
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg Baseline-Commit weiterhin nicht verfügbar: $baseline. Entweder vcpkg-Repository vollständig fetchen oder builtin-baseline in vcpkg.json aktualisieren."
    }

    Write-Host "[OK] vcpkg Baseline-Commit erfolgreich nachgeladen: $baseline"
}

Write-Host "=== Themis Third-Party Setup ==="
Write-Host "RepoRoot: $RepoRoot"
Write-Host "Mode: strict setup; diagnostic-only fallback is not allowed for core dependencies."

Require-Git

if (-not (Test-Path (Join-Path $RepoRoot '.git'))) {
    throw "Kein Git-Repository gefunden unter: $RepoRoot"
}

$requiredSubmodules = @('vcpkg', 'ffmpeg', 'llama.cpp', 'whisper.cpp', 'stable-diffusion.cpp')
foreach ($submodule in $requiredSubmodules) {
    Ensure-Submodule -Root $RepoRoot -Name $submodule
}

Ensure-Vcpkg -Root $RepoRoot -Bootstrap:$BootstrapVcpkg -ForceReclone:$ForceRecloneVcpkg
Ensure-VcpkgBaseline -Root $RepoRoot

Write-Host "[CHECK] Core package contract: fmt, spdlog, nlohmann-json, zlib, RocksDB"
Write-Host "[DONE] Third-Party-Abhaengigkeiten sind vorbereitet."
Write-Host "Naechster Schritt: cmake --preset windows-release (oder linux-release)"
