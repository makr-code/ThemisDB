<#
.SYNOPSIS
    Find and update vcpkg baseline to a recent stable version
    
.DESCRIPTION
    Queries vcpkg repository for a valid recent baseline and updates all config files
    consistently (vcpkg.json, vcpkg-community.json, vcpkg-configuration.json)
    
.PARAMETER DaysBack
    How many days back to look for baseline (default: 30 = ~1 month old, known stable)
#>

param(
    [int]$DaysBack = 30
)

$ErrorActionPreference = "Stop"

Write-Host "Finding vcpkg baseline from $DaysBack days ago..."

# Create temp vcpkg clone to query history
$tempVcpkg = Join-Path $env:TEMP "vcpkg-baseline-query"
if (Test-Path $tempVcpkg) { Remove-Item $tempVcpkg -Recurse -Force }

# Clone only recent history (shallow)
Write-Host "Cloning vcpkg repository (shallow, recent history only)..."
git clone --depth=50 https://github.com/microsoft/vcpkg.git $tempVcpkg

# Find baseline from ~month ago
$untilDate = (Get-Date).AddDays(-$DaysBack).ToString("yyyy-MM-dd")
Write-Host "Looking for commits until $untilDate..."

$baseline = & git -C $tempVcpkg log --pretty=format:"%H" --all --until=$untilDate | Select-Object -First 1

if (-not $baseline) {
    Write-Error "Failed to find baseline. Try increasing -DaysBack"
    exit 1
}

Write-Host "Found baseline: $baseline"

# Verify it's valid (check versions/baseline.json exists)
$isValid = & git -C $tempVcpkg cat-file -e "$baseline`:`versions/baseline.json" 2>&1
if ($?) {
    Write-Host "✓ Baseline is valid (has versions/baseline.json)"
} else {
    Write-Host "⚠ Warning: Could not verify baseline has versions/baseline.json"
}

# Update files
$projectRoot = $PSScriptRoot
$files = @(
    (Join-Path $projectRoot "vcpkg.json"),
    (Join-Path $projectRoot "docker\vcpkg-community.json"),
    (Join-Path $projectRoot "vcpkg-configuration.json")
)

$oldBaseline = "5fb40866d94f5e6c38ec1a91c0609549bb8aac01"  # Current known baseline

foreach ($file in $files) {
    if (Test-Path $file) {
        $content = Get-Content $file -Raw
        
        # Replace old baseline with new one
        if ($content -match $oldBaseline) {
            $content = $content -replace $oldBaseline, $baseline
            Set-Content $file $content -NoNewline
            Write-Host "✓ Updated: $(Split-Path $file -Leaf)"
        } else {
            Write-Host "⚠ No old baseline found in: $(Split-Path $file -Leaf)"
        }
    }
}

# Cleanup
Remove-Item $tempVcpkg -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "`n✓ Baseline updated to: $baseline"
Write-Host "`nNow rebuild with:"
Write-Host "  Local MSVC: cmake -B build-new && cmake --build build-new"
Write-Host "  Docker:     .\create-build-context.ps1; docker build -t themisdb:latest ..."
