# Wait for vcpkg to finish (max 10 minutes)
$maxSeconds = 600
$interval = 5
$elapsed = 0
while ($elapsed -lt $maxSeconds) {
  $p = Get-Process -Name vcpkg -ErrorAction SilentlyContinue
  if (-not $p) { Write-Output "vcpkg_done"; exit 0 }
  Write-Output ("vcpkg_running {0} CPU:{1}" -f $p.Id, $p.CPU)
  Start-Sleep -Seconds $interval
  $elapsed += $interval
}
Write-Output "vcpkg_timeout"
exit 2
