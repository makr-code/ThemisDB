# =============================================================================
# ThemisDB Retroactive Release Builder (PowerShell)
# =============================================================================
# Purpose: Extract source code at specific version tags and build/package
#          binaries retroactively for all past releases (Windows).
#
# Usage:
#   .\retroactive-release-builder.ps1 [OPTIONS]
#
# Parameters:
#   -Tag              Build specific tag (e.g., v1.3.4)
#   -AllTags          Build all version tags
#   -ListTags         List available version tags
#   -Platform         Target platform (windows|all) [default: windows]
#   -OutputDir        Output directory for artifacts [default: .\release-retroactive]
#   -SkipBuild        Skip build, only package existing binaries
#   -Clean            Clean build directories before building
#   -Help             Show this help message
#
# Examples:
#   # Build specific tag for Windows
#   .\retroactive-release-builder.ps1 -Tag v1.3.4
#
#   # Build all tags
#   .\retroactive-release-builder.ps1 -AllTags
#
#   # List available tags
#   .\retroactive-release-builder.ps1 -ListTags
#
# =============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$Tag = "",
    
    [Parameter(Mandatory=$false)]
    [switch]$AllTags,
    
    [Parameter(Mandatory=$false)]
    [switch]$ListTags,
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("windows", "all")]
    [string]$Platform = "windows",
    
    [Parameter(Mandatory=$false)]
    [string]$OutputDir = ".\release-retroactive",
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild,
    
    [Parameter(Mandatory=$false)]
    [switch]$Clean,
    
    [Parameter(Mandatory=$false)]
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# Color codes
$ColorSuccess = "Green"
$ColorError = "Red"
$ColorWarning = "Yellow"
$ColorInfo = "Cyan"

# Script paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

# =============================================================================
# Utility Functions
# =============================================================================

function Print-Header {
    Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor $ColorInfo
    Write-Host "║  ThemisDB Retroactive Release Builder (Windows)            ║" -ForegroundColor $ColorInfo
    Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor $ColorInfo
    Write-Host ""
}

function Print-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor $ColorSuccess
}

function Print-Error {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor $ColorError
}

function Print-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor $ColorWarning
}

function Print-Info {
    param([string]$Message)
    Write-Host "ℹ $Message" -ForegroundColor $ColorInfo
}

function Show-Help {
    @"
ThemisDB Retroactive Release Builder (Windows)

Usage: .\retroactive-release-builder.ps1 [OPTIONS]

Parameters:
    -Tag              Build specific tag (e.g., v1.3.4)
    -AllTags          Build all version tags
    -ListTags         List available version tags
    -Platform         Target platform (windows|all) [default: windows]
    -OutputDir        Output directory for artifacts [default: .\release-retroactive]
    -SkipBuild        Skip build, only package existing binaries
    -Clean            Clean build directories before building
    -Help             Show this help message

Examples:
    # Build specific tag for Windows
    .\retroactive-release-builder.ps1 -Tag v1.3.4

    # Build all tags
    .\retroactive-release-builder.ps1 -AllTags

    # List available tags
    .\retroactive-release-builder.ps1 -ListTags

"@
}

# =============================================================================
# Git Operations
# =============================================================================

function Get-VersionTags {
    $tags = git tag -l "v*" 2>$null | Sort-Object { [Version]($_ -replace '^v', '') }
    return $tags
}

function Show-VersionTags {
    Print-Info "Available version tags:"
    $tags = Get-VersionTags
    foreach ($tag in $tags) {
        Write-Host "  $tag"
    }
}

function Checkout-Tag {
    param([string]$TagName)
    
    Print-Info "Checking out tag: $TagName"
    
    # Stash any local changes
    $status = git status --porcelain
    if ($status) {
        Print-Warning "Stashing local changes..."
        git stash push -m "Auto-stash before retroactive build"
    }
    
    # Checkout the tag
    try {
        git checkout $TagName 2>&1 | Out-Null
        
        # Update submodules if any
        if (Test-Path ".gitmodules") {
            Print-Info "Updating submodules..."
            git submodule update --init --recursive 2>&1 | Out-Null
        }
        
        Print-Success "Checked out tag: $TagName"
        return $true
    }
    catch {
        Print-Error "Failed to checkout tag: $TagName"
        return $false
    }
}

