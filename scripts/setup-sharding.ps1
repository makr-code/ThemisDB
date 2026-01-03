# ThemisDB Sharding Setup Script
# Build Docker image and start sharded cluster

$ErrorActionPreference = "Stop"

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "ThemisDB Sharding Setup" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

# Check if themis_server.exe exists
$serverPath = "C:\VCC\themis\build-msvc\Release\themis_server.exe"
if (-not (Test-Path $serverPath)) {
    Write-Host "❌ themis_server.exe nicht gefunden in: $serverPath" -ForegroundColor Red
    Write-Host "Bitte zuerst ThemisDB bauen:" -ForegroundColor Yellow
    Write-Host "  cd C:\VCC\themis" -ForegroundColor Yellow
    Write-Host "  cmake --build build-msvc --config Release --target themis_server" -ForegroundColor Yellow
    exit 1
}

Write-Host "✓ themis_server.exe gefunden" -ForegroundColor Green

# Stop existing containers
Write-Host ""
Write-Host "🛑 Stoppe existierende Container..." -ForegroundColor Yellow
docker-compose -f C:\VCC\themis\docker\compose\docker-compose-sharding.yml down 2>$null

# Build Docker image
Write-Host ""
Write-Host "🔨 Baue Docker Image..." -ForegroundColor Yellow
Push-Location C:\VCC\themis
docker build -t themisdb:1.3.0 -f Dockerfile.themis-server .
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Docker Build fehlgeschlagen" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

Write-Host "✓ Docker Image gebaut: themisdb:1.3.0" -ForegroundColor Green

# Start sharded cluster
Write-Host ""
Write-Host "🚀 Starte Sharding Cluster..." -ForegroundColor Yellow
docker-compose -f C:\VCC\themis\docker\compose\docker-compose-sharding.yml up -d

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Cluster Start fehlgeschlagen" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==================================" -ForegroundColor Green
Write-Host "✓ Cluster gestartet!" -ForegroundColor Green
Write-Host "==================================" -ForegroundColor Green
Write-Host ""
Write-Host "Endpoints:" -ForegroundColor Cyan
Write-Host "  Coordinator (Main):  http://localhost:8765" -ForegroundColor White
Write-Host "  Shard 1:             http://localhost:8081" -ForegroundColor White
Write-Host "  Shard 2:             http://localhost:8082" -ForegroundColor White
Write-Host "  Shard 3:             http://localhost:8083" -ForegroundColor White
Write-Host "  Grafana:             http://localhost:3000" -ForegroundColor White
Write-Host ""
Write-Host "Status prüfen:" -ForegroundColor Cyan
Write-Host "  docker-compose -f C:\VCC\themis\docker\compose\docker-compose-sharding.yml ps" -ForegroundColor Gray
Write-Host ""
Write-Host "Logs anschauen:" -ForegroundColor Cyan
Write-Host "  docker-compose -f C:\VCC\themis\docker\compose\docker-compose-sharding.yml logs -f" -ForegroundColor Gray
Write-Host ""
Write-Host "Cluster stoppen:" -ForegroundColor Cyan
Write-Host "  docker-compose -f C:\VCC\themis\docker\compose\docker-compose-sharding.yml down" -ForegroundColor Gray
Write-Host ""

# Wait for health checks
Write-Host "⏳ Warte auf Health Checks (30s)..." -ForegroundColor Yellow
Start-Sleep -Seconds 30

# Check health status
Write-Host ""
Write-Host "Health Status:" -ForegroundColor Cyan
docker ps --filter "name=themis-" --format "table {{.Names}}\t{{.Status}}"

Write-Host ""
Write-Host "🎉 Setup abgeschlossen!" -ForegroundColor Green
