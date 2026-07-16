#!/usr/bin/env pwsh
# Production/Dev Package Builder für ThemisDB v1.3.0
param(
    [string]$Version = "1.3.0",
    [switch]$Dev,
    [switch]$IncludeTests,
    [switch]$IncludeBenchmarks,
    [switch]$PrepareMiniLlm,
    [string]$PythonExecutable = "python",
    [string]$MiniLlmSourceFile = ""
)

$ErrorActionPreference = 'Stop'
$repo = "C:\VCC\themis"
$buildDir = Join-Path $repo "build-msvc\Release"
$releaseDir = Join-Path $repo "release"
$suffix = if ($Dev) { "dev" } else { "production" }
$stage = Join-Path $releaseDir "themisdb-$Version-$suffix"

# Default: Dev-Paket enthält Tests/Benchmarks, Prod nicht
if (-not $PSBoundParameters.ContainsKey('IncludeTests')) { $IncludeTests = $Dev }
if (-not $PSBoundParameters.ContainsKey('IncludeBenchmarks')) { $IncludeBenchmarks = $Dev }

Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  ThemisDB v$Version - $suffix Package Builder" -ForegroundColor Cyan
Write-Host "  'ThemisDB keeps his own llamas'" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan

# Clean and create directory structure
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
$dirs = @(
    "bin",
    "config", "config/processors", "config/schemas",
    "conf",  # alias/samples
    "data", "data/db", "data/wal", "data/backups", "data/temp",
    "docs", "docs/api", "docs/guides", "docs/architecture",
    "examples", "examples/api", "examples/integrations",
    "logs",
    "models",
    "plugins",
    "certificates",
    "import",
    "scripts", "scripts/admin", "scripts/maintenance", "scripts/monitoring",
    "tests", "tests/integration", "tests/performance",
    "tools", "tools/cli", "tools/migration"
)

foreach ($dir in $dirs) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage $dir) | Out-Null
}

# Copy Binaries
Write-Host "`n[1/6] Kopiere Binaries..." -ForegroundColor Yellow
$files = @(
    "themis_server.exe",
    "themis_demo.exe",
    "themis_demo_encryption.exe"
)
foreach ($file in $files) {
    $src = Join-Path $buildDir $file
    if (Test-Path $src) {
        Copy-Item $src -Destination "$stage\bin\" -Force
        Write-Host "  + $file" -ForegroundColor Green
    }
}

# Copy DLLs
Get-ChildItem "$buildDir\*.dll" | ForEach-Object {
    Copy-Item $_.FullName -Destination "$stage\bin\" -Force
}
Write-Host "  + $(((Get-ChildItem "$stage\bin\*.dll").Count)) DLLs" -ForegroundColor Green

# Optional: Tests & Benchmarks
if ($IncludeTests) {
    $tests = Get-ChildItem "$buildDir\*test*.exe" -ErrorAction SilentlyContinue
    foreach ($t in $tests) {
        Copy-Item $t.FullName -Destination "$stage\tests\" -Force
        Write-Host "  + $($t.Name)" -ForegroundColor Green
    }
}

if ($IncludeBenchmarks) {
    $benches = Get-ChildItem "$buildDir\bench_*.exe" -ErrorAction SilentlyContinue
    foreach ($b in $benches) {
        Copy-Item $b.FullName -Destination "$stage\tests\performance\" -Force
        Write-Host "  + $($b.Name)" -ForegroundColor Green
    }
}

# Create Config Files
Write-Host "`n[2/6] Erstelle Konfigurationen..." -ForegroundColor Yellow
$config = @{
    server = @{
        host = "0.0.0.0"
        port = 8765
        threads = 4
        max_connections = 1000
    }
    database = @{
        path = "./data/db"
        wal_path = "./data/wal"
        backup_path = "./data/backups"
    }
    logging = @{
        level = "info"
        file = "./logs/themis.log"
        max_size_mb = 100
        max_files = 10
    }
    security = @{
        enable_tls = $false
        cert_file = ""
        key_file = ""
    }
}
$config | ConvertTo-Json -Depth 10 | Out-File "$stage\config\config.json" -Encoding UTF8

$llmConfig = @'
llm:
  enabled: true
  plugin: llamacpp
  model:
        path: ./models/default.gguf
    n_gpu_layers: 32
    n_ctx: 4096
    n_batch: 512
