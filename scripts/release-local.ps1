param(
  [string]$Version = (Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue).Trim(),
  [switch]$BuildWindows,
  [switch]$BuildLinux,
  [switch]$BuildDocker,
  [switch]$PackageDeb,
  [switch]$PackageRpm,
  [switch]$PushDocker,
  [string]$DockerImage = 'themisdb/themisdb',
  [string]$Tag = '' ,
  [switch]$CreateGithubRelease,
  [string]$GithubRepo = 'makr-code/ThemisDB'
)

$ErrorActionPreference = 'Stop'

if (-not $Version) { throw "Version nicht gefunden. Bitte via -Version angeben oder VERSION-Datei pflegen." }
if (-not $Tag) { $Tag = "v$Version" }

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$releaseDir = Join-Path $root "release"
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null

Write-Host "=== ThemisDB Local Release $Version ($Tag) ===" -ForegroundColor Cyan

# 1) Optional: Build steps
if ($BuildWindows) {
  & (Join-Path $PSScriptRoot 'build.ps1') -Target windows
}
if ($BuildLinux) {
  & (Join-Path $PSScriptRoot 'build.ps1') -Target linux
}
if ($BuildDocker) {
  & (Join-Path $PSScriptRoot 'build.ps1') -Target docker -Push:([bool]$PushDocker) -Tag $Version
}

# 2) Package ZIPs
& (Join-Path $PSScriptRoot 'package-zip.ps1') -Version $Version -Platform windows
& (Join-Path $PSScriptRoot 'package-zip.ps1') -Version $Version -Platform linux

# 3) Deb/RPM (optional; requires Linux/WSL tools)
if ($PackageDeb) {
  if (Get-Command wsl -ErrorAction SilentlyContinue) {
    wsl bash -lc "cd /mnt/$(($root -replace ':','').Replace('\\','/').Substring(1)); ./scripts/package-deb.sh $Version"
  } else {
    Write-Host "WSL nicht gefunden. Versuche native Bash..." -ForegroundColor Yellow
    & bash "-lc" "cd '$root'; ./scripts/package-deb.sh $Version"
  }
}
if ($PackageRpm) {
  if (Get-Command wsl -ErrorAction SilentlyContinue) {
    wsl bash -lc "cd /mnt/$(($root -replace ':','').Replace('\\','/').Substring(1)); ./scripts/package-rpm.sh $Version"
  } else {
    Write-Host "WSL nicht gefunden. Versuche native Bash..." -ForegroundColor Yellow
    & bash "-lc" "cd '$root'; ./scripts/package-rpm.sh $Version"
  }
}

# 4) Checksums
& (Join-Path $PSScriptRoot 'package-checksums.ps1') -ArtifactsDir $releaseDir

# 5) Docker Push (wenn nicht bereits gepusht)
if ($PushDocker -and -not $BuildDocker) {
  & (Join-Path $PSScriptRoot 'build-docker.ps1') -Push -Tag $Version
}

# 6) Optional: GitHub Release via gh CLI
if ($CreateGithubRelease) {
  if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw "GitHub CLI (gh) nicht installiert. Bitte installieren oder -CreateGithubRelease weglassen."
  }

  $tagExists = (git tag --list $Tag) -ne $null
  if (-not $tagExists) {
    git tag -a $Tag -m "Release $Tag"
    git push origin $Tag
  }

  $assets = Get-ChildItem $releaseDir -Recurse -File | Where-Object { $_.Name -match '\.(zip|deb|rpm|txt)$' }
  $assetArgs = @()
  foreach ($a in $assets) { $assetArgs += @('-a', $a.FullName) }

  gh release create $Tag @assetArgs -R $GithubRepo -t "Release $Tag" -n @"
ThemisDB $Version

Docker:
  docker pull $DockerImage:$Version

Binary Packages:
  - Windows x64 ZIP
  - Linux x64 ZIP
  - Debian (.deb) [optional]
  - RPM (.rpm) [optional]

Checksums: SHA256SUMS.txt
"@
  Write-Host "✓ GitHub Release erstellt: $GithubRepo @ $Tag" -ForegroundColor Green
}

Write-Host "\n✓ Lokaler Release-Prozess abgeschlossen. Artefakte: $releaseDir" -ForegroundColor Green
