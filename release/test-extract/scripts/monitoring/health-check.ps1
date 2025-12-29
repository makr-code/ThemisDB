#!/usr/bin/env pwsh
$response = Invoke-WebRequest -Uri http://localhost:8765/health -UseBasicParsing
if ($response.StatusCode -eq 200) {
    Write-Host "ThemisDB is healthy" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Health check failed" -ForegroundColor Red
    exit 1
}
