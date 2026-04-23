#!/usr/bin/env pwsh
<#
.SYNOPSIS
    ThemisDB Documentation Database Builder — manuelles Pre-Build-Tool

.DESCRIPTION
    Dieses Script baut die Dokumentation aus docs/ als ThemisDB-kompatible
    RocksDB-Datenbank und legt das Ergebnis in artifacts/docs-db/ ab.
    Es muss einmalig manuell ausgeführt werden, bevor der Docker-Build gestartet wird.

    Workflow:
      1. themis_docs_builder kompilieren (CMake, optional)
      2. docs/ scannen und Markdown-Dateien parsen
      3. Ergebnis-DB in artifacts/docs-db/ schreiben
      4. STAMP-Datei für Wiederbau-Erkennung schreiben

.PARAMETER DocsDir
    Pfad zum docs-Verzeichnis (Standard: <Repo-Root>/docs)

.PARAMETER OutputDir
    Ausgabepfad für die RocksDB-Datenbank (Standard: <Repo-Root>/artifacts/docs-db)

.PARAMETER ToolBuildDir
    Build-Verzeichnis für themis_docs_builder (Standard: <Repo-Root>/build-docs-builder)

.PARAMETER Force
    Erzwingt Neubau auch wenn STAMP-Datei existiert

.PARAMETER SkipBuild
    Überspringt das Kompilieren des Tools (Tool-Exe muss bereits existieren)

.EXAMPLE
    # Erster Lauf: Tool bauen + DB erzeugen
    .\tools\build_docs_db.ps1

.EXAMPLE
    # Nur DB neu erzeugen, Tool-Compile überspringen
    .\tools\build_docs_db.ps1 -SkipBuild

.EXAMPLE
    # Erzwungener Neubau
    .\tools\build_docs_db.ps1 -Force
#>

