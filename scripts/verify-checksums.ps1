param(
  [string]$ArtifactsDir = (Join-Path $PSScriptRoot "..\release"),
  [string]$ChecksumFile = (Join-Path $PSScriptRoot "..\release\SHA256SUMS.txt")
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $ChecksumFile)) { throw "Checksum-Datei nicht gefunden: $ChecksumFile" }

$lines = Get-Content -Path $ChecksumFile -Encoding ASCII | Where-Object { $_.Trim() }
if ($lines.Count -eq 0) { throw "Checksum-Datei ist leer: $ChecksumFile" }

$failed = 0
foreach ($line in $lines) {
  # Format: <hash><two spaces><filename>
  if ($line -match '^(?<hash>[0-9a-fA-F]{64})\s\s(?<name>.+)$') {
    $expected = $Matches['hash'].ToLower()
    $name = $Matches['name']
    $file = Join-Path $ArtifactsDir $name
    if (-not (Test-Path $file)) {
      Write-Host "Fehlt: $name" -ForegroundColor Red
      $failed++
      continue
    }
    $actual = (Get-FileHash $file -Algorithm SHA256).Hash.ToLower()
    if ($actual -ne $expected) {
      Write-Host "Mismatch: $name" -ForegroundColor Red
      Write-Host "  expected: $expected" -ForegroundColor DarkGray
      Write-Host "  actual  : $actual" -ForegroundColor DarkGray
      $failed++
    } else {
      Write-Host "OK: $name" -ForegroundColor Green
    }
  }
}

if ($failed -gt 0) {
  throw "$failed Checksum-Validierungen fehlgeschlagen."
} else {
  Write-Host "✓ Alle Checksummen verifiziert." -ForegroundColor Green
}
