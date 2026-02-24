#!/usr/bin/env pwsh

# Docker Build Monitor - Hyperscaler Edition
# Real-time build progress tracking

param(
    [ValidateSet("hyperscaler", "all")]
    [string]$Target = "all",
    
    [int]$CheckInterval = 30  # seconds between checks
)

$buildImages = @{
    "hyperscaler" = @{
        dockerfile = "docker/Dockerfile.hyperscaler-simple"
        tags = @("themisdb:1.4.0-hyperscaler", "themisdb:hyperscaler-simple")
    }
}

$metrics = @{
    StartTime = $null
    CurrentStep = 0
    TotalSteps = 0
    Status = "starting"
    LastUpdate = Get-Date
}

function Get-BuildMetrics {
    # Try to get build info from docker
    try {
        $buildProcess = Get-Process docker -ErrorAction SilentlyContinue | Where-Object { $_.CommandLine -match "build" }
        if ($buildProcess) {
            return @{
                Active = $true
                ProcessId = $buildProcess.Id
                Memory = [math]::Round($buildProcess.WorkingSet / 1MB, 2)
            }
        }
    } catch {
        # docker might not be accessible
        Write-Warning "Docker not accessible: $_"
    }
    
    return @{ Active = $false }
}

function Show-BuildStatus {
    param($metrics)
    
    Clear-Host
    Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║  ThemisDB HYPERSCALER Docker Build Monitor                      ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
    
    $elapsed = (Get-Date) - $metrics.StartTime
    Write-Host "📊 Build Status" -ForegroundColor Green
    Write-Host "  Status: $($metrics.Status)" -ForegroundColor Yellow
    Write-Host "  Elapsed: $([int]$elapsed.TotalMinutes)m $($elapsed.Seconds)s"
    Write-Host "  Last Update: $($metrics.LastUpdate.ToString('HH:mm:ss'))" -ForegroundColor Gray
    Write-Host ""
    
    $buildMetrics = Get-BuildMetrics
    if ($buildMetrics.Active) {
        Write-Host "🔨 Build Process" -ForegroundColor Green
        Write-Host "  Process ID: $($buildMetrics.ProcessId)"
        Write-Host "  Memory Usage: $($buildMetrics.Memory) MB"
    } else {
        Write-Host "⏸️  Build Process: Idle or Completed" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "📦 Expected Steps" -ForegroundColor Green
    Write-Host "  1. Base image setup (ubuntu:24.04)"
    Write-Host "  2. apt-get install dependencies"
    Write-Host "  3. vcpkg bootstrap"
    Write-Host "  4. CMake configure"
    Write-Host "  5. Ninja build themis_server"
    Write-Host "  6. Copy binary to runtime image"
    Write-Host ""
    
    # Check for images
    Write-Host "🖼️  Built Images" -ForegroundColor Green
    try {
        $images = docker images --filter "reference=themisdb:1.4.0-hyperscaler" --no-trunc -q 2>/dev/null
        if ($images) {
            Write-Host "  ✅ themisdb:1.4.0-hyperscaler exists" -ForegroundColor Green
        } else {
            Write-Host "  ❌ themisdb:1.4.0-hyperscaler not found yet" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  ⚠️  Could not check images" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "💡 Next Steps (when build completes):" -ForegroundColor Cyan
    Write-Host "  1. docker-compose -f docker/compose/docker-compose-raid-hyperscaler.yml up -d"
    Write-Host "  2. Wait for all 6 nodes to be healthy (health check)"
    Write-Host "  3. Configure RAID (RAID0, RAID1, RAID5, RAID6, RAID10)"
    Write-Host "  4. Run comprehensive benchmarks"
    Write-Host ""
    Write-Host "Press Ctrl+C to stop monitoring" -ForegroundColor Gray
}

# Main monitoring loop
$metrics.StartTime = Get-Date
$metrics.Status = "Docker build in progress..."

while ($true) {
    Show-BuildStatus $metrics
    
    # Check if build completed
    try {
        $image = docker images --filter "reference=themisdb:1.4.0-hyperscaler" -q 2>/dev/null
        if ($image) {
            $metrics.Status = "✅ Build COMPLETED"
            Show-BuildStatus $metrics
            Write-Host ""
            Write-Host "Build finished successfully! Image available." -ForegroundColor Green
            break
        }
    } catch {
        # docker command might fail
        Write-Warning "Docker command failed: $_"
    }
    
    $metrics.LastUpdate = Get-Date
    Start-Sleep -Seconds $CheckInterval
}
