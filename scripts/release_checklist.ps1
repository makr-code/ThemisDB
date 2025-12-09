#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Release Quality Checklist for THEMIS Production Releases
    
.DESCRIPTION
    Comprehensive pre-release validation checklist following SLSA framework
    
.EXAMPLE
    .\scripts\release_checklist.ps1 -Version "1.0.2"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [string]$ReleaseDir = "release",
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║   THEMIS Release Checklist - SLSA Framework               ║" -ForegroundColor Cyan
Write-Host "║   Version: $Version" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan

$checklist = @{
    "Code Quality" = @(
        @{ Item = "Unit tests passing"; Check = $false }
        @{ Item = "Build succeeds"; Check = $false }
        @{ Item = "No compiler warnings"; Check = $false }
        @{ Item = "Code review completed"; Check = $false }
        @{ Item = "Security scan passed"; Check = $false }
    )
    
    "Version Control" = @(
        @{ Item = "Version in CMakeLists.txt updated"; Check = $false }
        @{ Item = "Version in VERSION file updated"; Check = $false }
        @{ Item = "CHANGELOG.md updated"; Check = $false }
        @{ Item = "Git commit created"; Check = $false }
        @{ Item = "Git tag created (v$Version)"; Check = $false }
    )
    
    "Packages" = @(
        @{ Item = "Production packages created (3 variants × 3 platforms)"; Check = $false }
        @{ Item = "Minimal packages created (Windows, Linux, QNAP, DEB, RPM)"; Check = $false }
        @{ Item = "All packages > 1 MB"; Check = $false }
        @{ Item = "All packages have unique names"; Check = $false }
        @{ Item = "Binary executable included"; Check = $false }
    )
    
    "Configuration" = @(
        @{ Item = "config.json ready"; Check = $false }
        @{ Item = "mime_types.yaml ready"; Check = $false }
        @{ Item = "content_processors.yaml ready"; Check = $false }
        @{ Item = "34+ config templates included"; Check = $false }
        @{ Item = "Platform-specific configs (Windows, Linux, QNAP, RPi)"; Check = $false }
    )
    
    "Documentation" = @(
        @{ Item = "README.md updated"; Check = $false }
        @{ Item = "CHANGELOG.md complete"; Check = $false }
        @{ Item = "INSTALLATION.md up-to-date"; Check = $false }
        @{ Item = "Release guide created"; Check = $false }
        @{ Item = "Package contents documented"; Check = $false }
    )
    
    "Scripts & Tools" = @(
        @{ Item = "start-themis.sh created and tested"; Check = $false }
        @{ Item = "start-themis.bat created and tested"; Check = $false }
        @{ Item = "Helper scripts (backup, restore) included"; Check = $false }
        @{ Item = "Example code included"; Check = $false }
        @{ Item = ".gitignore/dockerignore updated"; Check = $false }
    )
    
    "Integrity & Security" = @(
        @{ Item = "SHA256 checksums generated"; Check = $false }
        @{ Item = "Checksums verified"; Check = $false }
        @{ Item = "HMAC signatures created"; Check = $false }
        @{ Item = "SBOM (CycloneDX) generated"; Check = $false }
        @{ Item = "No secrets in packages"; Check = $false }
    )
    
    "Distribution" = @(
        @{ Item = "Packages on GitHub releases page"; Check = $false }
        @{ Item = "Checksums available"; Check = $false }
        @{ Item = "SBOM available"; Check = $false }
        @{ Item = "Release notes published"; Check = $false }
        @{ Item = "Announcement in documentation"; Check = $false }
    )
    
    "Testing" = @(
        @{ Item = "Windows package tested"; Check = $false }
        @{ Item = "Linux package tested"; Check = $false }
        @{ Item = "QNAP package tested"; Check = $false }
        @{ Item = "Startup scripts verified"; Check = $false }
        @{ Item = "Health check endpoint tested"; Check = $false }
    )
}

# Verify actual files
function Test-ReleaseArtifacts {
    param([string]$Version, [string]$ReleaseDir)
    
    Write-Host "`n📦 VERIFYING RELEASE ARTIFACTS..." -ForegroundColor Yellow
    
    $found = @{}
    
    # Check for packages
    $packages = Get-ChildItem -Path $ReleaseDir -Include "*$Version*.zip", "*$Version*.deb", "*$Version*.rpm" -ErrorAction SilentlyContinue
    $found["Packages"] = $packages.Count
    
    # Check for documentation
    $docs = Get-ChildItem -Path $ReleaseDir -Include "*RELEASE_GUIDE*", "*INSTALLATION*", "README*" -ErrorAction SilentlyContinue
    $found["Docs"] = $docs.Count
    
    # Check for signatures
    $sigs = Get-ChildItem -Path $ReleaseDir -Include "*SIGNATURES*", "*SBOM*" -ErrorAction SilentlyContinue
    $found["Signatures"] = $sigs.Count
    
    return $found
}

