# Docker Build Script for ThemisDB
# Creates a clean build context without vcpkg directory

param(
    [string]$Target = "runtime",
    [string]$Tag = "themisdb:latest",
    [string]$Edition = "COMMUNITY"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ThemisDB Docker Build (Clean Context)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Target:  $Target"
Write-Host "Tag:     $Tag"
Write-Host "Edition: $Edition"
Write-Host ""

$buildContext = ".docker-build-context"

try {
    # Create clean build context directory
    Write-Host "Creating clean build context..." -ForegroundColor Yellow
    if (Test-Path $buildContext) {
        Remove-Item $buildContext -Recurse -Force
    }
    New-Item -ItemType Directory -Path $buildContext | Out-Null
    
    # Copy only needed files
    Write-Host "Copying source files..." -ForegroundColor Yellow
    
    $filesToCopy = @(
        "Dockerfile",
        ".dockerignore",
        "CMakeLists.txt",
        "VERSION",
        "vcpkg-configuration.json"
    )
    
    $directoriesToCopy = @(
        "cmake",
        "include",
        "src",
        "proto",
        "internal",
        "docker",
        "llama.cpp"
    )
    
    foreach ($file in $filesToCopy) {
        if (Test-Path $file) {
            Copy-Item $file $buildContext -Force
        }
    }
    
    foreach ($dir in $directoriesToCopy) {
        if (Test-Path $dir) {
            Write-Host "  Copying $dir..." -ForegroundColor Gray
            
            if ($dir -eq "llama.cpp") {
                # Exclude large subdirectories from llama.cpp
                robocopy $dir "$buildContext\$dir" /E /XD build build-* models .git .github /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
            } else {
                Copy-Item $dir $buildContext -Recurse -Force
            }
        }
    }
    
    # Build Docker image from clean context
    Write-Host "`nStarting Docker build..." -ForegroundColor Green
    Push-Location $buildContext
    
    try {
        $buildArgs = @(
            "build",
            ".",
            "--target=$Target",
            "--tag=$Tag",
            "--build-arg", "THEMIS_EDITION=$Edition"
        )
        
        & docker @buildArgs
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`nBuild successful!" -ForegroundColor Green
            Write-Host "Image tagged as: $Tag"
        } else {
            Write-Host "`nBuild failed with exit code: $LASTEXITCODE" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    # Clean up build context
    if (Test-Path $buildContext) {
        Write-Host "`nCleaning up build context..." -ForegroundColor Yellow
        Remove-Item $buildContext -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`nDone!" -ForegroundColor Cyan
