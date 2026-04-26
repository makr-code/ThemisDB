# build.ps1 - ThemisDB Docker multiarch build with caching
# Usage:
#   .\build.ps1 -Edition community -Platform multiarch
#   .\build.ps1 -Edition minimal -Platform amd64
#   .\build.ps1 -All

param(
    [ValidateSet("community", "enterprise", "minimal", "hyperscaler", "debug")]
    [string]$Edition = "community",
    
    [ValidateSet("amd64", "arm64", "multiarch")]
    [string]$Platform = "multiarch",
    
    [switch]$All,
    [switch]$NoPush,
    [switch]$NoCache,
    [switch]$Help
)

if ($Help) {
    Write-Host @"
ThemisDB Docker Build Script

USAGE:
  .\build.ps1 [OPTIONS]

OPTIONS:
  -Edition     Edition to build: community, enterprise, minimal, hyperscaler, debug (default: community)
  -Platform    Target platform: amd64, arm64, multiarch (default: multiarch)
  -All         Build all editions (overrides -Edition)
  -NoPush      Build only, don't push to registry
  -NoCache     Disable build cache (slower, guarantees fresh build)
  -Help        Show this help message

EXAMPLES:
  # Build community edition for multiple architectures with cache
  .\build.ps1 -Edition community

  # Build all editions
  .\build.ps1 -All

  # Debug build (single platform, fastest iteration)
  .\build.ps1 -Edition debug

  # Force rebuild without cache
  .\build.ps1 -Edition minimal -NoCache
"@
    exit 0
}

$ErrorActionPreference = "Stop"

# Validate buildx builder exists
$builder = if ($Platform -eq "multiarch") { "themis-multiarch" } else { "default" }
$builderList = docker buildx ls 2>&1 | Out-String
if ($builderList -notmatch $builder) {
    Write-Host "ERROR: Buildx builder '$builder' not found" -ForegroundColor Red
    exit 1
}

# Determine targets
$targets = if ($All) {
    @("themisdb-community", "themisdb-enterprise", "themisdb-minimal", "themisdb-hyperscaler")
} else {
    @("themisdb-$Edition")
}

$platformArg = switch ($Platform) {
    "amd64" { "--set '*.platforms=linux/amd64'" }
    "arm64" { "--set '*.platforms=linux/arm64'" }
    "multiarch" { "" }
}

$cacheArg = if ($NoCache) { "--no-cache" } else { "" }
$pushArg = if ($NoPush) { "" } else { "--push" } # Note: local output in bake.hcl prevents push by default

Write-Host "ThemisDB Build Configuration" -ForegroundColor Cyan
Write-Host "============================="
Write-Host "Builder:  $builder"
Write-Host "Targets:  $($targets -join ', ')"
Write-Host "Platform: $Platform"
Write-Host "Cache:    $(if ($NoCache) { 'DISABLED' } else { 'enabled (local)' })"
Write-Host ""

foreach ($target in $targets) {
    Write-Host "Building: $target" -ForegroundColor Green
    
    $cmd = "docker buildx bake -f docker-bake.hcl --builder=$builder $cacheArg $platformArg $target"
    
    Write-Host "CMD: $cmd" -ForegroundColor DarkGray
    Invoke-Expression $cmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Build failed for $target" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "Build completed successfully!" -ForegroundColor Green
if (-not $NoPush) {
    Write-Host "Images ready for push" -ForegroundColor Cyan
}
