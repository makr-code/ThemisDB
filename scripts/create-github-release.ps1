param(
    [string]$Version = "1.3.0",
    [string]$GitHubToken = $env:GITHUB_TOKEN,
    [switch]$SkipTag,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

Write-Host "=== ThemisDB v$Version GitHub Release ===" -ForegroundColor Cyan

$repo = "C:\VCC\themis"
Set-Location $repo

$releaseDir = Join-Path $repo "release"
$tag = "v$Version"

# Validate release artifacts exist
if (-not (Test-Path $releaseDir)) {
    Write-Error "Release directory not found: $releaseDir"
    Write-Host "Run first: .\scripts\package-release-v1.3.0.ps1" -ForegroundColor Yellow
    exit 1
}

$artifacts = @(
    "themisdb-v$Version-windows-x64.zip"
)

$missing = @()
foreach ($artifact in $artifacts) {
    $path = Join-Path $releaseDir $artifact
    if (-not (Test-Path $path)) {
        $missing += $artifact
    }
}

if ($missing.Count -gt 0) {
    Write-Error "Missing release artifacts:"
    $missing | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    Write-Host "`nBuild artifacts first with: .\scripts\package-release-v1.3.0.ps1" -ForegroundColor Yellow
    exit 1
}

Write-Host "`n=== Release Artifacts ===" -ForegroundColor Yellow
Get-ChildItem $releaseDir -File | ForEach-Object {
    $size = $_.Length / 1MB
    Write-Host "  ✓ $($_.Name) ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
}

# Create Git Tag
if (-not $SkipTag) {
    Write-Host "`n=== Git Tag ===" -ForegroundColor Yellow
    
    $tagExists = git tag -l $tag
    if ($tagExists) {
        Write-Warning "Tag $tag already exists locally"
        $response = Read-Host "Delete and recreate? (y/n)"
        if ($response -eq 'y') {
            if (-not $DryRun) {
                git tag -d $tag
                Write-Host "  Deleted local tag $tag" -ForegroundColor Cyan
            }
        } else {
            Write-Host "  Using existing tag $tag" -ForegroundColor Cyan
        }
    }
    
    if (-not $tagExists -or $response -eq 'y') {
        $tagMessage = "Release v$Version - Keep Your Own Llamas

Native LLM Integration with llama.cpp
- Embedded llama.cpp inference engine
- GPU acceleration (CUDA)
- GGUF model support (LLaMA 3, Mistral, Phi-3)
- Lazy loading with caching
- Multi-LoRA adapter support

Full release notes: RELEASE_NOTES_v$Version.md"

        if ($DryRun) {
            Write-Host "  [DRY RUN] Would create tag: $tag" -ForegroundColor Cyan
            Write-Host "  Message: $tagMessage" -ForegroundColor DarkGray
        } else {
            git tag -a $tag -m $tagMessage
            Write-Host "  ✓ Created tag: $tag" -ForegroundColor Green
        }
    }
}

# Push Git Tag
if (-not $SkipTag) {
    Write-Host "`n=== Push Git Tag ===" -ForegroundColor Yellow
    Write-Host "Tag $tag will be pushed to origin" -ForegroundColor Cyan
    
    if ($DryRun) {
        Write-Host "  [DRY RUN] Would push: git push origin $tag" -ForegroundColor Cyan
    } else {
        $response = Read-Host "Push tag to GitHub? (y/n)"
        if ($response -eq 'y') {
            git push origin $tag
            Write-Host "  ✓ Pushed tag to origin" -ForegroundColor Green
        } else {
            Write-Host "  Skipped push (manual: git push origin $tag)" -ForegroundColor Yellow
        }
    }
}

# GitHub Release
Write-Host "`n=== GitHub Release ===" -ForegroundColor Yellow

if (-not $GitHubToken) {
    Write-Warning "GITHUB_TOKEN not set - cannot create release via API"
    Write-Host "`nManual steps:" -ForegroundColor Yellow
    Write-Host "  1. Go to: https://github.com/makr-code/ThemisDB/releases/new" -ForegroundColor White
    Write-Host "  2. Select tag: $tag" -ForegroundColor White
    Write-Host "  3. Release title: ThemisDB v$Version - Keep Your Own Llamas" -ForegroundColor White
    Write-Host "  4. Copy content from: RELEASE_NOTES_v$Version.md" -ForegroundColor White
    Write-Host "  5. Upload artifacts from: $releaseDir" -ForegroundColor White
    Write-Host "  6. Publish release" -ForegroundColor White
    exit 0
}

# GitHub CLI (gh) fallback
$ghCliAvailable = Get-Command gh -ErrorAction SilentlyContinue
if (-not $ghCliAvailable) {
    Write-Warning "GitHub CLI (gh) not installed - falling back to manual instructions"
    Write-Host "`nInstall GitHub CLI: winget install GitHub.cli" -ForegroundColor Yellow
    Write-Host "Or create release manually: https://github.com/makr-code/ThemisDB/releases/new" -ForegroundColor Yellow
    exit 0
}

# Create release with gh CLI
Write-Host "Creating GitHub release with gh CLI..." -ForegroundColor Cyan

$releaseNotes = Get-Content "$repo\RELEASE_NOTES_v$Version.md" -Raw -ErrorAction SilentlyContinue
if (-not $releaseNotes) {
    $releaseNotes = "Release v$Version

See full documentation at: https://makr-code.github.io/ThemisDB/"
}

if ($DryRun) {
    Write-Host "  [DRY RUN] Would create release:" -ForegroundColor Cyan
    Write-Host "  Tag: $tag" -ForegroundColor DarkGray
    Write-Host "  Title: ThemisDB v$Version - Keep Your Own Llamas" -ForegroundColor DarkGray
    Write-Host "  Artifacts: $($artifacts -join ', ')" -ForegroundColor DarkGray
} else {
    # Build gh release create command
    $ghArgs = @(
        "release", "create", $tag,
        "--title", "ThemisDB v$Version - Keep Your Own Llamas",
        "--notes-file", "$repo\RELEASE_NOTES_v$Version.md"
    )
    
    # Add artifacts
    foreach ($artifact in $artifacts) {
        $ghArgs += Join-Path $releaseDir $artifact
    }
    
    # Add checksums
    $checksumFile = Join-Path $releaseDir "SHA256SUMS.txt"
    if (Test-Path $checksumFile) {
        $ghArgs += $checksumFile
    }
    
    Write-Host "  Running: gh $($ghArgs -join ' ')" -ForegroundColor DarkGray
    
    & gh @ghArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ GitHub release created successfully!" -ForegroundColor Green
        Write-Host "`nRelease URL: https://github.com/makr-code/ThemisDB/releases/tag/$tag" -ForegroundColor Cyan
    } else {
        Write-Error "Failed to create GitHub release (exit code: $LASTEXITCODE)"
        Write-Host "`nFallback to manual creation:" -ForegroundColor Yellow
        Write-Host "  https://github.com/makr-code/ThemisDB/releases/new?tag=$tag" -ForegroundColor White
    }
}

Write-Host "`n=== Release Complete ===" -ForegroundColor Green
Write-Host "Version: v$Version" -ForegroundColor Cyan
Write-Host "Artifacts: $($artifacts.Count) files" -ForegroundColor Cyan
Write-Host "Tag: $tag" -ForegroundColor Cyan
