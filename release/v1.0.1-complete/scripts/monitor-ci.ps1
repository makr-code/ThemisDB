# Monitor CI Workflow Progress
# Tracks act execution and reports status

param(
    [string]$LogFile = "ci_local_test.log",
    [int]$IntervalSeconds = 15,
    [int]$MaxDuration = 1800  # 30 minutes
)

$startTime = Get-Date
$lastSize = 0

Write-Host "=== CI Workflow Monitor ===" -ForegroundColor Cyan
Write-Host "Log File: $LogFile" -ForegroundColor Gray
Write-Host "Refresh: Every $IntervalSeconds seconds" -ForegroundColor Gray
Write-Host "Max Duration: $($MaxDuration/60) minutes`n" -ForegroundColor Gray

while ($true) {
    $elapsed = ((Get-Date) - $startTime).TotalSeconds
    
    if ($elapsed -gt $MaxDuration) {
        Write-Host "`n⏱️  Timeout reached ($($MaxDuration/60) minutes)" -ForegroundColor Yellow
        break
    }
    
    if (Test-Path $LogFile) {
        $content = Get-Content $LogFile -Raw -ErrorAction SilentlyContinue
        $currentSize = $content.Length
        
        if ($currentSize -ne $lastSize) {
            Clear-Host
            Write-Host "=== CI Workflow Progress ===" -ForegroundColor Cyan
            Write-Host "Elapsed: $([math]::Round($elapsed/60, 1)) minutes" -ForegroundColor Gray
            Write-Host "Log Size: $([math]::Round($currentSize/1024, 1)) KB`n" -ForegroundColor Gray
            
            # Extract key progress indicators
            $lines = $content -split "`n"
            
            # Current step
            $currentStep = $lines | Select-String -Pattern "\[CI.*\] Ô¡É Run Main" | Select-Object -Last 1
            if ($currentStep) {
                $stepName = $currentStep -replace '.*Run Main\s+', '' -replace '\s+$', ''
                Write-Host "📍 Current Step: $stepName" -ForegroundColor Green
            }
            
            # Completed steps
            $completed = ($lines | Select-String -Pattern "Ô£à  Success").Count
            Write-Host "✅ Completed Steps: $completed" -ForegroundColor Green
            
            # Failures
            $failures = ($lines | Select-String -Pattern "ÔØî  Failure").Count
            if ($failures -gt 0) {
                Write-Host "❌ Failed Steps: $failures" -ForegroundColor Red
            }
            
            # Recent activity (last 10 lines with content)
            Write-Host "`n📋 Recent Activity:" -ForegroundColor Yellow
            $lines | Where-Object { $_ -match '\|' -and $_.Trim().Length -gt 0 } | 
                Select-Object -Last 10 | 
                ForEach-Object { 
                    $line = $_ -replace '.*\|\s*', '  '
                    Write-Host $line -ForegroundColor Gray
                }
            
            $lastSize = $currentSize
        }
    } else {
        Write-Host "⏳ Waiting for log file..." -ForegroundColor Yellow
    }
    
    # Check if workflow completed
    if ($content -match "(Job succeeded|Job failed|Failure - Main)") {
        Write-Host "`n🏁 Workflow Completed!" -ForegroundColor Cyan
        
        if ($content -match "Job succeeded") {
            Write-Host "✅ Status: SUCCESS" -ForegroundColor Green
        } else {
            Write-Host "❌ Status: FAILED" -ForegroundColor Red
        }
        
        break
    }
    
    Start-Sleep -Seconds $IntervalSeconds
}

Write-Host "`n📄 Full log available at: $LogFile" -ForegroundColor Gray
