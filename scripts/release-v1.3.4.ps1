# ThemisDB v1.3.4 Release Automation Script
# Automatisiert: Build, Test, Docker, GitHub Push

param(
    [switch]$BuildOnly = $false,
    [switch]$SkipTests = $false,
    [switch]$SkipDocker = $false,
    [switch]$DryRun = $false,
    [string]$DockerRegistry = "YOUR_USERNAME/themis",
    [string]$GitRemote = "origin"
)

$ErrorActionPreference = "Stop"
$Version = "1.3.4"

Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB v$Version Release Automation         ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Helper functions
function Write-Step {
    param([string]$Message)
    Write-Host "► $Message" -ForegroundColor Green
}

function Write-Error-Step {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

# Step 1: Verify we're on main branch
Write-Step "Checking git branch..."
$branch = git rev-parse --abbrev-ref HEAD
if ($branch -ne "main") {
    Write-Error-Step "Not on main branch (current: $branch)"
    exit 1
}
Write-Success "On main branch"

# Step 2: Verify VERSION file
Write-Step "Verifying VERSION file..."
$versionContent = Get-Content "VERSION" -Raw
if ($versionContent.Trim() -ne $Version) {
    Write-Error-Step "VERSION file mismatch (expected: $Version, got: $($versionContent.Trim()))"
    exit 1
}
Write-Success "VERSION file correct ($Version)"

# Step 3: Build Release
Write-Step "Building MSVC Release..."
Push-Location "build-msvc"
cmake --build . --config Release --parallel 8 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Pop-Location
    Write-Error-Step "Build failed"
    exit 1
}
Pop-Location
Write-Success "Build completed"

if ($BuildOnly) {
    Write-Host ""
    Write-Host "Build-only mode - stopping here" -ForegroundColor Yellow
    exit 0
}

# Step 4: Run Tests
if (-not $SkipTests) {
    Write-Step "Running Release tests..."
    Push-Location "build-msvc"
    ctest -C Release -j 8 --output-on-failure -E NOT_BUILT 2>&1 | Out-Null
    $testResult = $LASTEXITCODE
    Pop-Location
    
    if ($testResult -ne 0) {
        Write-Error-Step "Tests failed"
        exit 1
    }
    Write-Success "All tests passed"
} else {
    Write-Host "► Skipping tests" -ForegroundColor Yellow
}

# Step 5: Git Operations
Write-Step "Checking git status..."
$gitStatus = git status --porcelain
if ($gitStatus) {
    Write-Error-Step "Uncommitted changes detected:"
    git status --short
    exit 1
}
Write-Success "Working directory clean"

# Check if tag exists
$tagExists = git tag -l "v$Version"
if ($tagExists) {
    Write-Host "► Tag v$Version already exists" -ForegroundColor Yellow
} else {
    Write-Step "Tag v$Version not found - please create it first"
    Write-Host "Run: git tag -a v$Version -m 'Release v$Version'" -ForegroundColor Cyan
    exit 1
}

# Step 6: Push to GitHub
if (-not $DryRun) {
    Write-Step "Pushing to GitHub..."
    git push $GitRemote main
    git push $GitRemote "v$Version"
    Write-Success "Pushed to GitHub"
} else {
    Write-Host "► DRY RUN: Would push to GitHub" -ForegroundColor Yellow
}

# Step 7: Docker Build
if (-not $SkipDocker) {
    Write-Step "Building Docker image..."
    
    if (-not $DryRun) {
        docker build -t "${DockerRegistry}:${Version}" -t "${DockerRegistry}:latest" .
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Step "Docker build failed"
            exit 1
        }
        Write-Success "Docker image built"
        
        # Step 8: Docker Push
        Write-Step "Pushing Docker image..."
        docker push "${DockerRegistry}:${Version}"
        docker push "${DockerRegistry}:latest"
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Step "Docker push failed"
            exit 1
        }
        Write-Success "Docker images pushed"
    } else {
        Write-Host "► DRY RUN: Would build and push Docker image" -ForegroundColor Yellow
    }
} else {
    Write-Host "► Skipping Docker build" -ForegroundColor Yellow
}

# Step 9: Summary
Write-Host ""
Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║        Release v$Version Completed! 🎉           ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Create GitHub Release at: https://github.com/YOUR_USERNAME/themis/releases/new" -ForegroundColor White
Write-Host "   - Tag: v$Version" -ForegroundColor White
Write-Host "   - Title: ThemisDB v$Version - Insert Performance Optimization" -ForegroundColor White
Write-Host "   - Copy description from GITHUB_RELEASE_GUIDE.md" -ForegroundColor White
Write-Host ""
Write-Host "2. Upload assets:" -ForegroundColor White
Write-Host "   - build-msvc/Release/themis_server.exe" -ForegroundColor White
Write-Host "   - RELEASE_NOTES_v$Version.md" -ForegroundColor White
Write-Host ""
Write-Host "3. Announce release on social media/community channels" -ForegroundColor White
Write-Host ""

if ($DryRun) {
    Write-Host "NOTE: This was a DRY RUN - no actual changes were made" -ForegroundColor Yellow
}
