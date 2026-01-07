#!/usr/bin/env pwsh
param([string]$BackupPath = "./data/backups/backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')")
Write-Host "Creating backup to: $BackupPath" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $BackupPath | Out-Null
Copy-Item ./data/db/* -Destination $BackupPath -Recurse -Force
Write-Host "Backup completed!" -ForegroundColor Green
