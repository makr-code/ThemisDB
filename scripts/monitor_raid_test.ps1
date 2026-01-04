# RAID Endurance Test Monitor
# Überwacht den laufenden RAID-Test und zeigt Fortschritt

$testStart = Get-Date "2026-01-04 14:07:13"
$testEnd = $testStart.AddHours(2)

Write-Host "=== RAID Endurance Test Monitor ===" -ForegroundColor Cyan
Write-Host "Start:    $($testStart.ToString('HH:mm:ss'))" -ForegroundColor Yellow
Write-Host "Ende:     $($testEnd.ToString('HH:mm:ss'))" -ForegroundColor Yellow
Write-Host "Dauer:    2 Stunden" -ForegroundColor Yellow
Write-Host ""

while ($true) {
    $now = Get-Date
    $elapsed = $now - $testStart
    $remaining = $testEnd - $now
    
    $progressPercent = [Math]::Min(100, ($elapsed.TotalMinutes / 120) * 100)
    
    Clear-Host
    Write-Host "=== RAID Endurance Test Monitor ===" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Test-Status:" -ForegroundColor White
    Write-Host "  Start:      $($testStart.ToString('HH:mm:ss'))" -ForegroundColor Gray
    Write-Host "  Aktuell:    $($now.ToString('HH:mm:ss'))" -ForegroundColor Gray
    Write-Host "  Ende:       $($testEnd.ToString('HH:mm:ss'))" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Fortschritt:" -ForegroundColor White
    Write-Host "  Elapsed:    $($elapsed.ToString('hh\:mm\:ss'))" -ForegroundColor Green
    Write-Host "  Remaining:  $($remaining.ToString('hh\:mm\:ss'))" -ForegroundColor Yellow
    Write-Host "  Progress:   $([Math]::Round($progressPercent, 1))%" -ForegroundColor Cyan
    
    # Progress Bar
    $barLength = 50
    $filledLength = [Math]::Floor($barLength * $progressPercent / 100)
    $bar = "█" * $filledLength + "░" * ($barLength - $filledLength)
    Write-Host "  [$bar]" -ForegroundColor Cyan
    Write-Host ""
    
    # Container Status
    Write-Host "Container-Status:" -ForegroundColor White
    try {
        $containers = docker ps --filter "name=themis-raid" --format "{{.Names}}: {{.Status}}"
        $unhealthy = 0
        $healthy = 0
        foreach ($line in $containers) {
            if ($line -match "unhealthy") {
                $unhealthy++
                Write-Host "  ⚠ $line" -ForegroundColor Yellow
            } elseif ($line -match "Up") {
                $healthy++
            }
        }
        if ($unhealthy -eq 0) {
            Write-Host "  ✓ Alle Shards healthy ($healthy/9)" -ForegroundColor Green
        } else {
            Write-Host "  ⚠ $unhealthy unhealthy, $healthy healthy" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  ✗ Fehler beim Abrufen des Container-Status" -ForegroundColor Red
    }
    Write-Host ""
    
    # Disk Usage
    Write-Host "Volumes:" -ForegroundColor White
    try {
        $volumes = docker volume ls --filter "name=compose_raid" --format "{{.Name}}"
        Write-Host "  RAID Volumes: $($volumes.Count)" -ForegroundColor Gray
    } catch {
        Write-Host "  ✗ Fehler beim Abrufen der Volumes" -ForegroundColor Red
    }
    Write-Host ""
    
    # Test noch aktiv?
    if ($now -gt $testEnd) {
        Write-Host "⏰ Test sollte abgeschlossen sein!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Prüfe Test-Output mit:" -ForegroundColor Cyan
        Write-Host "  Get-Content raid_endurance_test.log" -ForegroundColor White
        break
    }
    
    Write-Host "Drücke Ctrl+C zum Beenden (Test läuft weiter)" -ForegroundColor DarkGray
    Start-Sleep -Seconds 30
}
