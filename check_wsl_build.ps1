# WSL Build Status Monitor
Write-Host "=== WSL Build Status ===" -ForegroundColor Cyan

# Check if build process is running
$processes = wsl -d Ubuntu bash -c "ps aux | grep -E '(cmake|ninja)' | grep -v grep"
if ($processes) {
    Write-Host "`nActive build processes:" -ForegroundColor Green
    Write-Host $processes
} else {
    Write-Host "`nNo active build processes found" -ForegroundColor Yellow
}

# Show last 30 lines of config log
Write-Host "`n=== CMake Config Log (last 30 lines) ===" -ForegroundColor Cyan
wsl -d Ubuntu bash -c "tail -n 30 /mnt/c/VCC/themis/wsl_cmake_config.log 2>&1"

# Show last 30 lines of build log if it exists
Write-Host "`n=== Build Log (last 30 lines) ===" -ForegroundColor Cyan
wsl -d Ubuntu bash -c "tail -n 30 /mnt/c/VCC/themis/wsl_build.log 2>&1 || echo 'Build log not yet created'"

# Check if build succeeded
if (Test-Path "c:\VCC\themis\build-wsl\themis_server") {
    Write-Host "`n=== BUILD SUCCESSFUL ===" -ForegroundColor Green
    wsl -d Ubuntu bash -c "ls -lh /mnt/c/VCC/themis/build-wsl/themis_server"
} else {
    Write-Host "`nBuild not yet complete or failed" -ForegroundColor Yellow
}
