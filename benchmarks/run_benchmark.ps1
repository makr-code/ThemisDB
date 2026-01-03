#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Multi-Shard RAID Benchmark Runner für Windows PowerShell
.DESCRIPTION
    Orchestriert ThemisDB Multi-Shard-Cluster mit verschiedenen RAID-Konfigurationen
.PARAMETER Scenario
    Benchmark-Szenario (S1-S8)
.PARAMETER RaidLevel
    RAID-Konfiguration (RAID0, RAID1, RAID5, RAID6, RAID10)
.PARAMETER NumShards
    Anzahl der Shards (3, 6, 12, 24)
.PARAMETER DurationHours
    Test-Dauer in Stunden
.EXAMPLE
    .\run_benchmark.ps1 -Scenario S4 -RaidLevel RAID10 -NumShards 6 -DurationHours 12
#>

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('S1', 'S2', 'S3', 'S4', 'S5', 'S6', 'S7', 'S8')]
    [string]$Scenario,
    
    [Parameter(Mandatory = $true)]
    [ValidateSet('RAID0', 'RAID1', 'RAID5', 'RAID6', 'RAID10')]
    [string]$RaidLevel,
    
    [Parameter(Mandatory = $true)]
    [ValidateSet(3, 6, 12, 24)]
    [int]$NumShards = 6,
    
    [Parameter(Mandatory = $false)]
    [int]$DurationHours = 12
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Logging
function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] [$Level] $Message"
}

# Szenario-Konfigurationen
$scenarios = @{
    S1 = @{
        name = "Baseline Performance RAID0"
        shards = 3
        raid = "RAID0"
        workload = "OLTP"
        documents = 100GB
        duration = 4
        targetQps = 10000
    }
    S2 = @{
        name = "High Availability RAID1"
        shards = 3
        raid = "RAID1"
        workload = "OLTP"
        documents = 100GB
        duration = 6
        targetQps = 8000
    }
    S3 = @{
        name = "Balanced RAID5"
        shards = 3
        raid = "RAID5"
        workload = "OLTP"
        documents = 100GB
        duration = 8
        targetQps = 9000
    }
    S4 = @{
        name = "Production Standard RAID10"
        shards = 6
        raid = "RAID10"
        workload = "Mixed"
        documents = 500GB
        duration = 12
        targetQps = 50000
    }
    S5 = @{
        name = "Data Warehouse RAID6"
        shards = 12
        raid = "RAID6"
        workload = "OLAP"
        documents = 1TB
        duration = 18
        targetQps = 500
    }
    S6 = @{
        name = "Time-Series RAID10"
        shards = 24
        raid = "RAID10"
        workload = "TimeSeries"
        documents = 1TB
        duration = 24
        targetQps = 20000
    }
    S7 = @{
        name = "Vector Search RAID5"
        shards = 6
        raid = "RAID5"
        workload = "VectorSearch"
        documents = 500GB
        duration = 10
        targetQps = 1000
    }
    S8 = @{
        name = "Multi-DC Failover RAID1"
        shards = 12
        raid = "RAID1"
        workload = "Mixed"
        documents = 500GB
        duration = 16
        targetQps = 30000
    }
}

# Umgebungsvariablen setzen
$env:SCENARIO = $Scenario
$env:RAID_LEVEL = $RaidLevel
$env:NUM_SHARDS = $NumShards
$env:DURATION_HOURS = $DurationHours

$config = $scenarios[$Scenario]
Write-Log "Starte Szenario $Scenario $($config.name)"
Write-Log "RAID-Level $RaidLevel"
Write-Log "Shards $NumShards"
Write-Log "Workload $($config.workload)"
Write-Log "Dauer $DurationHours Stunden"

# Pre-Checks
Write-Log "Führe Pre-Checks durch..."

# Prüfe Docker-Installation
$docker = Get-Command docker -ErrorAction SilentlyContinue
if (-not $docker) {
    Write-Log "Docker nicht gefunden. Bitte Docker installieren." "ERROR"
    exit 1
}
Write-Log "Docker gefunden: $(docker --version)"

