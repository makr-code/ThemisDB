#!/usr/bin/env powershell
<#
.SYNOPSIS
    CMake PREFIX_PATH Fix für ThemisDB FAISS + gRPC Integration
    
.DESCRIPTION
    Behebt automatisch CMake find_package() Probleme mit FAISS und gRPC
    Setzt CMAKE_PREFIX_PATH und explizite DIR-Variablen
    
.PARAMETER Action
    'diagnose' - Überprüft aktuellen Status
    'build'    - Führt CMake Configure mit Fixes durch
    'clean'    - Löscht build-msvc und konfiguriert neu
    
.EXAMPLE
    .\fix-cmake-prefix-path.ps1 -Action diagnose
    .\fix-cmake-prefix-path.ps1 -Action build
    .\fix-cmake-prefix-path.ps1 -Action clean
#>

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('diagnose', 'build', 'clean')]
    [string]$Action,
    
    [string]$ProjectRoot = "C:\VCC\themis",
    [string]$BuildType = "Release",
    [switch]$EnableGPU = $false,
    [switch]$EnableLLM = $false
)

$ErrorActionPreference = "Stop"

# Farbe-Output
function Write-Success {
    param([string]$Message)
    Write-Host "✅ $Message" -ForegroundColor Green
}

function Write-Error_ {
    param([string]$Message)
    Write-Host "❌ $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ️  $Message" -ForegroundColor Cyan
}

function Write-Warning_ {
    param([string]$Message)
    Write-Host "⚠️  $Message" -ForegroundColor Yellow
}

# Diagnose-Funktion
function Diagnose {
    Write-Info "=== DIAGNOSE: FAISS + gRPC CMAKE CONFIG STATUS ==="
    
    $VCPKG_INSTALLED = "$ProjectRoot\vcpkg_installed\x64-windows"
    
    # Check FAISS
    Write-Info ""
    Write-Info "[1/5] Checking FAISS..."
    if (Test-Path "$VCPKG_INSTALLED\share\faiss\faiss-config.cmake") {
        Write-Success "FAISS config found: $VCPKG_INSTALLED\share\faiss\faiss-config.cmake"
    } else {
        Write-Error_ "FAISS config NOT found: $VCPKG_INSTALLED\share\faiss\faiss-config.cmake"
    }
    
    if (Test-Path "$VCPKG_INSTALLED\lib\faiss.lib") {
        Write-Success "FAISS library found: $VCPKG_INSTALLED\lib\faiss.lib"
    } else {
        Write-Error_ "FAISS library NOT found: $VCPKG_INSTALLED\lib\faiss.lib"
    }
    
    # Check gRPC
    Write-Info ""
    Write-Info "[2/5] Checking gRPC..."
    if (Test-Path "$VCPKG_INSTALLED\share\grpc\gRPCConfig.cmake") {
        Write-Success "gRPC config found: $VCPKG_INSTALLED\share\grpc\gRPCConfig.cmake"
    } else {
        Write-Error_ "gRPC config NOT found: $VCPKG_INSTALLED\share\grpc\gRPCConfig.cmake"
    }
    
    if (Test-Path "$VCPKG_INSTALLED\lib\grpc.lib") {
        Write-Success "gRPC library found: $VCPKG_INSTALLED\lib\grpc.lib"
    } else {
        Write-Error_ "gRPC library NOT found: $VCPKG_INSTALLED\lib\grpc.lib"
    }
    
    # Check Protobuf (abhängig von gRPC)
    Write-Info ""
    Write-Info "[3/5] Checking Protobuf (gRPC dependency)..."
    if (Test-Path "$VCPKG_INSTALLED\share\protobuf\protobufConfig.cmake") {
        Write-Success "Protobuf config found"
    } else {
        Write-Error_ "Protobuf config NOT found"
    }
    
    # Check CMAKE_CACHE
    Write-Info ""
    Write-Info "[4/5] Checking CMake Cache..."
    if (Test-Path "$ProjectRoot\build-msvc\CMakeCache.txt") {
        $faiss_cache = Select-String -Path "$ProjectRoot\build-msvc\CMakeCache.txt" -Pattern "^faiss_DIR" -ErrorAction SilentlyContinue
        $grpc_cache = Select-String -Path "$ProjectRoot\build-msvc\CMakeCache.txt" -Pattern "^gRPC_DIR" -ErrorAction SilentlyContinue
        
        if ($faiss_cache) {
            Write-Success "CMake Cache has faiss_DIR: $($faiss_cache.Line)"
        } else {
            Write-Warning_ "CMake Cache missing faiss_DIR entry"
        }
        
        if ($grpc_cache) {
            Write-Success "CMake Cache has gRPC_DIR: $($grpc_cache.Line)"
        } else {
            Write-Warning_ "CMake Cache missing gRPC_DIR entry"
        }
    } else {
        Write-Info "No CMake Cache found (need to configure first)"
    }
    
    # Check VCPKG_ROOT
    Write-Info ""
    Write-Info "[5/5] Checking environment..."
    if ($env:VCPKG_ROOT) {
        Write-Success "VCPKG_ROOT is set: $env:VCPKG_ROOT"
    } else {
        Write-Warning_ "VCPKG_ROOT not set in environment"
    }
    
    Write-Info ""
    Write-Info "Diagnosis complete. Next: .\fix-cmake-prefix-path.ps1 -Action build"
}

