#!/usr/bin/env pwsh
<#
Update GitHub Issues with new labels based on template mapping
#>

$Updates = @(
    @{ issue = 333; name = "Error Code Phase 1"; labels = "type:enhancement,type:bug,type:refactoring,priority:P0" },
    @{ issue = 334; name = "Error Code Phase 2"; labels = "type:enhancement,type:bug,type:refactoring,priority:P2" },
    @{ issue = 335; name = "Error Code Phase 3"; labels = "type:enhancement,type:bug,type:refactoring,priority:P3" },
    @{ issue = 332; name = "Error Codes"; labels = "type:enhancement,type:bug,priority:P2" },
    @{ issue = 319; name = "LoRA Framework"; labels = "type:enhancement,area:llm,lora,documentation,priority:P0,strategic" },
    @{ issue = 326; name = "LoRA AQL"; labels = "type:enhancement,area:aql,lora" },
    @{ issue = 331; name = "LoRA REST API"; labels = "type:enhancement,area:api,lora" },
    @{ issue = 328; name = "LoRA Docker"; labels = "type:enhancement,area:docker,lora" },
    @{ issue = 329; name = "LoRA Grafana"; labels = "type:enhancement,documentation,area:monitoring,lora" },
    @{ issue = 336; name = "HTTP Server Error API"; labels = "type:enhancement,type:bug,area:api,priority:P3" }
)

Write-Host "=== Aktualisiere GitHub Issues mit neuen Labels ===" -ForegroundColor Cyan
Write-Host ""

$successCount = 0
$errorCount = 0

foreach ($update in $Updates) {
    $issueNum = $update.issue
    $name = $update.name
    $labels = $update.labels -split ','
    
    Write-Host "[#$issueNum] $name" -ForegroundColor Yellow
    
    try {
        # Get current labels to remove them
        $current = gh issue view $issueNum --json labels | ConvertFrom-Json
        $currentLabels = $current.labels.name
        
        if ($currentLabels -and $currentLabels.Count -gt 0) {
            Write-Host "  Entferne alte Labels: $($currentLabels -join ', ')" -ForegroundColor DarkCyan
            gh issue edit $issueNum --remove-label "$($currentLabels -join ',')" 2>&1 | Out-Null
        }
        
        # Add new labels
        Write-Host "  Füge neue Labels hinzu: $($labels -join ', ')" -ForegroundColor Green
        gh issue edit $issueNum --add-label "$($labels -join ',')" 2>&1 | Out-Null
        
        Write-Host "  ✅ Erfolgreich" -ForegroundColor Green
        $successCount++
    } catch {
        Write-Host "  ❌ Fehler: $($_.Exception.Message)" -ForegroundColor Red
        $errorCount++
    }
    
    Write-Host ""
}

Write-Host ""
Write-Host "=== Zusammenfassung ===" -ForegroundColor Green
Write-Host "✅ Erfolgreich: $successCount | ❌ Fehler: $errorCount" -ForegroundColor Yellow
Write-Host ""
