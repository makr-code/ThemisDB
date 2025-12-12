param(
  [string]$ArtifactsDir = (Join-Path $PSScriptRoot "..\release"),
  [string]$OutputFile = (Join-Path $PSScriptRoot "..\release\SHA256SUMS.txt")
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $ArtifactsDir)) { throw "Artifacts-Verzeichnis nicht gefunden: $ArtifactsDir" }

if (Test-Path $OutputFile) { Remove-Item $OutputFile -Force }

Get-ChildItem $ArtifactsDir -Recurse -File |
  Where-Object {
    $_.Name -match '\\.(zip|deb|rpm|exe)$' -or $_.Name -match '^themisdb-v.+-linux-.*$'
  } |
  ForEach-Object {
    $hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
    $line = "$hash  $($_.Name)"
    $line | Out-File -FilePath $OutputFile -Append -Encoding ASCII
    # per-file .sha256 neben dem Artefakt schreiben
    "$hash  $($_.Name)" | Out-File -FilePath (Join-Path $_.DirectoryName ("$($_.Name).sha256")) -Encoding ASCII
  }

Write-Host "✓ Checksums generiert: $OutputFile" -ForegroundColor Green
