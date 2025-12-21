# Start ThemisDB Server
Write-Host "Starting ThemisDB Server on port 8765..." -ForegroundColor Green
Set-Location "c:\VCC\themis"
.\build-msvc\Release\themis_server.exe
