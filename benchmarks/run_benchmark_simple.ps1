#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Multi-Shard RAID Benchmark Runner - Vereinfachte Version
.DESCRIPTION
    Orchestriert ThemisDB Multi-Shard-Cluster mit verschiedenen RAID-Konfigurationen
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
    [int]$DurationHours = 1
)

$ErrorActionPreference = "Stop"

# Stelle sicher dass wir im Benchmark-Verzeichnis sind
$benchmarkDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $benchmarkDir

# Logging
function Log {
    param([string]$msg)
    $ts = Get-Date -Format "HH:mm:ss"
    Write-Host "[$ts] $msg"
}

# Szenario-Konfigurationen
$scenarios = @{
    S1 = @{ name = "Baseline"; shards = 3; raid = "RAID0"; workload = "OLTP"; docs = "100GB" }
    S2 = @{ name = "HA"; shards = 3; raid = "RAID1"; workload = "OLTP"; docs = "100GB" }
    S3 = @{ name = "Balanced"; shards = 3; raid = "RAID5"; workload = "OLTP"; docs = "100GB" }
    S4 = @{ name = "Production"; shards = 6; raid = "RAID10"; workload = "Mixed"; docs = "500GB" }
    S5 = @{ name = "DataWarehouse"; shards = 12; raid = "RAID6"; workload = "OLAP"; docs = "1TB" }
    S6 = @{ name = "TimeSeries"; shards = 24; raid = "RAID10"; workload = "TimeSeries"; docs = "1TB" }
    S7 = @{ name = "VectorSearch"; shards = 6; raid = "RAID5"; workload = "VectorSearch"; docs = "500GB" }
    S8 = @{ name = "MultiDC"; shards = 12; raid = "RAID1"; workload = "Mixed"; docs = "500GB" }
}

Log "Starte Multi-Shard RAID Benchmark"
Log "Szenario: $Scenario"
Log "Shards: $NumShards"
Log "RAID: $RaidLevel"
Log "Dauer: $DurationHours Stunden"

# Pre-Checks
Log "Überprüfe Voraussetzungen..."

# Docker
$docker = Get-Command docker -ErrorAction SilentlyContinue
if (-not $docker) {
    Log "FEHLER: Docker nicht gefunden"
    exit 1
}
Log "Docker OK"

# Python
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    Log "FEHLER: Python nicht gefunden"
    exit 1
}
Log "Python OK"

# Verzeichnisse
$runId = Get-Date -Format "yyyyMMdd_HHmmss"
$runDir = "results/${Scenario}_${RaidLevel}_${NumShards}shards_${runId}"
New-Item -ItemType Directory -Path $runDir -Force | Out-Null
Log "Run-Verzeichnis: $runDir"

# Profile
$profile = "$NumShards-shards"

# Startup
Log "Starte Cluster mit Profile '$profile'..."
try {
    & docker-compose -f docker-compose.multi-shard-raid.yml --profile $profile up -d 2>&1 | Out-Null
    Log "Cluster erfolgreich gestartet"
} catch {
    Log "Warnung beim Starten: $_"
}

Log "Warte 30 Sekunden auf Startup..."
Start-Sleep -Seconds 30

# Health Check
Log "Health Check..."
$healthy = 0
$maxRetries = 10

for ($i = 0; $i -lt $maxRetries; $i++) {
    for ($j = 0; $j -lt $NumShards; $j++) {
        $port = 8080 + $j
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:$port/health" -TimeoutSec 2 -ErrorAction SilentlyContinue
            if ($response.StatusCode -eq 200) {
                $healthy++
            }
        }
        catch { }
    }
    
    if ($healthy -ge $NumShards) {
        break
    }
    
    Log "Health Check: $healthy/$NumShards Shards ready, retry..."
    Start-Sleep -Seconds 5
    $healthy = 0
}

Log "Cluster Status: $healthy/$NumShards Shards ready"

# Run Benchmark
$durationSec = $DurationHours * 3600
Log "Starte Benchmark für $DurationHours Stunden ($durationSec Sekunden)..."

$pythonCmd = "python run_multi_shard_raid_benchmark.py --scenario $Scenario --shards $NumShards --raid $RaidLevel --workload $($scenarios[$Scenario].workload) --duration $durationSec"
Log "Befehl: $pythonCmd"

$env:SCENARIO = $Scenario
$env:NUM_SHARDS = $NumShards
$env:RAID_LEVEL = $RaidLevel

try {
    Invoke-Expression $pythonCmd
    Log "Benchmark abgeschlossen"
}
catch {
    Log "Fehler beim Benchmark: $_"
}
finally {
    # Cleanup
    Log "Stoppe Cluster..."
    $cleanupOutput = docker-compose -f docker-compose.multi-shard-raid.yml --profile $profile down -v 2>&1
    Log "Cluster gestoppt"
}

Log "Fertig!"
Log "Ergebnisse in: $runDir"

# Zurück zum ursprünglichen Verzeichnis
Pop-Location

exit 0