function Restore-OriginalBranch {
    Print-Info "Restoring original branch..."
    
    try {
        # Try to return to main or develop branch
        $branches = @("main", "develop", "master")
        foreach ($branch in $branches) {
            $exists = git rev-parse --verify $branch 2>$null
            if ($exists) {
                git checkout $branch 2>&1 | Out-Null
                Print-Success "Returned to branch: $branch"
                break
            }
        }
        
        # Pop stashed changes if any
        $stashList = git stash list
        if ($stashList -match "Auto-stash before retroactive build") {
            Print-Info "Restoring stashed changes..."
            git stash pop 2>&1 | Out-Null
        }
        
        return $true
    }
    catch {
        Print-Warning "Could not fully restore original state"
        return $false
    }
}

# =============================================================================
# Build Functions
# =============================================================================

function Build-Windows {
    param([string]$TagName)
    
    $version = $TagName -replace '^v', ''
    $buildDir = "build-retroactive-msvc"
    
    Print-Info "Building Windows binaries for $TagName..."
    
    # Clean if requested
    if ($Clean) {
        Print-Info "Cleaning build directory..."
        if (Test-Path $buildDir) {
            Remove-Item -Recurse -Force $buildDir
        }
    }
    
    # Create build directory
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    
    # Detect Visual Studio
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath
        Print-Info "Found Visual Studio at: $vsPath"
    }
    
    # Configure CMake
    Print-Info "Configuring CMake..."
    try {
        cmake -S . -B $buildDir -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_BUILD_TYPE=Release `
            -DTHEMIS_BUILD_TESTS=OFF `
            -DTHEMIS_BUILD_BENCHMARKS=OFF `
            -DTHEMIS_ENABLE_TRACING=ON
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
    }
    catch {
        Print-Error "CMake configuration failed: $_"
        return $false
    }
    
    # Build
    Print-Info "Building (using $env:NUMBER_OF_PROCESSORS cores)..."
    try {
        cmake --build $buildDir --config Release -j $env:NUMBER_OF_PROCESSORS
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
    }
    catch {
        Print-Error "Build failed: $_"
        return $false
    }
    
    # Package
    Print-Info "Generating packages..."
    try {
        Push-Location $buildDir
        cpack -G "ZIP" -C Release
        if ($LASTEXITCODE -ne 0) {
            Print-Warning "Package generation had errors, but continuing..."
        }
        Pop-Location
    }
    catch {
        Print-Warning "Package generation failed: $_"
        Pop-Location
    }
    
    Print-Success "Windows build completed for $TagName"
    return $true
}

function Build-Platform {
    param(
        [string]$TagName,
        [string]$TargetPlatform
    )
    
    switch ($TargetPlatform) {
        "windows" {
            return Build-Windows $TagName
        }
        "all" {
            $result = Build-Windows $TagName
            # Could add cross-compilation for other platforms here
            return $result
        }
        default {
            Print-Error "Unknown platform: $TargetPlatform"
            return $false
        }
    }
}

# =============================================================================
# Packaging Functions
# =============================================================================

function Package-Artifacts {
    param(
        [string]$TagName,
        [string]$TargetPlatform
    )
    
    $version = $TagName -replace '^v', ''
    Print-Info "Packaging artifacts for $TagName ($TargetPlatform)..."
    
    $tagOutputDir = Join-Path $OutputDir $TagName
    New-Item -ItemType Directory -Force -Path $tagOutputDir | Out-Null
    
    # Find and copy build artifacts
    $buildDir = "build-retroactive-msvc"
    
    if (Test-Path $buildDir) {
        # Copy packages
        Get-ChildItem -Path $buildDir -Filter "*.zip" -Recurse | 
            Where-Object { $_.Directory.Name -eq $buildDir -or $_.Directory.Parent.Name -eq $buildDir } |
            Copy-Item -Destination $tagOutputDir -Force
        
        # Generate SHA256 checksums
        Print-Info "Generating SHA256 checksums..."
        $checksumFile = Join-Path $tagOutputDir "SHA256SUMS.txt"
        Get-ChildItem -Path $tagOutputDir -File | 
            Where-Object { $_.Name -ne "SHA256SUMS.txt" } | 
            ForEach-Object {
                $hash = Get-FileHash -Path $_.FullName -Algorithm SHA256
                "$($hash.Hash.ToLower())  $($_.Name)" | Add-Content -Path $checksumFile
            }
        
        # Create release notes
        Create-ReleaseNotes $TagName $tagOutputDir
        
        Print-Success "Artifacts packaged in: $tagOutputDir"
        
        # List generated files
        Print-Info "Generated files:"
        Get-ChildItem -Path $tagOutputDir | ForEach-Object {
            Write-Host "  $($_.Name) ($([math]::Round($_.Length / 1MB, 2)) MB)"
        }
    }
    else {
        Print-Warning "Build directory not found: $buildDir"
    }
}

function Create-ReleaseNotes {
    param(
        [string]$TagName,
        [string]$OutputPath
    )
    
    $version = $TagName -replace '^v', ''
    $notesFile = Join-Path $OutputPath "RELEASE_NOTES_${TagName}.md"
    
    $commit = git rev-parse HEAD
    $date = Get-Date -Format "yyyy-MM-dd HH:mm:ss UTC"
    
    $content = @"
# ThemisDB ${TagName} - Retroactive Build

**Version:** ${version}  
**Build Date:** ${date}  
**Build Type:** Retroactive Release Build  
**Build Host:** $env:COMPUTERNAME  
**Build Platform:** Windows

## Build Information

This release was built retroactively from the source code at tag ${TagName}.

- **Git Tag:** ${TagName}
- **Git Commit:** ${commit}
- **Build Date:** ${date}

## Artifacts

"@
    
    # List artifacts
    Get-ChildItem -Path $OutputPath -File | 
        Where-Object { $_.Name -ne "RELEASE_NOTES_${TagName}.md" } |
        ForEach-Object {
            $content += "`n- ``$($_.Name)``"
        }
    
    $content += @"