function Display-Checklist {
    param([hashtable]$List)
    
    $totalItems = 0
    $completedItems = 0
    
    foreach ($category in $List.Keys | Sort-Object) {
        Write-Host "`n$category" -ForegroundColor Yellow
        Write-Host "─" * 50 -ForegroundColor Gray
        
        foreach ($item in $List[$category]) {
            $symbol = if ($item.Check) { "✓" } else { "☐" }
            $color = if ($item.Check) { "Green" } else { "Gray" }
            
            Write-Host "  $symbol $($item.Item)" -ForegroundColor $color
            
            $totalItems++
            if ($item.Check) { $completedItems++ }
        }
    }
    
    $percentage = [math]::Round(($completedItems / $totalItems) * 100, 1)
    
    Write-Host "`n" -ForegroundColor Gray
    Write-Host "═" * 50 -ForegroundColor Gray
    Write-Host "PROGRESS: $completedItems/$totalItems items ($percentage%)" -ForegroundColor Cyan
    Write-Host "═" * 50 -ForegroundColor Gray
    
    return @{ Completed = $completedItems; Total = $totalItems; Percentage = $percentage }
}

function Get-SLSA-Level {
    param([int]$Percentage)
    
    if ($Percentage -lt 50) { return "❌ Below SLSA L1" }
    elseif ($Percentage -lt 70) { return "⚠️  SLSA L1 (Partial)" }
    elseif ($Percentage -lt 85) { return "✅ SLSA L1 (Complete)" }
    elseif ($Percentage -lt 95) { return "✅ SLSA L2 (Near Complete)" }
    else { return "✅ SLSA L3 (Enterprise Ready)" }
}

# ============================================================================
# INTERACTIVE MODE
# ============================================================================

function Interactive-Checklist {
    param([hashtable]$List)
    
    Write-Host "`n🔍 INTERACTIVE VERIFICATION MODE" -ForegroundColor Cyan
    Write-Host "Enter 'y' for yes, 'n' for no, 'skip' to skip item, 'done' to finish`n"
    
    $categoryCount = 1
    foreach ($category in $List.Keys | Sort-Object) {
        Write-Host "`n[$categoryCount] $category" -ForegroundColor Yellow
        
        $itemCount = 1
        foreach ($item in $List[$category]) {
            $response = Read-Host "  [$itemCount] $($item.Item) (y/n/skip)"
            
            switch ($response.ToLower()) {
                "y" { $item.Check = $true }
                "n" { $item.Check = $false }
                "skip" { }
                "done" { return }
                default { Write-Host "    Invalid input, skipping..." -ForegroundColor Gray }
            }
            
            $itemCount++
        }
        
        $categoryCount++
    }
}

# ============================================================================
# MAIN
# ============================================================================

Write-Host "`nMODE SELECTION:" -ForegroundColor Cyan
Write-Host "1. Auto-verify (quick check of files)" -ForegroundColor Green
Write-Host "2. Interactive (manual verification)" -ForegroundColor Green
Write-Host ""

$mode = Read-Host "Select mode (1-2, default 1)"
if ([string]::IsNullOrEmpty($mode)) { $mode = "1" }

if ($mode -eq "2") {
    Interactive-Checklist -List $checklist
}

# Verify artifacts
$artifacts = Test-ReleaseArtifacts -Version $Version -ReleaseDir $ReleaseDir

Write-Host "`n📂 ARTIFACT SUMMARY:" -ForegroundColor Yellow
Write-Host "  Packages found: $($artifacts.Packages)" -ForegroundColor Green
Write-Host "  Documentation: $($artifacts.Docs)" -ForegroundColor Green
Write-Host "  Signatures/SBOM: $($artifacts.Signatures)" -ForegroundColor Green

# Display checklist
$progress = Display-Checklist -List $checklist

# Determine SLSA level
$salsaLevel = Get-SLSA-Level -Percentage $progress.Percentage

Write-Host "`n🏆 MATURITY LEVEL:" -ForegroundColor Cyan
Write-Host "  $salsaLevel" -ForegroundColor Green

# Final recommendations
Write-Host "`n📋 RECOMMENDATIONS:" -ForegroundColor Yellow

if ($progress.Percentage -lt 100) {
    Write-Host "  Complete remaining items before release:" -ForegroundColor Yellow
    
    foreach ($category in $checklist.Keys | Sort-Object) {
        foreach ($item in $checklist[$category]) {
            if (-not $item.Check) {
                Write-Host "    ❌ $($item.Item)" -ForegroundColor Red
            }
        }
    }
}

if ($progress.Percentage -ge 95) {
    Write-Host "  ✅ Ready for production release!" -ForegroundColor Green
} elseif ($progress.Percentage -ge 85) {
    Write-Host "  ⚠️  Fix remaining items before release" -ForegroundColor Yellow
} else {
    Write-Host "  ❌ Complete more items before release" -ForegroundColor Red
}

Write-Host "`nFor automated release pipeline, run:" -ForegroundColor Cyan
Write-Host "  .\scripts\enterprise_release.ps1 -Version '$Version' -Action 'full'" -ForegroundColor Green

if ($progress.Percentage -eq 100) {
    exit 0
} else {
    exit 1
}
