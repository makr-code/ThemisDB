<#
.SYNOPSIS
    Build vcpkg packages for all platforms and configurations
    
.DESCRIPTION
    Kompiliert vcpkg-Pakete für:
    - x64-windows (MSVC) - Debug & Release
    - x64-linux (WSL) - Debug & Release
    
    Die Pakete werden strukturiert abgelegt für Docker-Mounting.

.PARAMETER Platform
    Plattform: windows, linux, oder all (default: all)

.PARAMETER Configuration
    Build-Konfiguration: debug, release, oder all (default: release)

.PARAMETER Edition
    ThemisDB Edition: COMMUNITY, MINIMAL, ENTERPRISE, HYPERSCALER (default: COMMUNITY)

.PARAMETER SkipWindows
    Überspringe Windows-Builds

.PARAMETER SkipLinux
    Überspringe Linux-Builds (WSL)

.EXAMPLE
    .\build-vcpkg-packages.ps1 -Platform all -Configuration release
    .\build-vcpkg-packages.ps1 -Platform linux -Configuration all
    .\build-vcpkg-packages.ps1 -Edition COMMUNITY -SkipWindows
#>

param(
    [Parameter()]
    [ValidateSet('windows', 'linux', 'all')]
    [string]$Platform = 'all',
    
    [Parameter()]
    [ValidateSet('debug', 'release', 'all')]
    [string]$Configuration = 'release',
    
    [Parameter()]
    [ValidateSet('COMMUNITY', 'MINIMAL', 'ENTERPRISE', 'HYPERSCALER')]
    [string]$Edition = 'COMMUNITY',
    
    [Parameter()]
    [switch]$SkipWindows,
    
    [Parameter()]
    [switch]$SkipLinux,
    
    [Parameter()]
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# =============================================================================
# Configuration
# =============================================================================

$rootDir = $PSScriptRoot
$vcpkgRoot = Join-Path $rootDir "vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"

# Strukturierte Package-Verzeichnisse
$packageStore = Join-Path $rootDir "vcpkg_packages"
$windowsDebugDir = Join-Path $packageStore "x64-windows\debug"
$windowsReleaseDir = Join-Path $packageStore "x64-windows\release"
$linuxDebugDir = Join-Path $packageStore "x64-linux\debug"
$linuxReleaseDir = Join-Path $packageStore "x64-linux\release"

# vcpkg Manifest basierend auf Edition
$vcpkgManifest = Join-Path $rootDir "docker\vcpkg-$($Edition.ToLower()).json"

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host "`n$('='*80)" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "$('='*80)`n" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Text)
    Write-Host ">>> $Text" -ForegroundColor Green
}

function Write-Error-Message {
    param([string]$Text)
    Write-Host "ERROR: $Text" -ForegroundColor Red
}

function Ensure-Directory {
    param([string]$Path)
    if (!(Test-Path $Path)) {
        New-Item -Path $Path -ItemType Directory -Force | Out-Null
        Write-Step "Created directory: $Path"
    }
}

# =============================================================================
# Validation
# =============================================================================

Write-Header "vcpkg Multi-Platform Package Builder"

Write-Step "Validating environment..."

if (!(Test-Path $vcpkgExe)) {
    Write-Error-Message "vcpkg.exe not found at: $vcpkgExe"
    Write-Host "Run: git clone https://github.com/microsoft/vcpkg.git"
    Write-Host "     cd vcpkg && .\bootstrap-vcpkg.bat"
    exit 1
}

if (!(Test-Path $vcpkgManifest)) {
    Write-Error-Message "vcpkg manifest not found: $vcpkgManifest"
    Write-Host "Available editions:"
    Get-ChildItem (Join-Path $rootDir "docker") -Filter "vcpkg-*.json" | ForEach-Object { Write-Host "  - $($_.BaseName -replace 'vcpkg-','')" }
    exit 1
}

# WSL Check für Linux-Builds
if (($Platform -eq 'linux' -or $Platform -eq 'all') -and !$SkipLinux) {
    Write-Step "Checking WSL availability..."
    try {
        $wslVersion = wsl --version 2>&1
        if ($LASTEXITCODE -ne 0) { throw "WSL not available" }
        Write-Host "  ✓ WSL available" -ForegroundColor Green
        
        $distros = wsl --list --quiet
        if (!$distros -or $distros.Count -eq 0) {
            throw "No WSL distributions installed"
        }
        Write-Host "  ✓ WSL distributions: $($distros -join ', ')" -ForegroundColor Green
    }
    catch {
        Write-Error-Message "WSL required for Linux builds but not available: $_"
        Write-Host "Install WSL: wsl --install"
        exit 1
    }
}