# Build-Funktion mit Fixes
function BuildWithFixes {
    Write-Info "=== CMAKE CONFIGURE WITH PREFIX_PATH FIX ==="
    
    $VCPKG_INSTALLED = "$ProjectRoot\vcpkg_installed\x64-windows"
    $BUILD_DIR = "$ProjectRoot\build-msvc"
    
    # Create build dir if not exists
    if (-not (Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
        Write-Success "Created build directory: $BUILD_DIR"
    }
    
    # Set CMAKE_PREFIX_PATH
    $CMAKE_PREFIX_PATH = "$VCPKG_INSTALLED;$VCPKG_INSTALLED\share"
    
    Write-Info ""
    Write-Info "CMake Configuration Parameters:"
    Write-Info "  Generator: Visual Studio 17 2022"
    Write-Info "  Architecture: x64"
    Write-Info "  Build Type: $BuildType"
    Write-Info "  CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
    Write-Info "  faiss_DIR: $VCPKG_INSTALLED\share\faiss"
    Write-Info "  gRPC_DIR: $VCPKG_INSTALLED\share\grpc"
    if ($EnableGPU) { Write-Info "  THEMIS_ENABLE_GPU: ON" }
    if ($EnableLLM) { Write-Info "  THEMIS_ENABLE_LLM: ON" }
    
    # CMake configure command
    Write-Info ""
    Write-Info "Running CMake configure..."
    
    Push-Location $ProjectRoot
    
    try {
        $cmake_args = @(
            "-S", ".",
            "-B", $BUILD_DIR,
            "-G", "Visual Studio 17 2022",
            "-A", "x64",
            "-DCMAKE_TOOLCHAIN_FILE=$ProjectRoot\vcpkg\scripts\buildsystems\vcpkg.cmake",
            "-DVCPKG_TARGET_TRIPLET=x64-windows",
            "-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH",
            "-Dfaiss_DIR=$VCPKG_INSTALLED\share\faiss",
            "-DgRPC_DIR=$VCPKG_INSTALLED\share\grpc",
            "-DCMAKE_BUILD_TYPE=$BuildType"
        )
        
        if ($EnableGPU) {
            $cmake_args += "-DTHEMIS_ENABLE_GPU=ON"
        }
        
        if ($EnableLLM) {
            $cmake_args += "-DTHEMIS_ENABLE_LLM=ON"
        }
        
        & cmake @cmake_args
        
        if ($LASTEXITCODE -eq 0) {
            Write-Success "CMake configuration successful!"
            Write-Info ""
            Write-Info "Next steps:"
            Write-Info "  1. Build: cmake --build $BUILD_DIR --config $BuildType --parallel 8"
            Write-Info "  2. Test:  cmake --build $BUILD_DIR --config $BuildType --target RUN_TESTS"
        } else {
            Write-Error_ "CMake configuration failed with exit code $LASTEXITCODE"
            exit 1
        }
    }
    finally {
        Pop-Location
    }
}

# Clean and rebuild
function CleanAndBuild {
    Write-Warning_ "=== CLEAN BUILD ==="
    Write-Warning_ "This will DELETE: $ProjectRoot\build-msvc"
    Write-Host "Continue? (Y/N): " -NoNewline
    $response = Read-Host
    
    if ($response -ne "Y") {
        Write-Info "Cancelled"
        return
    }
    
    Write-Info "Removing old build directory..."
    if (Test-Path "$ProjectRoot\build-msvc") {
        Remove-Item -Recurse -Force "$ProjectRoot\build-msvc"
        Write-Success "Removed: $ProjectRoot\build-msvc"
    }
    
    Write-Info ""
    BuildWithFixes
}

# Main dispatch
switch ($Action) {
    'diagnose' {
        Diagnose
    }
    'build' {
        BuildWithFixes
    }
    'clean' {
        CleanAndBuild
    }
}

Write-Info ""
Write-Info "Script completed"
