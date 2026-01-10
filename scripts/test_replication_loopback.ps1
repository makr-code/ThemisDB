# Replication Loopback Test Script
# Tests WALShipper -> HTTP Apply flow with MAJORITY write concern
#
# Setup:
# - Primary on port 8765 with shipper enabled
# - Replica on port 8766 with apply endpoint
# - Write to primary with ?write_concern=MAJORITY
# - Verify entry applied on replica

param(
    [int]$Timeout = 30,
    [string]$BuildDir = "C:\VCC\themis\build-msvc\Release",
    [string]$TestDataDir = "C:\VCC\themis\test_replication_loopback_data"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Replication Loopback Test ===" -ForegroundColor Cyan
Write-Host "Build directory: $BuildDir"
Write-Host "Test data directory: $TestDataDir"
Write-Host ""

# Clean up previous test data
if (Test-Path $TestDataDir) {
    Write-Host "Cleaning up previous test data..." -ForegroundColor Yellow
    Remove-Item -Path $TestDataDir -Recurse -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

# Create directories
$primaryDataDir = Join-Path $TestDataDir "primary"
$replicaDataDir = Join-Path $TestDataDir "replica"
$configDir = Join-Path $TestDataDir "config"

New-Item -ItemType Directory -Path $primaryDataDir -Force | Out-Null
New-Item -ItemType Directory -Path $replicaDataDir -Force | Out-Null
New-Item -ItemType Directory -Path $configDir -Force | Out-Null

Write-Host "Created test directories" -ForegroundColor Green

# Create replica config (no shipper, just applier)
$replicaConfig = @"
server:
  port: 8766
  threads: 2
  host: "127.0.0.1"

storage:
  db_path: "$($replicaDataDir -replace '\\', '/')"
  memtable_size_mb: 64
  block_cache_size_mb: 128
  enable_wal: true
  enable_blobdb: false

replication:
  shipper_enabled: false
"@

$replicaConfigPath = Join-Path $configDir "replica.yaml"
$replicaConfig | Out-File -FilePath $replicaConfigPath -Encoding utf8
Write-Host "Created replica config: $replicaConfigPath" -ForegroundColor Green

# Create primary config (with shipper pointing to replica)
$primaryConfig = @"
server:
  port: 8765
  threads: 2
  host: "127.0.0.1"

storage:
  db_path: "$($primaryDataDir -replace '\\', '/')"
  memtable_size_mb: 64
  block_cache_size_mb: 128
  enable_wal: true
  enable_blobdb: false

replication:
  shipper_enabled: true
  primary_id: "primary-8765"
  batch_size: 100
  max_batch_bytes: 1048576
  ship_interval_ms: 100
  compression: "zstd"
  compression_level: 3
  replicas:
    - replica_id: "replica-8766"
      endpoint: "http://127.0.0.1:8766"
"@

$primaryConfigPath = Join-Path $configDir "primary.yaml"
$primaryConfig | Out-File -FilePath $primaryConfigPath -Encoding utf8
Write-Host "Created primary config: $primaryConfigPath" -ForegroundColor Green

# Check if themis_server.exe exists
$serverExe = Join-Path $BuildDir "themis_server.exe"
if (-not (Test-Path $serverExe)) {
    Write-Host "ERROR: themis_server.exe not found at $serverExe" -ForegroundColor Red
    Write-Host "Please build the project first with Release configuration" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "Starting replica server on port 8766..." -ForegroundColor Cyan
$replicaJob = Start-Job -ScriptBlock {
    param($exe, $cfg)
    & $exe --config $cfg 2>&1
} -ArgumentList $serverExe, $replicaConfigPath

Start-Sleep -Seconds 3

# Check if replica is running
$replicaHealthUrl = "http://127.0.0.1:8766/health"
$replicaReady = $false
for ($i = 0; $i -lt 10; $i++) {
    try {
        $response = Invoke-RestMethod -Uri $replicaHealthUrl -Method Get -TimeoutSec 2 -ErrorAction Stop
        if ($response.status -eq "healthy") {
            Write-Host "Replica server is ready" -ForegroundColor Green
            $replicaReady = $true
            break
        }
    } catch {
        Write-Host "Waiting for replica to start... ($($i+1)/10)" -ForegroundColor Yellow
        Start-Sleep -Seconds 1
    }
}

if (-not $replicaReady) {
    Write-Host "ERROR: Replica server failed to start" -ForegroundColor Red
    Stop-Job -Job $replicaJob -ErrorAction SilentlyContinue
    Remove-Job -Job $replicaJob -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host ""
Write-Host "Starting primary server on port 8765..." -ForegroundColor Cyan
$primaryJob = Start-Job -ScriptBlock {
    param($exe, $cfg)
    & $exe --config $cfg 2>&1
} -ArgumentList $serverExe, $primaryConfigPath

Start-Sleep -Seconds 3

# Check if primary is running
$primaryHealthUrl = "http://127.0.0.1:8765/health"
$primaryReady = $false
for ($i = 0; $i -lt 10; $i++) {
    try {
        $response = Invoke-RestMethod -Uri $primaryHealthUrl -Method Get -TimeoutSec 2 -ErrorAction Stop
        if ($response.status -eq "healthy") {
            Write-Host "Primary server is ready" -ForegroundColor Green
            $primaryReady = $true
            break
        }
    } catch {
        Write-Host "Waiting for primary to start... ($($i+1)/10)" -ForegroundColor Yellow
        Start-Sleep -Seconds 1
    }
}

if (-not $primaryReady) {
    Write-Host "ERROR: Primary server failed to start" -ForegroundColor Red
    Write-Host ""
    Write-Host "=== Primary Server Output ===" -ForegroundColor Yellow
    $primaryOutput = Receive-Job -Job $primaryJob -ErrorAction SilentlyContinue
    if ($primaryOutput) {
        $primaryOutput | Select-Object -Last 50 | ForEach-Object { Write-Host $_ }
    } else {
        Write-Host "(No output captured)" -ForegroundColor Gray
    }
    
    Stop-Job -Job $primaryJob -ErrorAction SilentlyContinue
    Remove-Job -Job $primaryJob -Force -ErrorAction SilentlyContinue
    Stop-Job -Job $replicaJob -ErrorAction SilentlyContinue
    Remove-Job -Job $replicaJob -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host ""
Write-Host "=== Test Phase: Writing to Primary with MAJORITY ===" -ForegroundColor Cyan

$testKey = "test:replication:$(Get-Random)"
$testValue = @{
    message = "Replication loopback test"
    timestamp = (Get-Date).ToString("o")
    test_id = [guid]::NewGuid().ToString()
} | ConvertTo-Json

$writeUrl = "http://127.0.0.1:8765/entities/${testKey}?write_concern=MAJORITY"

Write-Host "Writing to primary: $testKey" -ForegroundColor Yellow
Write-Host "Write concern: MAJORITY" -ForegroundColor Yellow

try {
    $writeResponse = Invoke-RestMethod -Uri $writeUrl -Method Put -Body $testValue -ContentType "application/json" -TimeoutSec 10 -ErrorAction Stop
    Write-Host "Write succeeded" -ForegroundColor Green
    Write-Host "Response: $($writeResponse | ConvertTo-Json -Compress)" -ForegroundColor Gray
} catch {
    Write-Host "ERROR: Write failed - $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "This could indicate quorum timeout or replication failure" -ForegroundColor Yellow
    
    # Cleanup
    Stop-Job -Job $primaryJob -ErrorAction SilentlyContinue
    Remove-Job -Job $primaryJob -Force -ErrorAction SilentlyContinue
    Stop-Job -Job $replicaJob -ErrorAction SilentlyContinue
    Remove-Job -Job $replicaJob -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host ""
Write-Host "Waiting for replication to complete..." -ForegroundColor Yellow
Start-Sleep -Seconds 2

Write-Host ""
Write-Host "=== Verification: Reading from Replica ===" -ForegroundColor Cyan

$readUrl = "http://127.0.0.1:8766/entities/${testKey}"
$verificationSuccess = $false

for ($i = 0; $i -lt 5; $i++) {
    try {
        Write-Host "Attempting to read from replica... ($(($i+1))/5)" -ForegroundColor Yellow
        $replicaResponse = Invoke-RestMethod -Uri $readUrl -Method Get -TimeoutSec 5 -ErrorAction Stop
        
        if ($replicaResponse) {
            Write-Host "SUCCESS: Entry found on replica!" -ForegroundColor Green
            Write-Host "Replica data: $($replicaResponse | ConvertTo-Json -Compress)" -ForegroundColor Gray
            $verificationSuccess = $true
            break
        }
    } catch {
        if ($_.Exception.Response.StatusCode.value__ -eq 404) {
            Write-Host "Entry not yet replicated (404), waiting..." -ForegroundColor Yellow
            Start-Sleep -Seconds 1
        } else {
            Write-Host "ERROR reading from replica: $($_.Exception.Message)" -ForegroundColor Red
            break
        }
    }
}

Write-Host ""
Write-Host "=== Metrics Check ===" -ForegroundColor Cyan

# Check primary metrics
try {
    $primaryMetrics = Invoke-WebRequest -Uri "http://127.0.0.1:8765/metrics" -Method Get -UseBasicParsing -TimeoutSec 5
    Write-Host "Primary metrics retrieved (${$primaryMetrics.Content.Length} bytes)" -ForegroundColor Green
    
    # Look for ship metrics
    $shipBatches = ($primaryMetrics.Content -split "`n" | Where-Object { $_ -match "themis_wal_ship_batches_total" })
    if ($shipBatches) {
        Write-Host "Ship metrics:" -ForegroundColor Cyan
        $shipBatches | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
} catch {
    Write-Host "Warning: Could not retrieve primary metrics" -ForegroundColor Yellow
}

# Check replica metrics
try {
    $replicaMetrics = Invoke-WebRequest -Uri "http://127.0.0.1:8766/metrics" -Method Get -UseBasicParsing -TimeoutSec 5
    Write-Host "Replica metrics retrieved (${$replicaMetrics.Content.Length} bytes)" -ForegroundColor Green
    
    # Look for apply metrics
    $applyBatches = ($replicaMetrics.Content -split "`n" | Where-Object { $_ -match "themis_wal_apply" })
    if ($applyBatches) {
        Write-Host "Apply metrics:" -ForegroundColor Cyan
        $applyBatches | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
} catch {
    Write-Host "Warning: Could not retrieve replica metrics" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Cleanup ===" -ForegroundColor Cyan

# Stop servers
Write-Host "Stopping primary server..." -ForegroundColor Yellow
Stop-Job -Job $primaryJob -ErrorAction SilentlyContinue
Remove-Job -Job $primaryJob -Force -ErrorAction SilentlyContinue

Write-Host "Stopping replica server..." -ForegroundColor Yellow
Stop-Job -Job $replicaJob -ErrorAction SilentlyContinue
Remove-Job -Job $replicaJob -Force -ErrorAction SilentlyContinue

Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan

if ($verificationSuccess) {
    Write-Host "PASS: Replication loopback test succeeded" -ForegroundColor Green
    Write-Host "  - Write with MAJORITY concern completed" -ForegroundColor Green
    Write-Host "  - Entry successfully replicated to replica" -ForegroundColor Green
    Write-Host "  - Replica applied WAL entry and served read" -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAIL: Replication loopback test failed" -ForegroundColor Red
    Write-Host "  - Entry was not found on replica after timeout" -ForegroundColor Red
    Write-Host "  - Check server logs for WAL ship/apply errors" -ForegroundColor Yellow
    exit 1
}
