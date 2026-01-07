$ErrorActionPreference = "Stop"
Write-Host "=== ThemisDB Documentation Builder ===" -ForegroundColor Cyan

$OutputDir = ".\docs\compiled"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$timestamp = Get-Date -Format "yyyy-MM-dd"
$singleMdPath = Join-Path $OutputDir "ThemisDB-Complete-$timestamp.md"

$content = @"
# ThemisDB - Complete Documentation
**Version:** 1.3.0
**Generated:** $timestamp

---

"@

$docFiles = @(
    "README.md",
    "QUICK_REFERENCE.md",
    "RELEASE_NOTES_v1.3.0.md",
    "CHANGELOG.md",
    "docker\README.md",
    "docs\INDEX.md",
    "docs\Home.md",
    "docs\DOCUMENTATION_INDEX.md",
    "docs\architecture\OVERVIEW.md",
    "docs\guides\QUICK_START.md",
    "docs\guides\INSTALLATION.md",
    "docs\operations\CONFIGURATION.md",
    "docs\aql\aql_syntax.md",
    "docs\features\features_overview.md",
    "docs\api\REST_API.md",
    "docs\deployment\DOCKER_DEPLOYMENT.md",
    "LICENSE"
)

foreach ($file in $docFiles) {
    if (Test-Path $file) {
        Write-Host "Adding: $file" -ForegroundColor Gray
        $content += "`n## File: $file`n`n"
        $content += (Get-Content $file -Raw -Encoding UTF8)
        $content += "`n`n---`n"
    }
}

$content | Out-File -FilePath $singleMdPath -Encoding UTF8
Write-Host "Complete! $singleMdPath" -ForegroundColor Green