'@
$llmConfig | Out-File "$stage\config\llm_config.yaml" -Encoding UTF8
Write-Host "  + config.json, llm_config.yaml" -ForegroundColor Green

# Mirror config as samples in conf/ (align with Neo4j/Postgres style)
Copy-Item "$stage\config\config.json" -Destination "$stage\conf\config.sample.json" -Force
Copy-Item "$stage\config\llm_config.yaml" -Destination "$stage\conf\llm_config.sample.yaml" -Force

# Create Admin Tools
Write-Host "`n[3/6] Erstelle Admin-Tools..." -ForegroundColor Yellow
$backupScript = @'
#!/usr/bin/env pwsh
param([string]$BackupPath = "./data/backups/backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')")
Write-Host "Creating backup to: $BackupPath" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $BackupPath | Out-Null
Copy-Item ./data/db/* -Destination $BackupPath -Recurse -Force
Write-Host "Backup completed!" -ForegroundColor Green
'@
$backupScript | Out-File "$stage\scripts\admin\backup.ps1" -Encoding UTF8

$restoreScript = @'
#!/usr/bin/env pwsh
param([Parameter(Mandatory)][string]$BackupPath)
Write-Host "Restoring from: $BackupPath" -ForegroundColor Yellow
Copy-Item $BackupPath\* -Destination ./data/db -Recurse -Force
Write-Host "Restore completed!" -ForegroundColor Green
'@
$restoreScript | Out-File "$stage\scripts\admin\restore.ps1" -Encoding UTF8

$healthScript = @'
#!/usr/bin/env pwsh
$response = Invoke-WebRequest -Uri http://localhost:8765/health -UseBasicParsing
if ($response.StatusCode -eq 200) {
    Write-Host "ThemisDB is healthy" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Health check failed" -ForegroundColor Red
    exit 1
}
'@
$healthScript | Out-File "$stage\scripts\monitoring\health-check.ps1" -Encoding UTF8
Write-Host "  + backup.ps1, restore.ps1, health-check.ps1" -ForegroundColor Green

# Copy optional docs database and mini model bundle
Write-Host "`n[3b/6] Integriere docs.db und Mini-LLM Assets..." -ForegroundColor Yellow

$docsCandidates = @(
    (Join-Path $repo "build-msvc\data\docs.db"),
    (Join-Path $repo "data\docs.db")
)

$docsJsonCandidates = @(
    (Join-Path $repo "build-msvc\data\docs_database.json"),
    (Join-Path $repo "data\docs_database.json")
)

foreach ($candidate in $docsCandidates | Select-Object -Unique) {
    if (Test-Path $candidate) {
        Copy-Item $candidate -Destination "$stage\data\docs.db" -Force
        Write-Host "  + docs.db" -ForegroundColor Green
        break
    }
}

foreach ($candidate in $docsJsonCandidates | Select-Object -Unique) {
    if (Test-Path $candidate) {
        Copy-Item $candidate -Destination "$stage\data\docs_database.json" -Force
        Write-Host "  + docs_database.json" -ForegroundColor Green
        break
    }
}

if ($PrepareMiniLlm) {
    $miniLlmScript = Join-Path $repo "scripts\prepare_release_mini_llm.py"
    if (Test-Path $miniLlmScript) {
        $helperArgs = @($miniLlmScript, "--output-dir", "$stage\models")
        if ($MiniLlmSourceFile) {
            $helperArgs += @("--source-file", $MiniLlmSourceFile)
        }

        try {
            & $PythonExecutable @helperArgs
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  + Mini-LLM Bundle erstellt" -ForegroundColor Green
            } else {
                Write-Host "  - Mini-LLM Bundle fehlgeschlagen" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "  - Mini-LLM Bundle fehlgeschlagen: $_" -ForegroundColor Yellow
        }
    }
} else {
    foreach ($candidate in @("default.gguf", "mini-llm.manifest.json")) {
        $src = Join-Path $repo "models\$candidate"
        if (Test-Path $src) {
            Copy-Item $src -Destination "$stage\models\" -Force
            Write-Host "  + $candidate" -ForegroundColor Green
        }
    }
}

# Copy Documentation
Write-Host "`n[4/6] Kopiere Dokumentation..." -ForegroundColor Yellow
$docFiles = @("README.md", "LICENSE", "CHANGELOG.md", "SECURITY.md")
foreach ($doc in $docFiles) {
    $src = Join-Path $repo $doc
    if (Test-Path $src) {
        Copy-Item $src -Destination "$stage\docs\" -Force
    }
}

if (Test-Path "$repo\openapi") {
    Copy-Item "$repo\openapi\*" -Destination "$stage\docs\api\" -Recurse -Force
}

$installGuide = @'
# ThemisDB v1.3.0 - Production Installation

## Quick Start

### Windows
```powershell
# Extract and start
Expand-Archive -Path themisdb-v1.3.0-windows-x64-$suffix.zip
cd themisdb-1.3.0-$suffix\bin
.\themis_server.exe --config ..\config\config.json
```

## Directory Structure
bin/       - Executables and DLLs
config/    - Configuration files
data/      - Database storage
docs/      - Documentation
logs/      - Log files
scripts/   - Admin scripts
tools/     - Utilities

## Admin Tools
.\scripts\admin\backup.ps1                    # Backup database
.\scripts\admin\restore.ps1 -BackupPath <dir> # Restore
.\scripts\monitoring\health-check.ps1         # Health check

## REST API
http://localhost:8765/health  - Health check
http://localhost:8765/metrics - Metrics

## Support
GitHub: https://github.com/makr-code/ThemisDB
'@
$installGuide | Out-File "$stage\INSTALL.md" -Encoding UTF8
Write-Host "  + Documentation copied" -ForegroundColor Green

# Copy Examples
Write-Host "`n[5/6] Kopiere Beispiele..." -ForegroundColor Yellow
if (Test-Path "$repo\examples") {
    Copy-Item "$repo\examples\*" -Destination "$stage\examples\" -Recurse -Force -ErrorAction SilentlyContinue
}

$apiExample = @'
# REST API Example
$body = @{ query = "FOR doc IN users RETURN doc" } | ConvertTo-Json
$response = Invoke-RestMethod -Uri "http://localhost:8765/api/query" -Method POST -Body $body -ContentType "application/json"
$response | ConvertTo-Json
'@
$apiExample | Out-File "$stage\examples\api\query-example.ps1" -Encoding UTF8
Write-Host "  + Examples copied" -ForegroundColor Green

# Add placeholder README files for key empty dirs
@"
This directory stores database certificates (TLS). Provide cert.pem and key.pem, or leave empty for HTTP.
"@ | Out-File "$stage\certificates\README.txt" -Encoding ASCII

@"
Place import files (CSV/JSON) here for bulk loading.
"@ | Out-File "$stage\import\README.txt" -Encoding ASCII

@"
Place custom plugins here. See docs for building and loading plugins.
"@ | Out-File "$stage\plugins\README.txt" -Encoding ASCII

@"
Place downloaded LLM models here (e.g., gguf). Configure path in config/llm_config.yaml.
"@ | Out-File "$stage\models\README.txt" -Encoding ASCII

# Create Package
Write-Host "`n[6/6] Erstelle ZIP Package..." -ForegroundColor Yellow
$outZip = Join-Path $releaseDir "themisdb-v$Version-windows-x64-$suffix.zip"
if (Test-Path $outZip) { Remove-Item $outZip -Force }

Compress-Archive -Path "$stage\*" -DestinationPath $outZip -CompressionLevel Optimal -Force

$size = (Get-Item $outZip).Length / 1MB
Write-Host "  + Package: $([math]::Round($size, 2)) MB" -ForegroundColor Green

# Checksum
$hash = (Get-FileHash -Algorithm SHA256 $outZip).Hash.ToLower()
$sumFile = if ($Dev) { "SHA256SUMS_dev.txt" } else { "SHA256SUMS_production.txt" }
"$hash  $(Split-Path -Leaf $outZip)" | Out-File (Join-Path $releaseDir $sumFile) -Encoding ASCII
Write-Host "  + SHA256: $($hash.Substring(0,16))..." -ForegroundColor DarkGray

# Cleanup
Remove-Item $stage -Recurse -Force

Write-Host "`n═══════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "  $suffix Package Ready: $outZip" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Green
