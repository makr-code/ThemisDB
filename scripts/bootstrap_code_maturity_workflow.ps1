#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Bootstrap-Skript für die Code-Maturity-Pipeline
    
.DESCRIPTION
    Triggert den GitHub Actions Workflow "maintenance-docs.yml"
    manuell, um Code-Maturity-Report, Version-Tracking und Badge-JSONs
    zu aktualisieren.
    
    Der Workflow läuft zusätzlich automatisch per Schedule.

.EXAMPLE
    ./bootstrap_code_maturity_workflow.ps1
    
.NOTES
    Voraussetzung: GitHub CLI (gh) muss installiert und authentifiziert sein.
    Installation: https://cli.github.com/
#>

param(
    [switch]$UpdateHeaders = $false,
    [switch]$FailOnDrift = $false,
    [switch]$DryRun = $false
)

$ErrorActionPreference = 'Stop'

# ────────────────────────────────────────────────────────────────────────────
# 1. Überprüfe GitHub CLI
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n📋 Checking GitHub CLI..." -ForegroundColor Cyan

$ghPath = (Get-Command gh -ErrorAction SilentlyContinue)
if (-not $ghPath) {
    Write-Error @"
GitHub CLI is not installed or not in PATH.
Please install from: https://cli.github.com/
"@
}

Write-Host "✅ GitHub CLI found: $(gh --version)" -ForegroundColor Green

# ────────────────────────────────────────────────────────────────────────────
# 2. Überprüfe Git-Status
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n📦 Checking repository status..." -ForegroundColor Cyan

$repoRoot = (Get-Location).Path
if (-not (Test-Path '.git')) {
    Write-Error "Not in a Git repository (no .git directory found)."
}

# Authentifizierung prüfen
try {
    $status = gh repo view --json name 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "GitHub CLI not authenticated. Run 'gh auth login' first."
    }
}
catch {
    Write-Error "Failed to verify GitHub authentication: $_"
}

Write-Host "✅ Repository verified" -ForegroundColor Green

# ────────────────────────────────────────────────────────────────────────────
# 3. Prüfe auf fehlende Report-Datei
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n📄 Checking for code maturity report..." -ForegroundColor Cyan

$reportPath = 'docs\code_maturity_report.md'
if (Test-Path $reportPath) {
    Write-Host "⚠️  docs/code_maturity_report.md bereits vorhanden" -ForegroundColor Yellow
}
else {
    Write-Host "❌ Missing: $reportPath (this is expected on first run)" -ForegroundColor Yellow
}

# ────────────────────────────────────────────────────────────────────────────
# 4. Build workflow parameters
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n⚙️  Workflow parameters:" -ForegroundColor Cyan
Write-Host "  update_headers: $($UpdateHeaders.ToString().ToLower())"
Write-Host "  fail_on_drift:  $($FailOnDrift.ToString().ToLower())"

if ($UpdateHeaders) {
    Write-Host "  ⚠️  WARNING: Headers WILL be updated in source files" -ForegroundColor Yellow
    Write-Host "     Review the workflow artifacts before committing." -ForegroundColor Yellow
}
else {
    Write-Host "  ℹ️  Check-only mode: no source files will be modified" -ForegroundColor Cyan
}

# ────────────────────────────────────────────────────────────────────────────
# 5. Dry run oder echte Ausführung
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n🚀 Triggering workflow..." -ForegroundColor Cyan

$refBranch = (git rev-parse --abbrev-ref HEAD)
$workflowFile = 'maintenance-docs.yml'

$workflowCmd = @(
    'workflow', 'run', $workflowFile,
    '--ref', $refBranch
)

if ($UpdateHeaders) {
    $workflowCmd += @('-f', "update_headers=true")
}

if ($FailOnDrift) {
    $workflowCmd += @('-f', "fail_on_findings=true")
}

if ($DryRun) {
    Write-Host "`n📋 [DRY RUN] Command that would be executed:" -ForegroundColor Yellow
    Write-Host "  gh $(($workflowCmd -join ' '))`n" -ForegroundColor Gray
    exit 0
}

# Execute workflow trigger
$output = gh @workflowCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to trigger workflow:`n$output"
}

Write-Host "✅ Workflow triggered successfully!" -ForegroundColor Green
Write-Host "`n📊 Run details:" -ForegroundColor Cyan
Write-Host "  Workflow: $workflowFile"
Write-Host "  Branch:   $refBranch"
Write-Host "  Mode:     $(if ($UpdateHeaders) { 'REWRITE' } else { 'CHECK-ONLY' })"

# ────────────────────────────────────────────────────────────────────────────
# 6. Monitor workflow status
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n⏳ Monitoring workflow..." -ForegroundColor Cyan
Write-Host "Check live status: " -ForegroundColor Cyan -NoNewline
Write-Host "gh workflow view $workflowFile --ref $refBranch" -ForegroundColor Gray

# Get the latest run
$latestRun = gh run list --workflow=$workflowFile --branch=$refBranch --limit=1 --json url --jq '.[0].url' 2>&1

if ($latestRun -and -not $latestRun.Contains('error')) {
    Write-Host "`n🔗 Latest run: $latestRun" -ForegroundColor Cyan
}

# ────────────────────────────────────────────────────────────────────────────
# 7. Next steps
# ────────────────────────────────────────────────────────────────────────────

Write-Host "`n📋 Next steps:" -ForegroundColor Cyan
Write-Host "  1. Wait 30-60 seconds for the workflow to initialize"
Write-Host "  2. Check progress: " -NoNewline
Write-Host "gh run view --repo . --log-failed" -ForegroundColor Gray
Write-Host "  3. After completion, check artifacts:"
Write-Host "     - docs/code_maturity_report.md (generated report)"
Write-Host "     - .github/badges/*.json (badge files)"
Write-Host "     - .github/version_tracking.json (version tracking)"

Write-Host "`n✨ Bootstrap complete! The workflow now refreshes metrics on schedule (daily 04:00 UTC; weekly docs alignment Monday 05:00 UTC).`n" -ForegroundColor Green

if (-not $UpdateHeaders) {
    Write-Host "💡 Tip: To update headers automatically, run with -UpdateHeaders flag:`n" -ForegroundColor Cyan
    Write-Host "  ./scripts/bootstrap_code_maturity_workflow.ps1 -UpdateHeaders`n" -ForegroundColor Gray
}
