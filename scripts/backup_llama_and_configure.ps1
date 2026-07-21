$ErrorActionPreference = 'Stop'
$ts = Get-Date -Format 'yyyyMMdd-HHmmss'
if (Test-Path 'llama.cpp') {
  Rename-Item -LiteralPath 'llama.cpp' -NewName "llama.cpp.backup.$ts"
  Write-Output "Renamed llama.cpp -> llama.cpp.backup.$ts"
} else {
  Write-Output 'No llama.cpp directory found'
}

Write-Output "Running: cmake --preset windows-release"
cmake --preset windows-release