## Installation

### Windows

1. Download the ZIP package
2. Extract to your desired location
3. Add the bin directory to your PATH
4. Run ``themis_server.exe --help``

### From Source

``````powershell
# Clone repository and checkout tag
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git checkout ${TagName}

# Build
cmake -B build-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-msvc --config Release
``````

## Checksums

See ``SHA256SUMS.txt`` for file checksums.

---

For more information, visit the [ThemisDB GitHub repository](https://github.com/makr-code/ThemisDB).
"@
    
    $content | Out-File -FilePath $notesFile -Encoding utf8
    Print-Success "Release notes created: $notesFile"
}

# =============================================================================
# Main Build Process
# =============================================================================

function Build-Tag {
    param([string]$TagName)
    
    Print-Header
    Print-Info "Processing tag: $TagName"
    Print-Info "Platform: $Platform"
    Print-Info "Output directory: $OutputDir"
    Write-Host ""
    
    # Checkout the tag
    if (-not (Checkout-Tag $TagName)) {
        Print-Error "Failed to checkout tag: $TagName"
        return $false
    }
    
    # Build unless skipped
    if (-not $SkipBuild) {
        if (-not (Build-Platform $TagName $Platform)) {
            Print-Error "Build failed for tag: $TagName"
            Restore-OriginalBranch
            return $false
        }
    }
    
    # Package artifacts
    Package-Artifacts $TagName $Platform
    
    # Restore original branch
    Restore-OriginalBranch
    
    Print-Success "Completed processing tag: $TagName"
    Write-Host ""
    
    return $true
}

# =============================================================================
# Main
# =============================================================================

function Main {
    Set-Location $RepoRoot
    
    # Show help if requested
    if ($Help) {
        Show-Help
        return
    }
    
    # List tags if requested
    if ($ListTags) {
        Show-VersionTags
        return
    }
    
    # Validate options
    if (-not $AllTags -and -not $Tag) {
        Print-Error "Either -Tag or -AllTags must be specified"
        Show-Help
        return
    }
    
    # Create output directory
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
    
    # Build specific tag or all tags
    if ($Tag) {
        Build-Tag $Tag
    }
    elseif ($AllTags) {
        $tags = Get-VersionTags
        
        if ($tags.Count -eq 0) {
            Print-Warning "No version tags found"
            return
        }
        
        Print-Info "Found $($tags.Count) version tags"
        Write-Host ""
        
        foreach ($t in $tags) {
            if (-not (Build-Tag $t)) {
                Print-Error "Failed to build tag: $t"
                Print-Warning "Continuing with next tag..."
                Write-Host ""
            }
        }
    }
    
    Print-Success "All builds completed!"
    Print-Info "Artifacts available in: $OutputDir"
}

# Run main function
Main
