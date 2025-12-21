#!/usr/bin/env pwsh
param([Parameter(Mandatory)][string]$BackupPath)
Write-Host "Restoring from: $BackupPath" -ForegroundColor Yellow
Copy-Item $BackupPath\* -Destination ./data/db -Recurse -Force
Write-Host "Restore completed!" -ForegroundColor Green