# =============================================================================
# Package Store Setup
# =============================================================================

Write-Header "Setting up package store structure"

Ensure-Directory $packageStore
Ensure-Directory $windowsDebugDir
Ensure-Directory $windowsReleaseDir
Ensure-Directory $linuxDebugDir
Ensure-Directory $linuxReleaseDir

Write-Host @"

Package Store Structure:
$packageStore/
  x64-windows/
    debug/       → MSVC Debug builds
    release/     → MSVC Release builds
  x64-linux/
    debug/       → WSL/GCC Debug builds
    release/     → WSL/GCC Release builds

"@ -ForegroundColor Cyan

# =============================================================================
# Build Windows Packages (MSVC)
# =============================================================================

function Build-WindowsPackages {
    param(
        [string]$Config  # 'debug' oder 'release'
    )
    
    $triplet = "x64-windows"
    $manifestDir = Split-Path $vcpkgManifest -Parent
    $outputDir = if ($Config -eq 'debug') { $windowsDebugDir } else { $windowsReleaseDir }
    
    Write-Header "Building Windows packages ($Config)"
    
    Write-Step "Triplet: $triplet"
    Write-Step "Manifest: $vcpkgManifest"
    Write-Step "Output: $outputDir"
    
    # Visual Studio Developer Environment aktivieren
    Write-Step "Activating Visual Studio environment..."
    $vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    if (!(Test-Path $vsDevCmd)) {
        $vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    }
    if (!(Test-Path $vsDevCmd)) {
        Write-Error-Message "Visual Studio 2022 not found"
        exit 1
    }
    
    # vcpkg install mit Manifest-Mode
    $buildType = if ($Config -eq 'debug') { 'debug' } else { 'release' }
    
    Write-Step "Installing packages..."
    $installCmd = @"
call "$vsDevCmd" -arch=x64 >NUL 2>&1 && ^
cd /d "$rootDir" && ^
"$vcpkgExe" install --triplet=$triplet --x-install-root="$outputDir" --x-manifest-root="$manifestDir" --x-buildtrees-root="$rootDir\vcpkg\buildtrees" --x-packages-root="$rootDir\vcpkg\packages" --x-downloads-root="$rootDir\vcpkg\downloads"
"@
    
    cmd /c $installCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Message "Windows build failed for $Config"
        exit 1
    }
    
    # Größe anzeigen
    $size = (Get-ChildItem $outputDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Host "`n✓ Windows $Config packages built successfully" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($size, 2)) GB" -ForegroundColor Green
}

# =============================================================================
# Build Linux Packages (WSL)
# =============================================================================

function Build-LinuxPackages {
    param(
        [string]$Config  # 'debug' oder 'release'
    )
    
    $triplet = "x64-linux"
    $manifestDir = Split-Path $vcpkgManifest -Parent
    $outputDir = if ($Config -eq 'debug') { $linuxDebugDir } else { $linuxReleaseDir }
    
    Write-Header "Building Linux packages via WSL ($Config)"
    
    Write-Step "Triplet: $triplet"
    Write-Step "Manifest: $vcpkgManifest"
    Write-Step "Output: $outputDir"
    
    # Pfade für WSL konvertieren (C:\VCC\themis → /mnt/c/VCC/themis)
    $wslRootDir = $rootDir -replace '^([A-Z]):', { '/mnt/' + $_.Groups[1].Value.ToLower() } -replace '\\', '/'
    $wslVcpkgExe = "$wslRootDir/vcpkg/vcpkg"
    $wslManifestDir = $manifestDir -replace '^([A-Z]):', { '/mnt/' + $_.Groups[1].Value.ToLower() } -replace '\\', '/'
    $wslOutputDir = $outputDir -replace '^([A-Z]):', { '/mnt/' + $_.Groups[1].Value.ToLower() } -replace '\\', '/'
    $wslDownloadsDir = "$wslRootDir/vcpkg/downloads"
    $wslBuildtreesDir = "$wslRootDir/vcpkg/buildtrees"
    $wslPackagesDir = "$wslRootDir/vcpkg/packages"
    
    Write-Step "Installing required build tools in WSL..."
    
    # Build-Tools in WSL installieren
    $prepareCmd = @"
sudo apt-get update && \
sudo apt-get install -y build-essential cmake ninja-build curl zip unzip tar pkg-config git autoconf automake libtool
"@
    
    wsl bash -c $prepareCmd
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Message "Failed to install build tools in WSL"
        exit 1
    }
    
    # vcpkg bootstrappen falls nötig
    Write-Step "Ensuring vcpkg is bootstrapped in WSL..."
    wsl bash -c "cd $wslRootDir/vcpkg && if [ ! -f vcpkg ]; then ./bootstrap-vcpkg.sh; fi"
    
    # vcpkg install
    Write-Step "Installing packages in WSL..."
    $installCmd = @"
