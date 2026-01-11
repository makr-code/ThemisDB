#!/usr/bin/env pwsh
<#
Check if GitHub Issues match the templates and have correct labels
#>

$TemplatesDir = ".\.github\ISSUE_TEMPLATE"
$Templates = @{
    "error_code_migration_phase1.md" = "Error Code Migration Phase 1"
    "error_code_migration_phase2.md" = "Error Code Migration Phase 2"
    "error_code_migration_phase3.md" = "Error Code Migration Phase 3"
    "define_additional_error_codes.md" = "Define Additional Error Codes"
    "lora_framework.md" = "LoRA Adapter Framework"
    "lora-aql-functions.md" = "LoRA AQL Functions"
    "lora-rest-api.md" = "LoRA REST API"
    "lora-llm-integration.md" = "LoRA LLM Integration"
    "lora-docker-compose.md" = "LoRA Docker"
    "lora-grafana-dashboard.md" = "LoRA Grafana"
    "git_features_phase1_named_snapshots.md" = "Named Snapshots"
    "git_features_phase2_diff_api.md" = "Diff API"
    "git_features_phase3_pitr.md" = "Point-in-Time Recovery"
    "http_server_error_api_integration.md" = "HTTP Server Error API"
}

Write-Host "=== GitHub Issue Label Audit ===" -ForegroundColor Cyan

$Results = @()

foreach ($file in $Templates.Keys) {
    $path = Join-Path $TemplatesDir $file
    $displayName = $Templates[$file]
    
    if (-not (Test-Path $path)) {
        $Results += [PSCustomObject]@{
            Template = $displayName
            Issue = "FILE MISSING"
            Status = "❌ DATEI NICHT GEFUNDEN"
            ExpectedLabels = ""
            ActualLabels = ""
            Missing = ""
            Extra = ""
        }
        continue
    }
    
    # Extract expected labels from template
    $content = Get-Content $path -Raw
    $labelMatch = $content -match 'labels:\s*[''"]?([^''"`\r\n]+)'
    $expectedLabels = @()
    if ($labelMatch) {
        $expectedLabels = @($matches[1] -split '\s*,\s*' | ForEach-Object { $_.Trim().Trim('"').Trim("'") })
    }
    
    # Search GitHub for matching issue
    try {
        $searchResult = gh issue list --search $displayName --limit 1 --json title,labels,number,state 2>&1 | ConvertFrom-Json
        
        if ($searchResult -and $searchResult.number) {
            $issueNum = $searchResult.number
            $actualLabels = @($searchResult.labels.name)
            
            $missing = @($expectedLabels | Where-Object { $_ -notin $actualLabels })
            $extra = @($actualLabels | Where-Object { $_ -notin $expectedLabels })
            
            $status = if ($missing.Count -eq 0 -and $extra.Count -eq 0) {
                "✅ OK"
            } else {
                "⚠️ LABELS MISMATCH"
            }
            
            $Results += [PSCustomObject]@{
                Template = $displayName
                Issue = "#$issueNum"
                Status = $status
                ExpectedLabels = ($expectedLabels -join ", ")
                ActualLabels = ($actualLabels -join ", ")
                Missing = ($missing -join ", ")
                Extra = ($extra -join ", ")
            }
        } else {
            $Results += [PSCustomObject]@{
                Template = $displayName
                Issue = "N/A"
                Status = "❌ NICHT GEFUNDEN"
                ExpectedLabels = ($expectedLabels -join ", ")
                ActualLabels = ""
                Missing = ""
                Extra = ""
            }
        }
    } catch {
        $Results += [PSCustomObject]@{
            Template = $displayName
            Issue = "ERROR"
            Status = "❌ FEHLER"
            ExpectedLabels = ($expectedLabels -join ", ")
            ActualLabels = ""
            Missing = ""
            Extra = $_.Exception.Message
        }
    }
}

Write-Host ""
$Results | Format-Table -AutoSize -Wrap

Write-Host ""
Write-Host "=== Zusammenfassung ===" -ForegroundColor Green
$okCount = ($Results | Where-Object { $_.Status -eq "✅ OK" }).Count
$mismatchCount = ($Results | Where-Object { $_.Status -match "⚠️" }).Count
$notFoundCount = ($Results | Where-Object { $_.Status -match "❌" }).Count

Write-Host "✅ OK: $okCount | ⚠️ Mismatch: $mismatchCount | ❌ Nicht gefunden: $notFoundCount" -ForegroundColor Yellow
Write-Host ""

if ($mismatchCount -gt 0) {
    Write-Host "=== Details: Label Mismatches ===" -ForegroundColor Yellow
    $Results | Where-Object { $_.Status -match "⚠️" } | ForEach-Object {
        Write-Host "`n[$($_.Issue)] $($_.Template)" -ForegroundColor Yellow
        if ($_.Missing) { Write-Host "  ❌ Fehlende Labels: $($_.Missing)" -ForegroundColor Red }
        if ($_.Extra) { Write-Host "  ➕ Zusätzliche Labels: $($_.Extra)" -ForegroundColor Blue }
    }
}

Write-Host ""
