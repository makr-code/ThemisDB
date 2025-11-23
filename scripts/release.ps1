# ThemisDB Release Automation Script
# Handles versioning, tagging, building, and deployment
#
# Usage:
#   .\scripts\release.ps1 -Version 0.2.0 -DryRun
#   .\scripts\release.ps1 -Version 0.2.0 -Platforms windows,linux,docker
#   .\scripts\release.ps1 -Version 0.2.0 -PushDocker -PushGit

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [string[]]$Platforms = @('windows', 'linux', 'docker'),
    
    [switch]$DryRun,
    [switch]$PushGit,
    [switch]$PushDocker,
    [switch]$SkipBuild,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$DockerUser = "themisdb"
$GitHubOrg = "makr-code"
$GitHubRepo = "ThemisDB"

Write-Host "=== ThemisDB Release Automation ===" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Yellow
Write-Host "Platforms: $($Platforms -join ', ')" -ForegroundColor Gray
if ($DryRun) {
    Write-Host "DRY RUN MODE - No changes will be made" -ForegroundColor Magenta
}
Write-Host ""

# ============================================================================
# Validation
# ============================================================================

function Test-Version {
    param([string]$Ver)
    
    if ($Ver -notmatch '^\d+\.\d+\.\d+(-[\w\.]+)?(\+[\w\.]+)?$') {
        Write-Host "Invalid version format: $Ver" -ForegroundColor Red
        Write-Host "Expected: MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]" -ForegroundColor Yellow
        Write-Host "Examples: 0.1.0, 0.2.0-beta.1, 1.0.0+qnap" -ForegroundColor Gray
        exit 1
    }
    
    Write-Host "✓ Version format valid" -ForegroundColor Green
}

function Test-GitStatus {
    $status = git status --porcelain
    if ($status -and -not $Force) {
        Write-Host "Working directory has uncommitted changes!" -ForegroundColor Red
        Write-Host "Please commit or stash changes, or use -Force to ignore" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "✓ Git status clean" -ForegroundColor Green
}

function Test-GitTag {
    param([string]$Tag)
    
    $exists = git tag -l $Tag
    if ($exists -and -not $Force) {
        Write-Host "Tag '$Tag' already exists!" -ForegroundColor Red
        Write-Host "Use -Force to overwrite (not recommended)" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "✓ Git tag available" -ForegroundColor Green
}

# ============================================================================
# Version Update
# ============================================================================

function Update-CMakeVersion {
    param([string]$Ver)
    
    $cmakeFile = Join-Path $ProjectRoot "CMakeLists.txt"
    
    if (-not (Test-Path $cmakeFile)) {
        Write-Host "CMakeLists.txt not found!" -ForegroundColor Red
        exit 1
    }
    
    $content = Get-Content $cmakeFile -Raw
    $newContent = $content -replace 'project\(ThemisDB VERSION \d+\.\d+\.\d+\)', "project(ThemisDB VERSION $Ver)"
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would update CMakeLists.txt version to $Ver" -ForegroundColor Magenta
    } else {
        Set-Content -Path $cmakeFile -Value $newContent -NoNewline
        Write-Host "✓ Updated CMakeLists.txt to version $Ver" -ForegroundColor Green
    }
}

function Update-Changelog {
    param([string]$Ver)
    
    $changelogFile = Join-Path $ProjectRoot "CHANGELOG.md"
    
    if (-not (Test-Path $changelogFile)) {
        Write-Host "WARNING: CHANGELOG.md not found, skipping..." -ForegroundColor Yellow
        return
    }
    
    $date = Get-Date -Format "yyyy-MM-dd"
    $newEntry = @"
## [$Ver] - $date

### Added
- [TODO: Add release notes]

### Changed
- [TODO: Add changes]

### Fixed
- [TODO: Add fixes]

"@
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would add changelog entry for $Ver" -ForegroundColor Magenta
    } else {
        $content = Get-Content $changelogFile -Raw
        $updatedContent = $content -replace '(# Changelog\s+)', "`$1`n$newEntry`n"
        Set-Content -Path $changelogFile -Value $updatedContent -NoNewline
        Write-Host "✓ Updated CHANGELOG.md" -ForegroundColor Green
        Write-Host "  NOTE: Please fill in release notes manually!" -ForegroundColor Yellow
    }
}

# ============================================================================
# Build
# ============================================================================

