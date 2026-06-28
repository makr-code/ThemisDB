# Wait for vcpkg to finish (max 60 minutes)
$maxSeconds = 3600
$interval = 10
$elapsed = 0
while ($elapsed -lt $maxSeconds) {
  $p = Get-Process -Name vcpkg -ErrorAction SilentlyContinue
  if (-not $p) { Write-Output "vcpkg_done"; exit 0 }
  Write-Output ("vcpkg_running {0} CPU:{1} Elapsed:{2}s" -f $p.Id, $p.CPU, $elapsed)
  Start-Sleep -Seconds $interval
  $elapsed += $interval
}
Write-Output "vcpkg_timeout"
exit 2
