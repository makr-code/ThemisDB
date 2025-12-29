# Railway Monitoring System - Quick Start Script (Windows)
# Startet das komplette System mit einem Befehl

$ErrorActionPreference = "Stop"

Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   Railway Monitoring System - Quick Start (Windows)" -ForegroundColor Cyan
Write-Host "   Deutsche Bahn IoT & Energie-Management mit ThemisDB" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Check prerequisites
Write-Host "[1/6] Checking prerequisites..." -ForegroundColor Blue

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Docker not found. Please install Docker Desktop first." -ForegroundColor Red
    exit 1
}

if (-not (Get-Command docker-compose -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Docker Compose not found. Please install Docker Compose first." -ForegroundColor Red
    exit 1
}

Write-Host "✓ Docker and Docker Compose found" -ForegroundColor Green

# Check for .NET SDK (for WPF client)
$hasDotnet = Get-Command dotnet -ErrorAction SilentlyContinue
if ($hasDotnet) {
    Write-Host "✓ .NET SDK found" -ForegroundColor Green
} else {
    Write-Host "⚠ .NET SDK not found - WPF client will not be available" -ForegroundColor Yellow
}

# Generate network data
Write-Host ""
Write-Host "[2/6] Generating railway network data..." -ForegroundColor Blue

# Create minimal sample data (Python-based generator)
New-Item -ItemType Directory -Force -Path "..\..\data" | Out-Null

$sampleData = @"
{
  "metadata": {
    "name": "German Railway Network (Sample)",
    "version": "1.0",
    "generated": "2024-12-14",
    "stations": 5,
    "segments": 10
  },
  "stations": [
    {"id": "FF", "name": "Frankfurt (Main) Hbf", "lat": 50.1070, "lon": 8.6632},
    {"id": "MH", "name": "Mannheim Hbf", "lat": 49.4793, "lon": 8.4695},
    {"id": "KA", "name": "Karlsruhe Hbf", "lat": 48.9934, "lon": 8.4010},
    {"id": "HN", "name": "Heidelberg Hbf", "lat": 49.4039, "lon": 8.6752},
    {"id": "DA", "name": "Darmstadt Hbf", "lat": 49.8728, "lon": 8.6303}
  ]
}
"@

$sampleData | Out-File -FilePath "..\..\data\railway_network_base_germany.json" -Encoding UTF8
Write-Host "✓ Sample network data created" -ForegroundColor Green

# Start Docker services
Write-Host ""
Write-Host "[3/6] Starting Docker services..." -ForegroundColor Blue
Write-Host "   This will pull and start:" -ForegroundColor Gray
Write-Host "   - ThemisDB (port 8765)" -ForegroundColor Gray
Write-Host "   - Ollama LLM (port 11434)" -ForegroundColor Gray
Write-Host "   - Train Simulator (50 trains)" -ForegroundColor Gray
Write-Host "   - Web UI (port 8080)" -ForegroundColor Gray
Write-Host ""

docker-compose -f docker-compose.railway.yml up -d

Write-Host "✓ Docker services started" -ForegroundColor Green

# Wait for services
Write-Host ""
Write-Host "[4/6] Waiting for services to be ready..." -ForegroundColor Blue
Write-Host "   This may take 1-2 minutes..." -ForegroundColor Gray

$maxWait = 120
$elapsed = 0

while ($elapsed -lt $maxWait) {
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:8765/health" -UseBasicParsing -TimeoutSec 2 -ErrorAction SilentlyContinue
        if ($response.StatusCode -eq 200) {
            Write-Host "✓ ThemisDB ready" -ForegroundColor Green
            break
        }
    } catch {
        # Service not ready yet
    }
    
    Start-Sleep -Seconds 5
    $elapsed += 5
    Write-Host "." -NoNewline -ForegroundColor Gray
}

if ($elapsed -ge $maxWait) {
    Write-Host ""
    Write-Host "❌ Timeout waiting for ThemisDB" -ForegroundColor Red
    Write-Host "Check logs: docker-compose -f docker-compose.railway.yml logs themisdb" -ForegroundColor Yellow
    exit 1
}

Write-Host ""

# Import network data
Write-Host ""
Write-Host "[5/6] Importing railway network to ThemisDB..." -ForegroundColor Blue

Set-Location "..\..\scripts\railway"
python import_railway_network.py "..\..\data\railway_network_base_germany.json"

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Network data imported" -ForegroundColor Green
} else {
    Write-Host "⚠ Import had issues, but continuing..." -ForegroundColor Yellow
}

Set-Location "..\..\examples\railway"

# Show status
Write-Host ""
Write-Host "[6/6] System Status" -ForegroundColor Blue
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan

docker-compose -f docker-compose.railway.yml ps

Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "✓ Railway Monitoring System is running!" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
Write-Host "Access Points:" -ForegroundColor White
Write-Host "  🌐 Web UI:        http://localhost:8080" -ForegroundColor Cyan
Write-Host "  🗄️  ThemisDB API:  http://localhost:8765" -ForegroundColor Cyan
Write-Host "  🤖 Ollama LLM:    http://localhost:11434" -ForegroundColor Cyan
Write-Host ""
Write-Host "Quick Commands:" -ForegroundColor White
Write-Host "  📊 View logs:     docker-compose -f docker-compose.railway.yml logs -f" -ForegroundColor Gray
Write-Host "  📈 Simulator:     docker-compose -f docker-compose.railway.yml logs -f train-simulator" -ForegroundColor Gray
Write-Host "  🛑 Stop system:   docker-compose -f docker-compose.railway.yml down" -ForegroundColor Gray
Write-Host "  🗑️  Clean data:    docker-compose -f docker-compose.railway.yml down -v" -ForegroundColor Gray
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor White
Write-Host "  1. Open http://localhost:8080 to see live train map" -ForegroundColor Yellow
Write-Host "  2. Check simulator: docker logs railway-simulator" -ForegroundColor Yellow

if ($hasDotnet) {
    Write-Host "  3. Run WPF client: cd ..\..\clients\RailwayMonitor.WPF; dotnet run" -ForegroundColor Yellow
} else {
    Write-Host "  3. Install .NET 8 SDK to run WPF client" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Documentation:" -ForegroundColor White
Write-Host "  📖 Complete Guide: ..\..\RAILWAY_COMPLETE_GUIDE.md" -ForegroundColor Gray
Write-Host "  📖 API Docs:       ..\..\docs\projects\RAILWAY_MONITORING.md" -ForegroundColor Gray
Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