[CmdletBinding()]
param(
    [string]$DocsDir     = '',
    [string]$OutputDir   = '',
    [string]$ToolBuildDir = '',
    [switch]$Force,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ── Pfade auflösen ──────────────────────────────────────────────────────────
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if (-not $DocsDir)      { $DocsDir      = Join-Path $RepoRoot 'docs' }
if (-not $OutputDir)    { $OutputDir    = Join-Path $RepoRoot 'artifacts' 'docs-db' }
if (-not $ToolBuildDir) { $ToolBuildDir = Join-Path $RepoRoot 'build-docs-builder' }

$ToolSrcDir  = Join-Path $RepoRoot 'tools' 'themis_docs_builder'
$StampFile   = Join-Path $OutputDir '.build-stamp'
$ToolExe     = Join-Path $ToolBuildDir 'themis_docs_builder.exe'
if ($IsLinux -or $IsMacOS) {
    $ToolExe = Join-Path $ToolBuildDir 'themis_docs_builder'
}

Write-Host ''
Write-Host '═══════════════════════════════════════════════════════════' -ForegroundColor Cyan
Write-Host '  ThemisDB Docs-DB Builder' -ForegroundColor Cyan
Write-Host '═══════════════════════════════════════════════════════════' -ForegroundColor Cyan
Write-Host "  Repo    : $RepoRoot"
Write-Host "  Docs    : $DocsDir"
Write-Host "  Output  : $OutputDir"
Write-Host ''

# ── Voraussetzungen prüfen ───────────────────────────────────────────────────
if (-not (Test-Path $DocsDir)) {
    Write-Error "docs-Verzeichnis nicht gefunden: $DocsDir"
    exit 1
}

# ── STAMP-Prüfung (Wiederbau nur bei Änderungen) ─────────────────────────────
if (-not $Force -and (Test-Path $StampFile) -and (Test-Path $OutputDir)) {
    $stampTime = (Get-Item $StampFile).LastWriteTime
    $newerDocs = Get-ChildItem $DocsDir -Recurse -File -ErrorAction SilentlyContinue |
                 Where-Object { $_.LastWriteTime -gt $stampTime } |
                 Select-Object -First 1
    if (-not $newerDocs) {
        Write-Host '[SKIP] Docs-DB ist aktuell (keine Änderungen seit letztem Build).' -ForegroundColor Green
        Write-Host "       Verwende -Force um neu zu bauen."
        exit 0
    }
    Write-Host '[INFO] Docs-Änderungen erkannt — Neubau der DB...' -ForegroundColor Yellow
}

# ── Schritt 1: themis_docs_builder kompilieren ────────────────────────────────
if (-not $SkipBuild) {
    Write-Host ''
    Write-Host '[1/3] Kompiliere themis_docs_builder...' -ForegroundColor Yellow

    if (-not (Test-Path $ToolBuildDir)) {
        New-Item -ItemType Directory -Path $ToolBuildDir | Out-Null
    }

    Push-Location $ToolBuildDir
    try {
        # CMake konfigurieren
        $cmakeArgs = @(
            $ToolSrcDir,
            '-G', 'Ninja',
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_TESTS=OFF"
        )

        # vcpkg toolchain wenn vorhanden
        $vcpkgToolchain = Join-Path $RepoRoot 'vcpkg' 'scripts' 'buildsystems' 'vcpkg.cmake'
        if (Test-Path $vcpkgToolchain) {
            $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
        }

        Write-Host "  cmake $($cmakeArgs -join ' ')"
        cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake-Konfiguration fehlgeschlagen (Exit $LASTEXITCODE)" }

        Write-Host "  ninja"
        ninja
        if ($LASTEXITCODE -ne 0) { throw "Compile-Fehler (Exit $LASTEXITCODE)" }

        Write-Host '  [OK] themis_docs_builder kompiliert.' -ForegroundColor Green
    } finally {
        Pop-Location
    }
} else {
    Write-Host '[1/3] Compile übersprungen (-SkipBuild).' -ForegroundColor Gray
}

# ── Schritt 2: Ausgabeverzeichnis vorbereiten ─────────────────────────────────
Write-Host ''
Write-Host '[2/3] Ausgabeverzeichnis vorbereiten...' -ForegroundColor Yellow

if (Test-Path $OutputDir) {
    if ($Force) {
        Write-Host "  Entferne alte DB: $OutputDir"
        Remove-Item $OutputDir -Recurse -Force
    }
}
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# ── Schritt 3: DB bauen ──────────────────────────────────────────────────────
Write-Host ''
Write-Host '[3/3] Baue Docs-DB...' -ForegroundColor Yellow

if (Test-Path $ToolExe) {
    # C++-Tool verwenden wenn kompiliert
    Write-Host "  Verwende kompiliertes Tool: $ToolExe"
    $toolArgs = @(
        '--input',    $DocsDir,
        '--output',   $OutputDir,
        '--format',   'markdown',
        '--recursive',
        '--namespace', 'docs',
        '--validate'
    )
    & $ToolExe @toolArgs
    if ($LASTEXITCODE -ne 0) { throw "themis_docs_builder fehlgeschlagen (Exit $LASTEXITCODE)" }
} else {
    # Python-Fallback: generate_docs_rocksdb.py
    Write-Host '  [FALLBACK] C++-Tool nicht gefunden — verwende Python-Generator...' -ForegroundColor Yellow

    $pyScript = Join-Path $RepoRoot 'scripts' 'generate_docs_rocksdb.py'
    if (-not (Test-Path $pyScript)) {
        Write-Error "Weder C++-Tool noch Python-Fallback gefunden.`nErwartet: $ToolExe`noder:      $pyScript"
        exit 1
    }

    # Intermediäre JSON-Datei für das Docs-Artifact
    $jsonOut = Join-Path $OutputDir 'docs_artifact.json'

    # Schritt A: docs/ zu JSON indexieren
    $indexScript = Join-Path $RepoRoot 'scripts' 'generate_docs_database.py'
    if (Test-Path $indexScript) {
        Write-Host "  python $indexScript --repo-markdown --output $jsonOut"
        python $indexScript --repo-markdown --output $jsonOut
        if ($LASTEXITCODE -ne 0) { throw "generate_docs_database.py fehlgeschlagen" }

        # Legacy compatibility for components that still look for docs_database.json
        $legacyOut = Join-Path $OutputDir 'docs_database.json'
        Copy-Item -Path $jsonOut -Destination $legacyOut -Force
    } else {
        # Minimaler JSON-Index direkt erstellen
        Write-Host '  Erstelle minimalen Docs-Index...'
        $docs = Get-ChildItem $DocsDir -Recurse -Include '*.md','*.txt','*.html' -ErrorAction SilentlyContinue |
                Select-Object -First 5000
        $index = @{ documents = @() }
        foreach ($f in $docs) {
            $rel  = $f.FullName.Replace($DocsDir, '').TrimStart('\', '/')
            $content = ''
            try { $content = (Get-Content $f.FullName -Raw -Encoding UTF8 -ErrorAction SilentlyContinue) ?? '' } catch {}
            $index.documents += @{
                file_path = $rel
                metadata  = @{ file_name = $f.Name; extension = $f.Extension }
                content   = $content.Substring(0, [Math]::Min(2000, $content.Length))
            }
        }
        $index | ConvertTo-Json -Depth 5 | Set-Content $jsonOut -Encoding UTF8
        Write-Host "  [OK] $($index.documents.Count) Dokumente indexiert."
    }

    # Schritt B: JSON -> RocksDB-Import-Script generieren
    Write-Host "  python $pyScript --input $jsonOut --output $OutputDir"
    python $pyScript --input $jsonOut --output $OutputDir --method cpp
    if ($LASTEXITCODE -ne 0) { throw "generate_docs_rocksdb.py fehlgeschlagen" }
}

# ── STAMP-Datei schreiben ────────────────────────────────────────────────────
$stampContent = @"
ThemisDB Docs-DB Build Stamp
Generated : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
DocsDir   : $DocsDir
OutputDir : $OutputDir
Host      : $env:COMPUTERNAME
"@
Set-Content -Path $StampFile -Value $stampContent -Encoding UTF8

# ── Zusammenfassung ──────────────────────────────────────────────────────────
Write-Host ''
Write-Host '═══════════════════════════════════════════════════════════' -ForegroundColor Green
Write-Host '  Docs-DB erfolgreich erstellt!' -ForegroundColor Green
Write-Host '═══════════════════════════════════════════════════════════' -ForegroundColor Green
$dbSize = (Get-ChildItem $OutputDir -Recurse -File | Measure-Object -Property Length -Sum).Sum
Write-Host "  Pfad    : $OutputDir"
Write-Host ("  Größe   : {0:N1} MB" -f ($dbSize / 1MB))
Write-Host ''
Write-Host '  Nächster Schritt: Docker-Build starten' -ForegroundColor Cyan
Write-Host "    docker buildx bake -f docker-bake.hcl themisdb-community"
Write-Host ''