# Prüfe Docker Compose
$compose = Get-Command docker-compose -ErrorAction SilentlyContinue
if (-not $compose) {
    Write-Log "Docker Compose nicht gefunden. Bitte Docker Compose installieren." "ERROR"
    exit 1
}
Write-Log "Docker Compose gefunden: $(docker-compose --version)"

# Prüfe Python
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    Write-Log "Python nicht gefunden. Bitte Python installieren." "ERROR"
    exit 1
}
Write-Log "Python gefunden: $(python --version)"

# Verzeichnis-Struktur erstellen
Write-Log "Erstelle Verzeichnisse..."
$resultDir = "results"
$logsDir = "logs"
$dataDir = "data"

New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
New-Item -ItemType Directory -Path $logsDir -Force | Out-Null
New-Item -ItemType Directory -Path $dataDir -Force | Out-Null

# Timestamp für diesen Run
$runId = Get-Date -Format "yyyyMMdd_HHmmss"
$runDir = "results/${Scenario}_${RaidLevel}_${NumShards}shards_${runId}"
New-Item -ItemType Directory -Path $runDir -Force | Out-Null

Write-Log "Run-Verzeichnis $runDir"

# Docker Compose Profile auswählen
$profile = "$NumShards-shards"
Write-Log "Docker Compose Profile: $profile"

# Cleanup-Funktion
function Cleanup {
    Write-Log "Räume auf..." "INFO"
    
    try {
        Write-Log "Stoppe Docker Compose..."
        docker-compose -f docker-compose.multi-shard-raid.yml --profile $profile down -v 2>&1 | Out-Null
        Write-Log "Docker Compose gestoppt"
    }
    catch {
        Write-Log "Fehler beim Stoppen von Docker Compose: $_" "WARN"
    }
}

# Cleanup bei Exit registrieren
trap {
    Write-Log "Fehler aufgetreten: $_" "ERROR"
    Cleanup
    exit 1
}

# Starten Sie den Cluster
Write-Log "Starte ThemisDB Cluster mit $NumShards Shards..."
$composeCmd = "docker-compose -f docker-compose.multi-shard-raid.yml --profile $profile up -d"
Write-Log "Führe aus: $composeCmd"

Invoke-Expression $composeCmd
if ($LASTEXITCODE -ne 0) {
    Write-Log "Docker Compose Start fehlgeschlagen" "ERROR"
    Cleanup
    exit 1
}

Write-Log "Cluster gestartet. Warte auf Startup..."
Start-Sleep -Seconds 30

# Health-Check
Write-Log "Führe Health-Checks durch..."
$maxRetries = 30
$retryCount = 0
$healthyShards = 0

while ($retryCount -lt $maxRetries -and $healthyShards -lt $NumShards) {
    $retryCount++
    $healthyShards = 0
    
    for ($i = 0; $i -lt $NumShards; $i++) {
        try {
            $port = 8080 + $i
            $response = Invoke-WebRequest -Uri "http://localhost:$port/health" -TimeoutSec 5 -ErrorAction SilentlyContinue
            if ($response.StatusCode -eq 200) {
                $healthyShards++
            }
        }
        catch {
            # Shard nicht healthy
        }
    }
    
    if ($healthyShards -lt $NumShards) {
        Write-Log "Health-Check: $healthyShards/$NumShards Shards ready. Warte..."
        Start-Sleep -Seconds 10
    }
}

if ($healthyShards -lt $NumShards) {
    Write-Log "Nur $healthyShards/$NumShards Shards sind nach $maxRetries Versuchen ready" "WARN"
}

Write-Log "Health-Check abgeschlossen: $healthyShards/$NumShards Shards ready"

# Berechne Dokumentanzahl basierend auf Datengröße
$documentsTotal = switch ($config.documents) {
    "100GB" { 5000000 }      # 5M documents ≈ 100GB
    "500GB" { 25000000 }     # 25M documents ≈ 500GB
    "1TB"   { 50000000 }     # 50M documents ≈ 1TB
    default { 25000000 }
}

