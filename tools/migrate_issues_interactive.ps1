#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Quick-Start: Migrate GitHub Issues to 7-Phase Workflow

.DESCRIPTION
    Interactive PowerShell script to:
    1. Check GitHub CLI setup
    2. List existing gap-remediation issues
    3. Show dry-run preview
    4. Optionally apply live updates

.EXAMPLE
    .\migrate_issues_interactive.ps1

.NOTES
    Requires: gh CLI installed and authenticated
    Optional: GH_TOKEN environment variable
#>

param(
    [switch]$SkipValidation,
    [switch]$DryRun,
    [string]$Module = ""
)

function Write-Status {
    param($Message, [ValidateSet("OK", "WARN", "ERROR", "INFO")]$Type = "INFO")
    
    $colors = @{
        "OK" = "Green"
        "WARN" = "Yellow"
        "ERROR" = "Red"
        "INFO" = "Cyan"
    }
    
    $symbols = @{
        "OK" = "✅"
        "WARN" = "⚠️"
        "ERROR" = "❌"
        "INFO" = "ℹ️"
    }
    
    Write-Host "$($symbols[$Type]) $Message" -ForegroundColor $colors[$Type]
}

function Test-GitHubCLI {
    Write-Status "Checking GitHub CLI setup..." "INFO"
    
    try {
        $version = gh --version
        Write-Status "GitHub CLI found: $version" "OK"
        return $true
    } catch {
        Write-Status "GitHub CLI not found. Install with: brew install gh (macOS) or choco install gh (Windows)" "ERROR"
        return $false
    }
}

function Test-GitHubAuth {
    Write-Status "Checking GitHub authentication..." "INFO"
    
    try {
        $status = gh auth status 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Status "GitHub authenticated ✓" "OK"
            return $true
        } else {
            Write-Status "GitHub not authenticated. Run: gh auth login" "WARN"
            return $false
        }
    } catch {
        Write-Status "Could not check GitHub auth" "ERROR"
        return $false
    }
}

