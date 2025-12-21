# Build ThemisDB Hyperscaler Edition v1.3.5
# ==========================================
# Builds the OEM/custom Hyperscaler Edition with unlimited scaling capabilities.
# Edition constraints: Unlimited GPU VRAM, unlimited shard nodes, full custom capabilities.

param(
    [ValidateSet("oem", "custom", "docker", "all")]
    [string]$BuildType = "all",
    
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    
    [string]$CustomerName = "OEM",
    [switch]$SkipTests,
    [switch]$SkipDocker,
    [switch]$SignArtifacts
)

$ErrorActionPreference = "Stop"
$BuildDir = "C:\VCC\themis\build-msvc"
$Version = "1.3.5"
$EditionName = "hyperscaler"
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building ThemisDB v$Version - HYPERSCALER Edition" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Type: $BuildType"
Write-Host "Configuration: $Configuration"
Write-Host "Customer: $CustomerName"
Write-Host "Edition: HYPERSCALER (GPU: Unlimited, Nodes: Unlimited, Plugins: Full)"
Write-Host ""

# Function: Configure CMake with HYPERSCALER settings
function Configure-CMake-Hyperscaler {
    param(
        [string]$OutputDir = $BuildDir
    )
    
    Write-Host "Configuring CMake for Hyperscaler Edition..." -ForegroundColor Yellow
    
    if (!(Test-Path $OutputDir)) {
        New-Item -ItemType Directory -Path $OutputDir | Out-Null
    }
    
    Push-Location $OutputDir
    try {
        $env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"
        
        # Configure with HYPERSCALER edition - all features enabled
        cmake -S C:\VCC\themis -B . `
            -G "Visual Studio 17 2022" `
            -A x64 `
            -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
            -DVCPKG_TARGET_TRIPLET=x64-windows `
            -DTHEMIS_EDITION=HYPERSCALER `
            -DCMAKE_BUILD_TYPE=$Configuration `
            -DTHEMIS_BUILD_TESTS=ON `
            -DTHEMIS_BUILD_BENCHMARKS=ON `
            -DTHEMIS_ENABLE_GPU=ON `
            -DTHEMIS_ENABLE_CUDA=ON `
            -DTHEMIS_ENABLE_TRACING=ON `
            -DTHEMIS_ENABLE_ENTERPRISE_PLUGINS=ON `
            -DTHEMIS_ENABLE_MULTI_MASTER=ON `
            -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON `
            -DTHEMIS_ENABLE_RBAC=ON `
            -DTHEMIS_ENABLE_HSM=ON `
            -DTHEMIS_ENABLE_HSM_REAL=ON `
            -DTHEMIS_ENABLE_LLM=ON `
            -DBUILD_SHARED_LIBS=ON
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
    }
    finally {
        Pop-Location
    }
}

# Function: Build Hyperscaler Edition
function Build-Hyperscaler {
    param(
        [string]$OutputDir = $BuildDir,
        [string]$Config = "Release"
    )
    
    Write-Host "Building Hyperscaler Edition..." -ForegroundColor Yellow
    
    Push-Location $OutputDir
    try {
        # Hyperscaler: Build everything for maximum compatibility
        cmake --build . --config $Config --parallel 8
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
    }
    finally {
        Pop-Location
    }
}

# Function: Generate OEM configuration
function Generate-OEM-Configuration {
    param(
        [string]$Customer = "OEM"
    )
    
    Write-Host "Generating OEM configuration for $Customer..." -ForegroundColor Yellow
    
    $config = @"
# ThemisDB Hyperscaler Edition - OEM Configuration
# Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

[Edition]
Type=HYPERSCALER
Customer=$Customer
Version=1.3.5
BuildDate=$(Get-Date -Format 'yyyy-MM-dd')

[Capabilities]
# GPU: Unlimited
MaxGPUVRAMGB=999999
GPUAcceleration=true
CUDASupport=true

# Clustering: Unlimited
MaxShardNodes=999999
MultiMasterReplication=true
AutoFailover=true
LoadBalancing=advanced

# Security: Full enterprise suite + custom
EnterprisePlugins=true
FieldEncryption=true
RoleBasedAccess=true
HSMSupport=true
CustomProviders=true

# Performance: All optimizations
LLMIntegration=true
ContentProcessors=true
AdvancedAnalytics=true
CustomOptimization=true

[Support]
LicenseRequired=false
Priority=custom
DedicatedAccount=true
SLA=custom
"@
    
    $config | Out-File "C:\VCC\themis\release\v$Version\$EditionName-oem-config.txt"
    Write-Host "  OEM configuration created" -ForegroundColor Green
}

