$path = Join-Path $PSScriptRoot '..\..\vcpkg\buildtrees\grpc'
$latest = Get-ChildItem -Path $path -Recurse -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -ne $latest) {
  Write-Output "Latest: $($latest.FullName)"
  Get-Content -Path $latest.FullName -Tail 300 -ErrorAction SilentlyContinue
} else {
  Write-Output "No grpc log files found yet in $path"
}
