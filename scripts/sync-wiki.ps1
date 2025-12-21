param(
    [string]$WikiRepo = "https://github.com/makr-code/ThemisDB.wiki.git",
    [string]$TempDir = ".\wiki-temp",
    [switch]$DryRun = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Wiki Sync ===" -ForegroundColor Cyan
Write-Host ""

if (Test-Path $TempDir) {
    Remove-Item -Path $TempDir -Recurse -Force
}

Write-Host "Cloning wiki repository..." -ForegroundColor Yellow
git clone $WikiRepo $TempDir

if (-not (Test-Path $TempDir)) {
    Write-Host "ERROR: Failed to clone wiki" -ForegroundColor Red
    exit 1
}

Push-Location $TempDir

try {
    Write-Host "Copying documentation..." -ForegroundColor Yellow
    
    # Create Home page
    "# ThemisDB Documentation`n`nWelcome to ThemisDB Wiki!`n`n## Quick Links`n- Documentation Index`n- Quick Start`n- Installation Guide" | Out-File "Home.md" -Encoding UTF8
    Write-Host "  Created Home.md" -ForegroundColor Green
    
    # Create Sidebar
    "**ThemisDB Wiki**`n* [Home](Home)`n* [Documentation](INDEX)`n* [Quick Start](guides/QUICK_START)`n* [Docker](docker/README)" | Out-File "_Sidebar.md" -Encoding UTF8
    Write-Host "  Created _Sidebar.md" -ForegroundColor Green
    
    # Create Footer
    "---`n**ThemisDB v1.3.0**" | Out-File "_Footer.md" -Encoding UTF8
    Write-Host "  Created _Footer.md" -ForegroundColor Green
    
    # Copy documentation
    $docFiles = Get-ChildItem -Path "..\docs" -Recurse -Filter "*.md" | Where-Object {
        $_.FullName -notmatch '\\archive\\' -and $_.Name -ne '_Sidebar.md' -and $_.Name -ne '_Footer.md'
    }
    
    $count = 0
    foreach ($file in $docFiles) {
        $rel = $file.FullName.Substring((Resolve-Path "..\docs").Path.Length + 1)
        $target = Join-Path "." $rel
        $dir = Split-Path $target -Parent
        
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
        
        Copy-Item $file.FullName $target -Force
        $count++
    }
    
    Write-Host "  Copied $count doc files" -ForegroundColor Green
    
    # Copy root files
    $rootFiles = @("README.md", "CHANGELOG.md", "LICENSE", "QUICK_REFERENCE.md", "RELEASE_NOTES_v1.3.0.md")
    foreach ($rf in $rootFiles) {
        $src = "..\$rf"
        if (Test-Path $src) {
            Copy-Item $src "." -Force
            Write-Host "  Copied $rf" -ForegroundColor Green
        }
    }
    
    # Copy docker README
    if (Test-Path "..\docker\README.md") {
        if (-not (Test-Path "docker")) {
            New-Item -ItemType Directory -Path "docker" | Out-Null
        }
        Copy-Item "..\docker\README.md" "docker\README.md" -Force
        Write-Host "  Copied docker/README.md" -ForegroundColor Green
    }
    
    # Copy compiled docs
    if (Test-Path "..\docs\compiled") {
        if (-not (Test-Path "compiled")) {
            New-Item -ItemType Directory -Path "compiled" | Out-Null
        }
        Copy-Item "..\docs\compiled\*" "compiled\" -Force
        Write-Host "  Copied compiled documentation" -ForegroundColor Green
    }
    
    if ($DryRun) {
        Write-Host "`nDry run - not committing" -ForegroundColor Yellow
        git status
    } else {
        Write-Host "`nCommitting changes..." -ForegroundColor Yellow
        
        git add .
        $status = git status --porcelain
        
        if ($status) {
            $msg = "docs: sync from main repository $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
            git commit -m $msg
            
            Write-Host "Pushing to wiki..." -ForegroundColor Yellow
            git push origin master
            
            Write-Host "`nWiki sync complete!" -ForegroundColor Green
        } else {
            Write-Host "No changes to commit" -ForegroundColor Gray
        }
    }
    
} finally {
    Pop-Location
}

if (-not $DryRun) {
    Remove-Item -Path $TempDir -Recurse -Force
}

Write-Host "`n=== Complete ===" -ForegroundColor Cyan
Write-Host "Wiki: https://github.com/makr-code/ThemisDB/wiki" -ForegroundColor White
