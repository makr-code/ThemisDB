$target='C:\Projects\ThemisDB\build-msvc-windows-release\bin_out\test_content_security_focused.exe'
$zip="$env:TEMP\handle.zip"
$dir="$env:TEMP\handle_thems"
try {
    Invoke-WebRequest -Uri 'https://download.sysinternals.com/files/Handle.zip' -OutFile $zip -UseBasicParsing -ErrorAction Stop
} catch {
    Write-Output ("Download failed: {0}" -f $_)
    exit 2
}
if(Test-Path $dir){ Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue }
try {
    Expand-Archive -LiteralPath $zip -DestinationPath $dir -Force
} catch {
    Write-Output ("Expand failed: {0}" -f $_)
    exit 3
}
$h = Join-Path $dir 'handle.exe'
if(-not (Test-Path $h)) { Write-Output "handle.exe not found at $h"; exit 4 }
# Run handle to find locks
$lines = & $h -accepteula $target 2>&1
Write-Output "--- handle output start ---"
$lines | ForEach-Object { Write-Output $_ }
Write-Output "--- handle output end ---"
# Extract PIDs
$pids = $lines | ForEach-Object { if ($_ -match 'pid: (\d+)') { $matches[1] } } | Select-Object -Unique
if($pids -and $pids.Count -gt 0) {
    foreach($pid in $pids) {
        Write-Output "Stopping PID $pid"
        try { Stop-Process -Id $pid -Force -ErrorAction Stop; Write-Output ("Stopped {0}" -f $pid) } catch { Write-Output ("Failed to stop {0}: {1}" -f $pid, $_) }
    }
} else { Write-Output "No pids found locking file." }
# Try delete
    try { Remove-Item -LiteralPath $target -Force -ErrorAction Stop; Write-Output "Deleted target" } catch { Write-Output ("Delete failed: {0}" -f $_) }
exit 0