Write-Log "Lade Test-Daten: $($config.documents) ($documentsTotal Dokumente)..."
$dataLoaderCmd = "docker-compose -f docker-compose.multi-shard-raid.yml --profile data-load up data-loader --abort-on-container-exit"
Write-Log "Führe aus: $dataLoaderCmd"

Invoke-Expression $dataLoaderCmd
if ($LASTEXITCODE -ne 0) {
    Write-Log "Fehler beim Daten-Laden" "WARN"
}

# Starte Benchmark
Write-Log "Starte Benchmark mit $DurationHours Stunden Dauer..."
Write-Log "Target QPS: $($config.targetQps)"

$pythonScript = "run_multi_shard_raid_benchmark.py"
$pythonCmd = "python $pythonScript --scenario $Scenario --shards $NumShards --raid $RaidLevel --workload $($config.workload) --duration $DurationHours --target-qps $($config.targetQps)"

Write-Log "Führe aus: $pythonCmd"

$env:SCENARIO = $Scenario
$env:NUM_SHARDS = $NumShards
$env:RAID_LEVEL = $RaidLevel
$env:WORKLOAD_TYPE = $config.workload
$env:DURATION_SECONDS = $DurationHours * 3600
$env:TARGET_QPS = $config.targetQps

Invoke-Expression $pythonCmd

# Benchmark-Ergebnisse verarbeiten
Write-Log "Benchmark abgeschlossen. Verarbeite Ergebnisse..."

# Verschiebe Ergebnisse
if (Test-Path "benchmark_result.json") {
    Copy-Item "benchmark_result.json" "$runDir/result.json"
    Write-Log "Ergebnis gespeichert: $runDir/result.json"
}

# Sammle Logs
Write-Log "Sammle Logs..."
$logsPattern = "logs/**/*.log"
Get-ChildItem -Path $logsPattern -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName "$runDir/" -ErrorAction SilentlyContinue
}

# Ergebnisse anzeigen
Write-Log "=== Benchmark-Zusammenfassung ==="
if (Test-Path "$runDir/result.json") {
    $result = Get-Content "$runDir/result.json" | ConvertFrom-Json
    Write-Log "Szenario: $($result.scenario)"
    Write-Log "Shards: $($result.shard_count)"
    Write-Log "RAID: $($result.raid_level)"
    Write-Log "Workload: $($result.workload_type)"
    Write-Log "Durchsatz: $([math]::Round($result.throughput_qps, 2)) QPS"
    Write-Log "Latenz P99: $([math]::Round($result.latency_p99_ms, 2)) ms"
    Write-Log "Erfolgreiche Queries: $($result.successful_queries)"
    Write-Log "Fehlgeschlagene Queries: $($result.failed_queries)"
    
    # Speichere Zusammenfassung
    $summary = @{
        timestamp = Get-Date -Format "o"
        scenario = $result.scenario
        shard_count = $result.shard_count
        raid_level = $result.raid_level
        workload_type = $result.workload_type
        throughput_qps = $result.throughput_qps
        latency_p99_ms = $result.latency_p99_ms
        success_rate = ($result.successful_queries / ($result.successful_queries + $result.failed_queries) * 100)
        run_dir = $runDir
    }
    
    $summary | ConvertTo-Json | Out-File "$runDir/summary.json"
    Write-Log "Zusammenfassung gespeichert: $runDir/summary.json"
}

# Cleanup
Write-Log "Stoppe Cluster..."
Cleanup

Write-Log "Benchmark abgeschlossen!"
Write-Log "Ergebnisse verfügbar in: $runDir"
Write-Log "Nächste Schritte:"
Write-Log "  - Grafana öffnen: http://localhost:3000"
Write-Log "  - Ergebnisse anzeigen: Get-Content $runDir/summary.json | ConvertFrom-Json"
Write-Log "  - Weitere Tests: .\run_benchmark.ps1 -Scenario S5 -RaidLevel RAID6 -NumShards 12"

exit 0