function Get-ExistingIssues {
    Write-Status "Fetching existing gap-remediation issues..." "INFO"
    
    try {
        $issues = gh issue list `
            --repo makr-code/ThemisDB `
            --label gap-scanner `
            --state open `
            --json number,title,labels `
            2>$null | ConvertFrom-Json
        
        return $issues
    } catch {
        Write-Status "Could not fetch issues from GitHub" "ERROR"
        return $null
    }
}

function Show-IssuesPreview {
    param($Issues)
    
    if (-not $Issues) {
        Write-Status "No issues found" "WARN"
        return
    }
    
    Write-Host "`n📋 Existing Gap-Remediation Issues:`n" -ForegroundColor Cyan
    
    foreach ($issue in $Issues) {
        $title = $issue.title
        Write-Host "  #$($issue.number): $title"
    }
    
    Write-Host "`nTotal: $($Issues.Count) issues`n"
}

function Show-DryRun {
    param([string]$AggregateFile, [string]$Module)
    
    Write-Status "Running dry-run preview..." "INFO"
    
    $pyCmd = "python tools/migrate_issues_to_7phase.py `"$AggregateFile`""
    
    if ($Module) {
        $pyCmd += " --module $Module"
    }
    
    Write-Host "`nExecuting: $pyCmd`n" -ForegroundColor DarkGray
    
    & python tools/migrate_issues_to_7phase.py $AggregateFile $(if ($Module) { "--module", $Module } else { "" })
}

function Confirm-LiveUpdate {
    Write-Host "`n" -NoNewline
    Write-Status "LIVE UPDATE will modify issues on GitHub. Continue?" "WARN"
    
    $choices = @(
        [System.Management.Automation.Host.ChoiceDescription]::new("&Yes", "Update all issues"),
        [System.Management.Automation.Host.ChoiceDescription]::new("&No", "Cancel")
    )
    
    $result = $Host.UI.PromptForChoice("", "Continue with live update?", $choices, 1)
    return $result -eq 0
}

function Do-LiveUpdate {
    param([string]$AggregateFile, [string]$Module)
    
    $pyCmd = "python tools/migrate_issues_to_7phase.py `"$AggregateFile`" --github"
    
    if ($Module) {
        $pyCmd += " --module $Module"
    }
    
    Write-Host "`nExecuting: $pyCmd`n" -ForegroundColor DarkGray
    
    & python tools/migrate_issues_to_7phase.py $AggregateFile --github $(if ($Module) { "--module", $Module } else { "" })
}

function Show-VerificationSteps {
    Write-Status "Migration complete! Verification steps:" "OK"
    
    Write-Host @"
    
    1. Open GitHub: https://github.com/makr-code/ThemisDB/issues?labels=gap-scanner
    
    2. Click on an issue and verify new format:
       ✅ 7-Phase Workflow Progress section
       ✅ Phase 0-7 sections with checkboxes
       ✅ Error Handling & Escalation section
       ✅ New labels: ready-for-ai-agent, 7phase-workflow
    
    3. Start AI agent on first issue:
       gh issue view <number>
       
    4. Feedback: Comment on issue or open discussion
    
"@
}

# ===== MAIN =====

Write-Host @"

╔════════════════════════════════════════════════════════════════╗
║  🔄 GitHub Issue Migration to 7-Phase Workflow                ║
║                                                                ║
║  This script will:                                             ║
║  1. Check GitHub CLI setup                                    ║
║  2. List existing gap-remediation issues                      ║
║  3. Show dry-run preview of changes                           ║
║  4. Optionally apply live updates to GitHub                   ║
╚════════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

# Step 1: Validate environment
if (-not $SkipValidation) {
    if (-not (Test-GitHubCLI)) {
        Write-Status "GitHub CLI required. Exiting." "ERROR"
        exit 1
    }
    
    if (-not (Test-GitHubAuth)) {
        $choice = $Host.UI.PromptForChoice(
            "",
            "Continue without GitHub authentication? (dry-run only)",
            @(
                [System.Management.Automation.Host.ChoiceDescription]::new("&Yes", "Continue with dry-run"),
                [System.Management.Automation.Host.ChoiceDescription]::new("&No", "Exit and authenticate")
            ),
            1
        )
        
        if ($choice -ne 0) {
            exit 1
        }
    }
}

# Step 2: Check aggregate file
$AggregateFile = "ai_working/gap_scan_v3_aggregate.json"
if (-not (Test-Path $AggregateFile)) {
    Write-Status "Aggregate file not found: $AggregateFile" "ERROR"
    Write-Status "Run gap scanner first: python tools/gap_scanner_v3.py . ai_working" "INFO"
    exit 1
}

Write-Status "Aggregate file found: $AggregateFile" "OK"

# Step 3: Get existing issues
$issues = Get-ExistingIssues
Show-IssuesPreview -Issues $issues

# Step 4: Ask user what to do
Write-Host "`nWhat would you like to do?`n" -ForegroundColor Cyan

$options = @(
    [System.Management.Automation.Host.ChoiceDescription]::new("&1. Show dry-run preview", "Preview changes without modifying"),
    [System.Management.Automation.Host.ChoiceDescription]::new("&2. Apply LIVE update (all)", "Migrate all issues to 7-phase"),
    [System.Management.Automation.Host.ChoiceDescription]::new("&3. Apply LIVE update (specific module)", "Migrate only one module"),
    [System.Management.Automation.Host.ChoiceDescription]::new("&4. Exit", "Cancel")
)

$choice = $Host.UI.PromptForChoice("", "Select option", $options, 0)

switch ($choice) {
    0 {
        Write-Host "`n"
        Show-DryRun -AggregateFile $AggregateFile -Module $Module
        Write-Status "Dry-run complete. No changes made to GitHub." "INFO"
    }
    1 {
        if (Confirm-LiveUpdate) {
            Write-Host "`n"
            Do-LiveUpdate -AggregateFile $AggregateFile -Module ""
            Show-VerificationSteps
        } else {
            Write-Status "Update cancelled." "INFO"
        }
    }
    2 {
        $mod = Read-Host "Enter module name (security, memory, reliability, etc.)"
        if ($mod) {
            if (Confirm-LiveUpdate) {
                Write-Host "`n"
                Do-LiveUpdate -AggregateFile $AggregateFile -Module $mod
                Show-VerificationSteps
            } else {
                Write-Status "Update cancelled." "INFO"
            }
        }
    }
    3 {
        Write-Status "Exiting." "INFO"
        exit 0
    }
}

Write-Host "`nDone!`n" -ForegroundColor Green
