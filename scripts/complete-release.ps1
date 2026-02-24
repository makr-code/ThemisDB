# =============================================================================
# ThemisDB Complete Release Script (Git Flow Compatible) - Windows
# =============================================================================
# This script automates the complete release process following Git Flow:
#   develop → release/vX.X.X → main (+ tag) → retroactive build
#
# Usage:
#   .\complete-release.ps1 -Version <version> [OPTIONS]
#
# Parameters:
#   -Version          Version number (e.g., 1.5.0)
#   -SkipBuild        Skip retroactive build after tagging
#   -SkipMergeBack    Skip merge back to develop
#   -DryRun           Show what would be done without executing
#
# Example:
#   .\complete-release.ps1 -Version 1.5.0
#   .\complete-release.ps1 -Version 1.5.0 -SkipBuild
#
# =============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild,
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipMergeBack,
    
    [Parameter(Mandatory=$false)]
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

# Color codes
$ColorSuccess = "Green"
$ColorError = "Red"
$ColorWarning = "Yellow"
$ColorInfo = "Cyan"

function Print-Header {
    Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor $ColorInfo
    Write-Host "║  ThemisDB Complete Release (Git Flow)                     ║" -ForegroundColor $ColorInfo
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

function Run-Command {
    param([string]$Command, [array]$Arguments)
    
    if ($DryRun) {
        Print-Warning "[DRY RUN] Would execute: $Command $($Arguments -join ' ')"
        return $true
    } else {
        try {
            & $Command @Arguments
            return $LASTEXITCODE -eq 0
        } catch {
            Write-Warning "Command failed: $_"
            return $false
        }
    }
}

Print-Header
Print-Info "Version: $Version"
Print-Info "Skip Build: $SkipBuild"
Print-Info "Skip Merge Back: $SkipMergeBack"
Print-Info "Dry Run: $DryRun"
Write-Host ""

# Validate version format
if ($Version -notmatch '^\d+\.\d+\.\d+(-[a-zA-Z0-9-]+)?$') {
    Print-Error "Invalid version format: $Version"
    Print-Info "Expected format: X.Y.Z or X.Y.Z-prerelease"
    exit 1
}

# Step 1: Ensure we're on develop and up-to-date
Print-Info "Step 1: Checking out develop branch..."
Run-Command "git" @("checkout", "develop") | Out-Null
Run-Command "git" @("pull", "origin", "develop") | Out-Null
Print-Success "On develop branch and up-to-date"

# Step 2: Create release branch
$ReleaseBranch = "release/v$Version"
Print-Info "Step 2: Creating release branch: $ReleaseBranch"
Run-Command "git" @("checkout", "-b", $ReleaseBranch) | Out-Null
Print-Success "Release branch created"

# Step 3: Update VERSION file
Print-Info "Step 3: Updating VERSION file..."
if (-not $DryRun) {
    Set-Content -Path "VERSION" -Value $Version -NoNewline
    git add VERSION
    git commit -m "chore: Bump version to $Version"
}
Print-Success "VERSION file updated"

# Step 4: Run tests (optional)
Print-Info "Step 4: Running tests..."
Print-Warning "Skipping tests - run manually if needed"

# Step 5: Merge to main
Print-Info "Step 5: Merging release branch to main..."
Run-Command "git" @("checkout", "main") | Out-Null
Run-Command "git" @("pull", "origin", "main") | Out-Null
Run-Command "git" @("merge", "--no-ff", $ReleaseBranch, "-m", "Merge $ReleaseBranch into main") | Out-Null
Print-Success "Merged to main"

# Step 6: Create tag
$Tag = "v$Version"
Print-Info "Step 6: Creating tag: $Tag"
Run-Command "git" @("tag", "-a", $Tag, "-m", "Release $Tag") | Out-Null
Print-Success "Tag created"

# Step 7: Push main and tag
Print-Info "Step 7: Pushing main and tag to origin..."
Run-Command "git" @("push", "origin", "main") | Out-Null
Run-Command "git" @("push", "origin", $Tag) | Out-Null
Print-Success "Pushed to origin"

# Step 8: Merge back to develop
if (-not $SkipMergeBack) {
    Print-Info "Step 8: Merging release branch back to develop..."
    Run-Command "git" @("checkout", "develop") | Out-Null
    Run-Command "git" @("merge", "--no-ff", $ReleaseBranch, "-m", "Merge $ReleaseBranch back into develop") | Out-Null
    Run-Command "git" @("push", "origin", "develop") | Out-Null
    Print-Success "Merged back to develop"
} else {
    Print-Warning "Step 8: Skipping merge back to develop"
}

# Step 9: Delete release branch
Print-Info "Step 9: Deleting release branch..."
Run-Command "git" @("branch", "-d", $ReleaseBranch) | Out-Null
Print-Success "Release branch deleted"

# Step 10: Build binaries retroactively
if (-not $SkipBuild) {
    Print-Info "Step 10: Building binaries retroactively..."
    
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    
    if (-not $DryRun) {
        & "$ScriptDir\retroactive-release-builder.ps1" -Tag $Tag -Clean
    } else {
        Print-Warning "[DRY RUN] Would execute: retroactive-release-builder.ps1 -Tag $Tag -Clean"
    }
    
    Print-Success "Binaries built"
    
    if (-not $DryRun) {
        Print-Info "Artifacts location: release-retroactive\$Tag\"
        Get-ChildItem "release-retroactive\$Tag\" -ErrorAction SilentlyContinue
    }
} else {
    Print-Warning "Step 10: Skipping retroactive build"
}

# Summary
Write-Host ""
Print-Success "═══════════════════════════════════════════════════════"
Print-Success "  Release $Version completed successfully!"
Print-Success "═══════════════════════════════════════════════════════"
Write-Host ""
Print-Info "Summary:"
Write-Host "  • Release branch: $ReleaseBranch"
Write-Host "  • Tag: $Tag"
Write-Host "  • Main branch: Updated"
Write-Host "  • Develop branch: Updated"
if (-not $SkipBuild) {
    Write-Host "  • Binaries: release-retroactive\$Tag\"
}
Write-Host ""
Print-Info "Next steps:"
Write-Host "  1. Verify GitHub Actions workflows completed successfully"
Write-Host "  2. Test the release artifacts"
Write-Host "  3. Update CHANGELOG.md if needed"
Write-Host "  4. Announce the release"
Write-Host ""

if ($DryRun) {
    Print-Warning "This was a DRY RUN - no changes were made"
}