function Invoke-PlatformBuild {
    param([string]$Platform, [string]$Ver)
    
    Write-Host ""
    Write-Host "Building for $Platform..." -ForegroundColor Yellow
    
    $buildScript = Join-Path $ProjectRoot "build-unified.ps1"
    
    if (-not (Test-Path $buildScript)) {
        Write-Host "WARNING: build-unified.ps1 not found, using legacy scripts" -ForegroundColor Yellow
        return
    }
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would build: .\build-unified.ps1 -Platform $Platform -Config release" -ForegroundColor Magenta
        return
    }
    
    switch ($Platform) {
        'windows' {
            & $buildScript -Platform windows -Config release -Compiler msvc
        }
        'linux' {
            & $buildScript -Platform linux -Config release -Compiler clang
        }
        'docker' {
            & $buildScript -Platform docker -Tag "themisdb:$Ver"
        }
        'docker-qnap' {
            & $buildScript -Platform qnap -Tag "themisdb:$Ver-qnap" -Static
        }
        'arm64' {
            & $buildScript -Platform arm64 -Config release
        }
        default {
            Write-Host "Unknown platform: $Platform" -ForegroundColor Red
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed for $Platform!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✓ $Platform build successful" -ForegroundColor Green
}

# ============================================================================
# Git Operations
# ============================================================================

function Invoke-GitCommit {
    param([string]$Ver)
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would commit version bump" -ForegroundColor Magenta
        return
    }
    
    git add CMakeLists.txt CHANGELOG.md
    git commit -m "chore: Bump version to $Ver"
    
    Write-Host "✓ Version bump committed" -ForegroundColor Green
}

function Invoke-GitTag {
    param([string]$Ver)
    
    $tag = "v$Ver"
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would create tag: $tag" -ForegroundColor Magenta
        return
    }
    
    if ($Force) {
        git tag -f -a $tag -m "Release $Ver"
    } else {
        git tag -a $tag -m "Release $Ver"
    }
    
    Write-Host "✓ Git tag created: $tag" -ForegroundColor Green
}

function Invoke-GitPush {
    param([string]$Ver)
    
    $tag = "v$Ver"
    
    if ($DryRun) {
        Write-Host "[DRY RUN] Would push to GitHub" -ForegroundColor Magenta
        return
    }
    
    git push origin main
    git push origin $tag
    
    Write-Host "✓ Pushed to GitHub" -ForegroundColor Green
}

# ============================================================================
# Docker Operations
# ============================================================================

function Invoke-DockerPush {
    param([string]$Ver)
    
    $tags = @(
        "$DockerUser/themisdb:$Ver",
        "$DockerUser/themisdb:latest"
    )
    
    foreach ($tag in $tags) {
        if ($DryRun) {
            Write-Host "[DRY RUN] Would tag and push: $tag" -ForegroundColor Magenta
            continue
        }
        
        docker tag "themisdb:$Ver" $tag
        docker push $tag
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Failed to push $tag" -ForegroundColor Red
            exit 1
        }
        
        Write-Host "✓ Pushed $tag" -ForegroundColor Green
    }
}

# ============================================================================
# GitHub Release
# ============================================================================

function New-GitHubRelease {
    param([string]$Ver)
    
    Write-Host ""
    Write-Host "GitHub Release creation..." -ForegroundColor Yellow
    Write-Host "  Go to: https://github.com/$GitHubOrg/$GitHubRepo/releases/new" -ForegroundColor Gray
    Write-Host "  Tag: v$Ver" -ForegroundColor Gray
    Write-Host "  Title: ThemisDB $Ver" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Or use GitHub CLI:" -ForegroundColor Yellow
    Write-Host "  gh release create v$Ver --title 'ThemisDB $Ver' --notes-file CHANGELOG.md" -ForegroundColor Gray
}

# ============================================================================
# Main Execution
# ============================================================================

Write-Host "Step 1: Validation" -ForegroundColor Cyan
Write-Host "==================" -ForegroundColor Cyan
Test-Version -Ver $Version
Test-GitStatus
Test-GitTag -Tag "v$Version"

Write-Host ""
Write-Host "Step 2: Version Update" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan
Update-CMakeVersion -Ver $Version
Update-Changelog -Ver $Version

Write-Host ""
Write-Host "Step 3: Git Commit" -ForegroundColor Cyan
Write-Host "==================" -ForegroundColor Cyan
Invoke-GitCommit -Ver $Version
Invoke-GitTag -Ver $Version

if (-not $SkipBuild) {
    Write-Host ""
    Write-Host "Step 4: Build Artifacts" -ForegroundColor Cyan
    Write-Host "=======================" -ForegroundColor Cyan
    
    foreach ($platform in $Platforms) {
        Invoke-PlatformBuild -Platform $platform -Ver $Version
    }
}

if ($PushGit) {
    Write-Host ""
    Write-Host "Step 5: Push to GitHub" -ForegroundColor Cyan
    Write-Host "======================" -ForegroundColor Cyan
    Invoke-GitPush -Ver $Version
}

if ($PushDocker) {
    Write-Host ""
    Write-Host "Step 6: Push to Docker Hub" -ForegroundColor Cyan
    Write-Host "===========================" -ForegroundColor Cyan
    Invoke-DockerPush -Ver $Version
}

Write-Host ""
Write-Host "Step 7: GitHub Release" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan
New-GitHubRelease -Ver $Version

Write-Host ""
Write-Host "=== Release Process Complete ===" -ForegroundColor Green
Write-Host "Version: $Version" -ForegroundColor Cyan
Write-Host ""

if ($DryRun) {
    Write-Host "This was a DRY RUN - no actual changes were made" -ForegroundColor Magenta
    Write-Host "Run without -DryRun to execute for real" -ForegroundColor Yellow
}