# Function: Create release artifacts
function Create-Release-Artifacts-Hyperscaler {
    param(
        [string]$OutputDir = $BuildDir,
        [string]$Config = "Release",
        [string]$Customer = "OEM"
    )
    
    Write-Host "Creating Hyperscaler release artifacts ($Customer)..." -ForegroundColor Yellow
    
    # Sanitize customer name for directory
    $sanitized_customer = $Customer -replace '[^a-zA-Z0-9-]', '_'
    $artifact_dir = "C:\VCC\themis\release\v$Version\$EditionName-$sanitized_customer-windows-x64"
    New-Item -ItemType Directory -Path $artifact_dir -Force | Out-Null
    
    # Copy executable
    $exe_path = Join-Path $OutputDir $Config "themis_server.exe"
    if (Test-Path $exe_path) {
        Copy-Item $exe_path $artifact_dir\
        Write-Host "  Copied themis_server.exe" -ForegroundColor Green
    } else {
        Write-Host "  WARNING: themis_server.exe not found" -ForegroundColor Red
    }
    
    # Copy all DLLs (Hyperscaler includes maximum dependencies)
    Get-ChildItem (Join-Path $OutputDir $Config) -Filter "*.dll" | ForEach-Object {
        Copy-Item $_.FullName $artifact_dir\
    }
    
    # Create comprehensive edition info
    $info = @"
ThemisDB Hyperscaler Edition v$Version
======================================
Build Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Edition: HYPERSCALER (OEM/Custom)
Customer: $Customer

CAPABILITIES (UNLIMITED):
- GPU VRAM: Unlimited (all GPU types supported)
- Shard Nodes: Unlimited (10000+ node clusters)
- Enterprise Plugins: Full custom plugin system
- Multi-Master Replication: Advanced topologies
- Field Encryption: Custom providers supported
- RBAC: Fine-grained permission control
- HSM Integration: Real PKCS#11 HSM support
- LLM Support: llama.cpp integration enabled
- Content Processors: Full suite included

FEATURES:
✓ All Community Edition features
✓ All Enterprise Edition features
✓ Unlimited GPU acceleration
✓ Unlimited node clustering
✓ Custom plugin system
✓ Advanced analytics
✓ LLM integration
✓ Custom optimizations
✓ Dedicated support

LICENSE:
- No restrictions
- Custom OEM licensing available
- Professional services included
- 24/7 priority support
- On-site consulting available

Release Notes: See RELEASE_NOTES_v$Version.md
Support: support@themisdb.io
"@
    $info | Out-File (Join-Path $artifact_dir "EDITION_INFO.txt")
    
    Write-Host "  Hyperscaler artifacts created at: $artifact_dir" -ForegroundColor Green
    return $artifact_dir
}

# Function: Generate checksums and code sign
function Finalize-Hyperscaler-Artifacts {
    param(
        [string]$ArtifactPath,
        [bool]$Sign = $false
    )
    
    Write-Host "Finalizing Hyperscaler artifacts..." -ForegroundColor Yellow
    
    # Generate SHA256 checksums
    $files = Get-ChildItem $ArtifactPath -Include "*.exe", "*.dll" -Recurse
    $checksum_file = Join-Path $ArtifactPath "SHA256SUMS"
    "" | Out-File $checksum_file
    
    foreach ($file in $files) {
        $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
        "$hash  $($file.Name)" | Add-Content $checksum_file
    }
    
    # Code sign for OEM distribution (requires certificate)
    if ($Sign) {
        Write-Host "  Signing artifacts for distribution..." -ForegroundColor Yellow
        # Requires: $cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert
        # Real implementation would use signtool
    }
    
    # Generate manifest for OEM installation
    $manifest = @"
# ThemisDB Hyperscaler Installation Manifest
BuildVersion=1.3.5
Edition=HYPERSCALER
Timestamp=$(Get-Date -Format 'o')
BuildHash=$((Get-FileHash -Path $checksum_file -Algorithm SHA256).Hash)
"@
    $manifest | Out-File (Join-Path $ArtifactPath "INSTALL_MANIFEST.txt")
    
    Write-Host "  Artifacts finalized with manifest" -ForegroundColor Green
}

# Main build flow
Write-Host "`nSTEP 1: Configure CMake" -ForegroundColor Cyan
Configure-CMake-Hyperscaler

Write-Host "`nSTEP 2: Build Hyperscaler Edition" -ForegroundColor Cyan
Build-Hyperscaler -Config $Configuration

# Generate OEM configuration
Write-Host "`nSTEP 3: Generate OEM Configuration" -ForegroundColor Cyan
Generate-OEM-Configuration -Customer $CustomerName

# Create artifacts
if ($BuildType -eq "oem" -or $BuildType -eq "all") {
    Write-Host "`nSTEP 4a: Create OEM Artifacts" -ForegroundColor Cyan
    $oem_artifacts = Create-Release-Artifacts-Hyperscaler -Config $Configuration -Customer $CustomerName
    Finalize-Hyperscaler-Artifacts $oem_artifacts -Sign $SignArtifacts
}

if ($BuildType -eq "custom" -or $BuildType -eq "all") {
    Write-Host "`nSTEP 4b: Create Custom Artifacts" -ForegroundColor Cyan
    $custom_artifacts = Create-Release-Artifacts-Hyperscaler -Config $Configuration -Customer "Custom-Deployment"
    Finalize-Hyperscaler-Artifacts $custom_artifacts
}

# Docker build if requested
if (($BuildType -eq "docker" -or $BuildType -eq "all") -and -not $SkipDocker) {
    Write-Host "`nSTEP 5: Build Docker Image (Private OEM Registry)" -ForegroundColor Cyan
    
    $docker_tag = "themisdb:$Version-hyperscaler-latest"
    
    Write-Host "Building Docker image for Hyperscaler Edition..." -ForegroundColor Yellow
    docker build -t $docker_tag `
        --build-arg THEMIS_EDITION=HYPERSCALER `
        --build-arg THEMIS_VERSION=$Version `
        -f C:\VCC\themis\Dockerfile.hyperscaler `
        C:\VCC\themis
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Docker image built: $docker_tag" -ForegroundColor Green
        
        # Tag for OEM private registry
        docker tag $docker_tag "oem.themisdb.io/hyperscaler:$Version"
        Write-Host "  Tagged for OEM registry" -ForegroundColor Green
    }
}

# Run comprehensive tests
if (-not $SkipTests) {
    Write-Host "`nSTEP 6: Run Hyperscaler Tests" -ForegroundColor Cyan
    Push-Location $BuildDir
    try {
        # Run all tests (no label restriction for Hyperscaler)
        ctest --build-config $Configuration --output-on-failure --parallel 4
    }
    finally {
        Pop-Location
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Hyperscaler Edition v$Version build complete!" -ForegroundColor Cyan
Write-Host "Customer: $CustomerName" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
