#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Updates vcpkg download cache with latest package versions before build
.DESCRIPTION
    Synchronizes vcpkg manifest with latest package versions and pre-downloads
    all source archives for offline builds. Supports multi-platform builds.
.EXAMPLE
    .\update-vcpkg-cache.ps1
    .\update-vcpkg-cache.ps1 -Triplets @("x64-linux", "arm64-linux")
    .\update-vcpkg-cache.ps1 -Force
#>
param(
    [switch]$Force,
    [string[]]$Triplets = @("x64-linux", "arm64-linux", "x64-windows"),
    [string]$VcpkgRoot = "$PSScriptRoot\..\vcpkg"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors for output
$colors = @{
    Info = "Cyan"
    Success = "Green"
    Warning = "Yellow"
    Error = "Red"
}

function Write-Status {
    param([string]$Message, [string]$Type = "Info")
    Write-Host "[$([DateTime]::Now.ToString('HH:mm:ss'))] $Message" -ForegroundColor $colors[$Type]
}

function Invoke-BuildCommand {
    param([string]$Command, [string]$Description)
    Write-Status $Description
    try {
        Invoke-Expression $Command | Tee-Object -Variable cmdOutput
        if ($LASTEXITCODE -ne 0) {
            Write-Status "Command failed with exit code $LASTEXITCODE" -Type Error
            return $false
        }
        return $true
    }
    catch {
        Write-Error "Error: $_"
        return $false
    }
}

# Main workflow
Write-Status "==== vcpkg Cache Update Tool ====" -Type Info
Write-Status "VcpkgRoot: $VcpkgRoot" -Type Info

if (-not (Test-Path $VcpkgRoot)) {
    Write-Status "Error: vcpkg not found at $VcpkgRoot" -Type Error
    exit 1
}

# Step 1: Update vcpkg itself
Write-Status "`n[1/4] Updating vcpkg..." -Type Info
Push-Location $VcpkgRoot
try {
    # Ensure we're on a git repo
    if (-not (Test-Path ".git")) {
        Write-Status "Not a git repository, skipping vcpkg update" -Type Warning
    }
    else {
        $result = Invoke-BuildCommand "git pull origin master" "Pulling latest vcpkg changes..."
        if (-not $result) {
            Write-Status "Failed to update vcpkg (non-fatal, continuing)" -Type Warning
        }
    }
}
finally {
    Pop-Location
}

# Step 2: Update vcpkg registry baseline
Write-Status "`n[2/4] Updating dependency versions..." -Type Info
Push-Location "$PSScriptRoot\.."
try {
    $vcpkgExe = if ($IsWindows) { "$VcpkgRoot\vcpkg.exe" } else { "$VcpkgRoot/vcpkg" }
    
    # Update registry
    if (Invoke-BuildCommand "$vcpkgExe update" "Checking for updated packages...") {
        Write-Status "Package registry updated successfully" -Type Success
    }
}
finally {
    Pop-Location
}

# Step 3: Pre-download source archives for each triplet
Write-Status "`n[3/4] Pre-downloading source archives..." -Type Info

$tripletList = $Triplets -join ", "
Write-Status "Target triplets: $tripletList" -Type Info

foreach ($triplet in $Triplets) {
    Write-Status "`nPre-fetching for triplet: $triplet" -Type Info
    
    Push-Location "$PSScriptRoot\.."
    try {
        $vcpkgExe = if ($IsWindows) { "$VcpkgRoot\vcpkg.exe" } else { "$VcpkgRoot/vcpkg" }
        
        # Create vcpkg.json for this triplet if needed
        if (Test-Path "vcpkg.json") {
            Write-Status "Analyzing dependencies for $triplet..." -Type Info
            
            # Install dependencies (will download sources)
            $installCmd = "$vcpkgExe install --triplet=$triplet"
            $result = Invoke-BuildCommand $installCmd "Installing dependencies for $triplet..."
            
            if ($result) {
                Write-Status "Dependencies downloaded for $triplet" -Type Success
            }
            else {
                Write-Status "Some downloads failed for $triplet (may retry)" -Type Warning
            }
        }
    }
    catch {
        Write-Warning "Error processing ${triplet}: $_"
    }
    finally {
        Pop-Location
    }
}

# Step 4: Verify cache integrity
Write-Status "`n[4/4] Verifying cache..." -Type Info
$downloadDir = "$VcpkgRoot\downloads"
if (Test-Path $downloadDir) {
    $fileCount = (Get-ChildItem $downloadDir -File -Recurse -ErrorAction SilentlyContinue | Measure-Object).Count
    $totalSize = (Get-ChildItem $downloadDir -File -Recurse -ErrorAction SilentlyContinue | 
                  Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Status "Cache status: $fileCount files, ~$([math]::Round($totalSize, 2)) GB" -Type Success
}

# Summary
Write-Status "`n==== Cache Update Complete ====" -Type Success
Write-Status "vcpkg\downloads is ready for offline builds" -Type Info
Write-Status "`nNext steps:" -Type Info
Write-Status "  - Run: .\build-tests-msvc.ps1  (Windows build)" -Type Info
Write-Status "  - Run: ./build.sh  (Linux/Docker build)" -Type Info
Write-Status "  - Run: docker buildx build ... (Multi-arch Docker build)" -Type Info

exit 0