cd $wslRootDir && \
$wslVcpkgExe install \
  --triplet=$triplet \
  --x-install-root="$wslOutputDir" \
  --x-manifest-root="$wslManifestDir" \
  --x-buildtrees-root="$wslBuildtreesDir" \
  --x-packages-root="$wslPackagesDir" \
  --x-downloads-root="$wslDownloadsDir"
"@
    
    wsl bash -c $installCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Message "Linux build failed for $Config"
        exit 1
    }
    
    # Größe anzeigen
    $size = (Get-ChildItem $outputDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Host "`n✓ Linux $config packages built successfully" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($size, 2)) GB" -ForegroundColor Green
}

# =============================================================================
# Main Execution
# =============================================================================

$startTime = Get-Date

try {
    # Determine which builds to run
    $buildWindows = ($Platform -eq 'windows' -or $Platform -eq 'all') -and !$SkipWindows
    $buildLinux = ($Platform -eq 'linux' -or $Platform -eq 'all') -and !$SkipLinux
    $buildDebug = $Configuration -eq 'debug' -or $Configuration -eq 'all'
    $buildRelease = $Configuration -eq 'release' -or $Configuration -eq 'all'
    
    Write-Host @"

Build Plan:
  Platform:      $Platform
  Configuration: $Configuration
  Edition:       $Edition
  
  Windows Debug:   $(if ($buildWindows -and $buildDebug) { '✓' } else { '✗' })
  Windows Release: $(if ($buildWindows -and $buildRelease) { '✓' } else { '✗' })
  Linux Debug:     $(if ($buildLinux -and $buildDebug) { '✓' } else { '✗' })
  Linux Release:   $(if ($buildLinux -and $buildRelease) { '✓' } else { '✗' })

"@ -ForegroundColor Yellow
    
    Read-Host "Press Enter to continue or Ctrl+C to abort"
    
    # Execute builds
    if ($buildWindows) {
        if ($buildDebug) { Build-WindowsPackages -Config 'debug' }
        if ($buildRelease) { Build-WindowsPackages -Config 'release' }
    }
    
    if ($buildLinux) {
        if ($buildDebug) { Build-LinuxPackages -Config 'debug' }
        if ($buildRelease) { Build-LinuxPackages -Config 'release' }
    }
    
    # Summary
    Write-Header "Build Summary"
    
    $totalSize = 0
    $platforms = @(
        @{ Name = "Windows Debug"; Path = $windowsDebugDir },
        @{ Name = "Windows Release"; Path = $windowsReleaseDir },
        @{ Name = "Linux Debug"; Path = $linuxDebugDir },
        @{ Name = "Linux Release"; Path = $linuxReleaseDir }
    )
    
    foreach ($platform in $platforms) {
        if (Test-Path $platform.Path) {
            $size = (Get-ChildItem $platform.Path -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
            $totalSize += $size
            Write-Host "$($platform.Name): $([math]::Round($size, 2)) GB" -ForegroundColor Cyan
        }
    }
    
    Write-Host "`nTotal package store size: $([math]::Round($totalSize, 2)) GB" -ForegroundColor Green
    
    $elapsed = (Get-Date) - $startTime
    Write-Host "Build time: $($elapsed.ToString('hh\:mm\:ss'))" -ForegroundColor Green
    
    Write-Host "`n✓ All builds completed successfully!" -ForegroundColor Green
    Write-Host "`nNext steps:" -ForegroundColor Yellow
    Write-Host "  1. Use .\docker-build-with-prebuilt-packages.ps1 to build Docker image with mounted packages"
    Write-Host "  2. Packages are ready for Docker mounting from: $packageStore"
}
catch {
    Write-Error-Message "Build failed: $_"
    Write-Host $_.ScriptStackTrace -ForegroundColor Red
    exit 1
}
