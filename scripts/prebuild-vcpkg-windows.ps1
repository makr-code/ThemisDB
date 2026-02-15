# =============================================================================
# ThemisDB vcpkg Prebuild Cache Generator for Windows (MSVC)
# =============================================================================
# Purpose: Generate prebuilt vcpkg packages for x64-windows in both debug and
#          release configurations, ready to be mounted in Docker containers
#
# Usage:
#   .\scripts\prebuild-vcpkg-windows.ps1 [-BuildType {debug|release|both}]
#
# Output: prebuilt-cache\x64-windows\{debug,release}\vcpkg_installed\
# =============================================================================

[CmdletBinding()]
param(
    [Parameter(Position=0)]
    [ValidateSet('debug', 'release', 'both')]
    [string]$BuildType = 'release',
    
    [Parameter()]
    [string]$VcpkgRoot = '',
    
    [Parameter()]
    [string]$Triplet = 'x64-windows',
    
    [Parameter()]
    [string]$Edition = 'COMMUNITY',
    
    [Parameter()]
    [switch]$Help
)

$ErrorActionPreference = 'Stop'

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] " -NoNewline -ForegroundColor Blue
    Write-Host $Message
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] " -NoNewline -ForegroundColor Green
    Write-Host $Message
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] " -NoNewline -ForegroundColor Yellow
    Write-Host $Message
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] " -NoNewline -ForegroundColor Red
    Write-Host $Message
}

function Show-Usage {
    @"
ThemisDB vcpkg Prebuild Cache Generator for Windows

Usage: .\scripts\prebuild-vcpkg-windows.ps1 [-BuildType <type>] [-Options]

Parameters:
  -BuildType    debug|release|both    Build configuration (default: release)
  -VcpkgRoot    <path>                Path to vcpkg (default: .\vcpkg)
  -Triplet      <triplet>             vcpkg triplet (default: x64-windows)
  -Edition      <edition>             ThemisDB edition (default: COMMUNITY)
  -Help                               Show this help

Examples:
  # Generate release packages only (fastest)
  .\scripts\prebuild-vcpkg-windows.ps1 -BuildType release

  # Generate both debug and release packages
  .\scripts\prebuild-vcpkg-windows.ps1 -BuildType both

  # Generate with specific edition
  .\scripts\prebuild-vcpkg-windows.ps1 -Edition MINIMAL -BuildType release

Output:
  prebuilt-cache\x64-windows\debug\vcpkg_installed\
  prebuilt-cache\x64-windows\release\vcpkg_installed\

"@
}

if ($Help) {
    Show-Usage
    exit 0
}

# Determine script and project paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$PrebuiltCacheDir = Join-Path $ProjectRoot "prebuilt-cache"

# Determine vcpkg root
if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
    if ($env:VCPKG_ROOT) {
        $VcpkgRoot = $env:VCPKG_ROOT
    }
    elseif (Test-Path (Join-Path $ProjectRoot "vcpkg")) {
        $VcpkgRoot = Join-Path $ProjectRoot "vcpkg"
    }
    else {
        Write-Error "VCPKG_ROOT not set and .\vcpkg not found"
        Write-Info "Please set VCPKG_ROOT or clone vcpkg to .\vcpkg"
        exit 1
    }
}

$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    Write-Error "vcpkg.exe not found at: $VcpkgExe"
    Write-Info "Please bootstrap vcpkg: cd $VcpkgRoot && .\bootstrap-vcpkg.bat"
    exit 1
}

Write-Info "================================================================"
Write-Info "ThemisDB vcpkg Prebuild Cache Generator"
Write-Info "================================================================"
Write-Info "Project Root:    $ProjectRoot"
Write-Info "vcpkg Root:      $VcpkgRoot"
Write-Info "Target Triplet:  $Triplet"
Write-Info "Build Type:      $BuildType"
Write-Info "Edition:         $Edition"
Write-Info "Output:          $PrebuiltCacheDir\$Triplet\"
Write-Info "================================================================"

