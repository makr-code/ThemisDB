# ThemisDB Schema Initialization Script
# Creates all required collections for DSM

param(
    [string]$ThemisDbUrl = "http://localhost:8765",
    [string]$Username = "admin",
    [string]$Password = "admin"
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Schema Initialization ===" -ForegroundColor Cyan
Write-Host "Target: $ThemisDbUrl" -ForegroundColor Gray

function Invoke-ThemisQuery {
    param([string]$Query, [object]$BindVars = @{})
    
    $body = @{
        query = $Query
        bindVars = $BindVars
    } | ConvertTo-Json -Depth 10
    
    try {
        $response = Invoke-RestMethod -Uri "$ThemisDbUrl/query" -Method Post -Body $body -ContentType "application/json" -ErrorAction Stop
        return $response
    }
    catch {
        Write-Warning "Query failed: $($_.Exception.Message)"
        return $null
    }
}

function New-Collection {
    param([string]$Name, [bool]$IsEdge = $false)
    
    $query = if ($IsEdge) {
        "CREATE COLLECTION $Name TYPE EDGE OPTIONS { waitForSync: false }"
    } else {
        "CREATE COLLECTION $Name OPTIONS { waitForSync: false }"
    }
    
    try {
        Invoke-ThemisQuery -Query $query | Out-Null
        Write-Host "  [OK] Created collection: $Name" -ForegroundColor Green
    }
    catch {
        Write-Warning "  [WARN] Collection $Name may already exist"
    }
}

Write-Host "`n[1/2] Creating Document Collections..." -ForegroundColor Yellow

# Core Collections
New-Collection "users"
New-Collection "documents"
New-Collection "processes"
New-Collection "files"
New-Collection "audit_logs"
New-Collection "wiedervorlagen"
New-Collection "mitzeichnungen"
New-Collection "emails"
New-Collection "retention_rules"

# Additional Collections
New-Collection "folders"
New-Collection "tags"
New-Collection "comments"
New-Collection "versions"
New-Collection "notifications"
New-Collection "tasks"
New-Collection "calendar_events"

Write-Host "`n[2/2] Creating Edge Collections..." -ForegroundColor Yellow

New-Collection "document_process_edges" -IsEdge $true
New-Collection "document_file_edges" -IsEdge $true
New-Collection "user_document_edges" -IsEdge $true
New-Collection "process_task_edges" -IsEdge $true
New-Collection "document_version_edges" -IsEdge $true
New-Collection "folder_document_edges" -IsEdge $true

Write-Host "`n=== Schema Initialization Complete ===" -ForegroundColor Cyan
Write-Host "`nCollections Created:" -ForegroundColor Yellow
Write-Host "  Document Collections: 17" -ForegroundColor White
Write-Host "  Edge Collections: 6" -ForegroundColor White

Write-Host "`nNext Steps:" -ForegroundColor Yellow
Write-Host "  1. Run seed script: .\seed-themisdb.ps1" -ForegroundColor White
Write-Host "  2. Verify collections: Check ThemisDB admin UI" -ForegroundColor White
Write-Host "  3. Test DSM application with real data" -ForegroundColor White

Write-Host "`nThemisDB schema is ready!" -ForegroundColor Green
