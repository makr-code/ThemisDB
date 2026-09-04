#!/usr/bin/env pwsh
# generate-winget-checksums.ps1
# Generates SHA256 checksums and SBOM for Windows release artifacts.
# Used during automated WinGet manifest creation.

param(
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDir,

    [Parameter(Mandatory = $false)]
    [string]$OutputDir = ".",

    [switch]$IncludeSBOM
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Status {
    param([string]$Message)
    Write-Host "[ ] $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[✓] $Message" -ForegroundColor Green
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "[✗] $Message" -ForegroundColor Red
}

# Validate artifact directory
if (-not (Test-Path $ArtifactDir -PathType Container)) {
    Write-Error-Custom "Artifact directory not found: $ArtifactDir"
    exit 1
}

# Ensure output directory exists
if (-not (Test-Path $OutputDir -PathType Container)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

Write-Status "Scanning artifacts in: $ArtifactDir"

# Find Windows release artifacts
$Artifacts = @()
$Artifacts += Get-ChildItem -Path $ArtifactDir -Filter "*.zip" -File | Where-Object { $_.Name -match "windows|x64|x86" }
$Artifacts += Get-ChildItem -Path $ArtifactDir -Filter "*.msi" -File | Where-Object { $_.Name -match "windows|x64|x86" }

if ($Artifacts.Count -eq 0) {
    Write-Error-Custom "No Windows artifacts (.zip, .msi) found in $ArtifactDir"
    exit 1
}

Write-Success "Found $($Artifacts.Count) Windows artifact(s)"

# Generate checksums
$ChecksumFile = Join-Path $OutputDir "checksums.txt"
$ChecksumJson = Join-Path $OutputDir "checksums.json"

Write-Status "Generating checksums..."

$Checksums = @{}
$ChecksumLines = @()

foreach ($Artifact in $Artifacts) {
    $Hash = (Get-FileHash -Algorithm SHA256 -Path $Artifact.FullName).Hash
    $FileName = $Artifact.Name
    
    $Checksums[$FileName] = @{
        filename = $FileName
        size     = $Artifact.Length
        sha256   = $Hash
        path     = $Artifact.FullName
    }
    
    # Format: SHA256  filename
    $ChecksumLines += "$Hash  $FileName"
    Write-Success "$FileName : $Hash"
}

# Write checksums.txt (standard format for verification)
$ChecksumLines | Set-Content -Path $ChecksumFile -Encoding UTF8
Write-Success "Checksums written to: $ChecksumFile"

# Write checksums.json (structured format)
$ChecksumsJson = @{
    generated = Get-Date -Format "o"
    artifacts = @($Checksums.Values)
}
$ChecksumsJson | ConvertTo-Json -Depth 10 | Set-Content -Path $ChecksumJson -Encoding UTF8
Write-Success "Checksums (JSON) written to: $ChecksumJson"

# Optional: Generate SBOM if syft is available
if ($IncludeSBOM) {
    Write-Status "Checking for syft (SBOM generation tool)..."
    
    $SyftPath = if ($IsWindows) { "syft.exe" } else { "syft" }
    
    if (Get-Command $SyftPath -ErrorAction SilentlyContinue) {
        $SbomFile = Join-Path $OutputDir "sbom.cdx.json"
        Write-Status "Generating CycloneDX SBOM..."
        
        foreach ($Artifact in $Artifacts) {
            $SbomOutput = Join-Path $OutputDir "sbom-$($Artifact.BaseName).cdx.json"
            & $SyftPath "file:$($Artifact.FullName)" -o cyclonedx-json | Set-Content -Path $SbomOutput -Encoding UTF8
            Write-Success "SBOM written to: $SbomOutput"
        }
    } else {
        Write-Host "[!] syft not found; skipping SBOM generation" -ForegroundColor Yellow
        Write-Host "    Install: https://github.com/anchore/syft#installation" -ForegroundColor Yellow
    }
}

# Summary
Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║                   Checksum Generation Complete                 ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Output files:" -ForegroundColor Cyan
Write-Host "  Checksums (text): $ChecksumFile"
Write-Host "  Checksums (JSON): $ChecksumJson"
if ($IncludeSBOM) {
    Write-Host "  SBOM files: $OutputDir/sbom-*.cdx.json"
}
Write-Host ""

# Return checksums as output for downstream jobs
Write-Host "checksums-file=$ChecksumFile" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
Write-Host "checksums-json=$ChecksumJson" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