function Build-Packages {
    param(
        [string]$Config  # debug or release
    )
    
    $CmakeBuildType = if ($Config -eq 'debug') { 'Debug' } else { 'Release' }
    
    Write-Info ""
    Write-Info "Building $Config packages for $Triplet..."
    Write-Info "----------------------------------------------------------------"
    
    # Create temporary build directory
    $TempBuildDir = Join-Path $ProjectRoot "build-prebuild-$Config"
    New-Item -ItemType Directory -Path $TempBuildDir -Force | Out-Null
    
    # Create output directory
    $OutputDir = Join-Path $PrebuiltCacheDir "$Triplet\$Config"
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    
    # Configure CMake to install packages
    Write-Info "Configuring CMake for $Config build..."
    Push-Location $TempBuildDir
    
    try {
        $cmakeArgs = @(
            $ProjectRoot,
            '-G', 'Ninja',
            "-DCMAKE_BUILD_TYPE=$CmakeBuildType",
            "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake",
            "-DVCPKG_TARGET_TRIPLET=$Triplet",
            "-DVCPKG_INSTALLED_DIR=$TempBuildDir\vcpkg_installed",
            "-DTHEMIS_EDITION=$Edition",
            '-DTHEMIS_BUILD_TESTS=OFF',
            '-DTHEMIS_BUILD_BENCHMARKS=OFF'
        )
        
        & cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed for $Config"
        }
        
        Write-Info "Installing vcpkg packages..."
        
        # vcpkg install will populate vcpkg_installed directory
        $vcpkgArgs = @(
            'install',
            "--triplet=$Triplet",
            "--x-manifest-root=$ProjectRoot",
            "--x-install-root=$TempBuildDir\vcpkg_installed",
            '--clean-after-build'
        )
        
        & $VcpkgExe @vcpkgArgs
        if ($LASTEXITCODE -ne 0) {
            throw "vcpkg install failed for $Config"
        }
        
        # Copy installed packages to output directory
        Write-Info "Copying packages to prebuilt cache..."
        $VcpkgInstalledSrc = Join-Path $TempBuildDir "vcpkg_installed"
        $VcpkgInstalledDst = Join-Path $OutputDir "vcpkg_installed"
        
        if (Test-Path $VcpkgInstalledSrc) {
            if (Test-Path $VcpkgInstalledDst) {
                Remove-Item -Path $VcpkgInstalledDst -Recurse -Force
            }
            Copy-Item -Path $VcpkgInstalledSrc -Destination $VcpkgInstalledDst -Recurse
            
            # Calculate size and library count
            $Size = (Get-ChildItem $VcpkgInstalledDst -Recurse | Measure-Object -Property Length -Sum).Sum
            $SizeMB = [math]::Round($Size / 1MB, 2)
            $LibCount = (Get-ChildItem "$VcpkgInstalledDst\$Triplet\lib" -Filter *.lib -Recurse -ErrorAction SilentlyContinue).Count
            
            Write-Success "$Config packages ready!"
            Write-Info "  Location: $VcpkgInstalledDst"
            Write-Info "  Size: $SizeMB MB"
            Write-Info "  Libraries: $LibCount"
        }
        else {
            throw "vcpkg_installed directory not found after build"
        }
        
        # Clean up temporary build directory (keep vcpkg_installed)
        Write-Info "Cleaning up temporary files..."
        Get-ChildItem $TempBuildDir -Exclude vcpkg_installed | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
        
        Write-Success "$Config prebuild complete!"
    }
    catch {
        Write-Error "Failed to build $Config packages: $_"
        Pop-Location
        throw
    }
    finally {
        Pop-Location
    }
}

# Main execution
$StartTime = Get-Date

try {
    if ($BuildType -in @('both', 'release')) {
        Build-Packages -Config 'release'
    }
    
    if ($BuildType -in @('both', 'debug')) {
        Build-Packages -Config 'debug'
    }
    
    $EndTime = Get-Date
    $Duration = ($EndTime - $StartTime).TotalSeconds
    
    Write-Info ""
    Write-Info "================================================================"
    Write-Success "Prebuild generation complete!"
    Write-Info "================================================================"
    Write-Info "Duration: $([math]::Round($Duration, 2))s"
    Write-Info "Output: $PrebuiltCacheDir\$Triplet\"
    Write-Info ""
    Write-Info "Next steps:"
    Write-Info "  1. Use in Docker (from WSL):"
    Write-Info "     docker build --build-context prebuilt=/mnt/c/path/to/prebuilt-cache/$Triplet/release ..."
    Write-Info ""
    Write-Info "  2. Or copy to Linux for docker-compose:"
    Write-Info "     # From WSL:"
    Write-Info "     cp -r /mnt/c/path/to/prebuilt-cache ./prebuilt-cache"
    Write-Info ""
    Write-Info "  3. Share with team (archive):"
    Write-Info "     Compress-Archive -Path `"$PrebuiltCacheDir\$Triplet\release\vcpkg_installed`" ``"
    Write-Info "                      -DestinationPath vcpkg-$Triplet-release.zip"
    Write-Info "================================================================"
}
catch {
    Write-Error "Prebuild generation failed: $_"
    exit 1
}
