#!/usr/bin/env pwsh
<#
.SYNOPSIS
    CI/CD Workflow Validator - Local Analyzer
.DESCRIPTION
    Validates ThemisDB .github/workflows/ structure and common issues.
    Tests can be run locally with -Local flag before pushing to GitHub.
.EXAMPLE
    .\workflow_validator.ps1 -Verbose
    .\workflow_validator.ps1 -Local
#>
param(
    [switch]$Verbose,
    [switch]$Local,
    [string]$OutputFile = 'workflow_validation_report.txt'
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Continue'

$workflowDir = '.github/workflows'
$results = @()
$errors = @()
$warnings = @()

# ─────────────────────────────────────────────────────────────────────────────
# Helper Functions
# ─────────────────────────────────────────────────────────────────────────────

function Validate-YamlStructure {
    param([string]$FilePath, [string]$FileName)
    
    $content = Get-Content $FilePath -Raw
    $lines = $content -split "`n"
    $issues = @()
    
    # 1. Check required top-level keys
    $hasName = $content -match '^\s*name:\s*'
    $hasOn = $content -match '^\s*on:\s*$'
    $hasJobs = $content -match '^\s*jobs:\s*$'
    $hasPermissions = $content -match '^\s*permissions:\s*'
    
    if (-not $hasName) { $issues += "Missing 'name:' field" }
    if (-not $hasOn) { $issues += "Missing 'on:' trigger definition" }
    if (-not $hasJobs) { $issues += "Missing 'jobs:' section" }
    if (-not $hasPermissions) { $issues += "Missing 'permissions:' field (security risk)" }
    
    # 2. Check for invalid indentation (YAML must use spaces, multiples of 2)
    $indentProblems = 0
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^( {1}|   |     | {6,})[\S]') {
            $indentProblems++
            if ($indentProblems -le 3) {
                $issues += "Invalid indentation at line $($i+1): '$line'"
            }
        }
    }
    if ($indentProblems -gt 3) {
        $issues += "... and $($indentProblems - 3) more indentation errors"
    }
    
    # 3. Check for unclosed quotes (simplified)
    $sq = ($content | Select-String -Pattern "'" -AllMatches | Measure-Object -Property Matches).Matches.Count
    if ($sq % 2 -ne 0) {
        $issues += "Unmatched single quotes detected"
    }
    
    # 4. Check for undefined variables
    $vars = [regex]::Matches($content, '\$\{\{\s*(\w+(?:\.\w+)?)\s*\}\}') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
    $builtin = @('github', 'env', 'secrets', 'inputs', 'matrix', 'jobs', 'steps', 'needs')
    $suspicious = $vars | Where-Object { $_ -notmatch '^(github|env|secrets|inputs|matrix|jobs|steps|needs|strategy)\.' -and $_ -notin $builtin }
    
    if ($suspicious.Count -gt 0) {
        $issues += "Potentially undefined variables: $($suspicious -join ', ')"
    }
    
    # 5. Check for missing 'uses' or 'run' in steps
    $stepMatches = [regex]::Matches($content, '^\s+-\s+name:\s+(.+?)$', 'Multiline')
    if ($stepMatches.Count -gt 0) {
        $hasRun = $content -match 'run:\s*(?:\||[^\s])'
        $hasUses = $content -match 'uses:\s*[\w/-]+@'
        if (-not $hasRun -and -not $hasUses) {
            $issues += "Found $($stepMatches.Count) named step(s) but no 'run' or 'uses' directives"
        }
    }
    
    # 6. Check for missing 'needs' dependencies
    $jobsSection = $content -match '(?<=jobs:)(.*?)$' -split "`n`n`n"
    $jobCount = [regex]::Matches($content, '^\s{2}[\w-]+:\s*$', 'Multiline').Count
    if ($jobCount -gt 1) {
        $needsCount = [regex]::Matches($content, '^\s+needs:\s*', 'Multiline').Count
        if ($needsCount -eq 0) {
            $issues += "⚠️  Multiple jobs ($jobCount) but no 'needs' dependencies found (may be intentional)"
        }
    }
    
    # 7. Check for 'if:' conditions without proper escaping
    $ifMatches = [regex]::Matches($content, "if:\s*(.+?)(?=\n\s{2,}\w+:|$)")
    $ifMatches | ForEach-Object {
        $condition = $_.Groups[1].Value
        if ($condition -match "==\s*'?'?" -or $condition -match "!=\s*null") {
            $issues += "Potentially incomplete condition: $condition"
        }
    }
    
    return $issues
}

function Validate-ActionVersions {
    param([string]$FilePath)
    
    $content = Get-Content $FilePath -Raw
    $issues = @()
    
    # Check for unpinned GitHub actions
    $unpinnedActions = [regex]::Matches($content, 'uses:\s*([a-zA-Z0-9/_-]+)(?!@)', 'IgnoreCase')
    if ($unpinnedActions.Count -gt 0) {
        $issues += "⚠️  Found $($unpinnedActions.Count) unpinned GitHub actions (should use @hash or @vX.Y.Z)"
        $unpinnedActions | ForEach-Object { 
            $action = $_.Groups[1].Value
            if ($action -notmatch '^(\./)') {  # Local actions (./) don't need version pins
                $issues += "    └─ $action"
            }
        }
    }
    
    # Check for very old action versions
    $oldActions = [regex]::Matches($content, 'uses:\s*(.+?)@v[0-2]\.', 'IgnoreCase')
    if ($oldActions.Count -gt 0) {
        $issues += "⚠️  Found $($oldActions.Count) potentially outdated actions (v2 or earlier)"
    }
    
    return $issues
}

function Validate-Permissions {
    param([string]$FilePath)
    
    $content = Get-Content $FilePath -Raw
    $issues = @()
    
    # Check if permissions is empty or missing
    if (-not ($content -match 'permissions:\s*\n\s+\w+:')) {
        $issues += "⚠️  Permissions section is empty or malformed (should specify read/write access)"
    }
    
    # Check for overly permissive permissions
    $allWrite = $content -match 'permissions:\s*\n\s+\S+:\s*write'
    if ($allWrite) {
        $issues += "⚠️  Detected write permissions - verify necessity for security"
    }
    
    return $issues
}

# ─────────────────────────────────────────────────────────────────────────────
# Main Validation Loop
# ─────────────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  GitHub Workflows Validator v1.0                              ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $workflowDir)) {
    Write-Error "Workflow directory not found: $workflowDir"
    exit 1
}

$results += "═════════════════════════════════════════════════════════════════"
$results += "CI/CD WORKFLOW VALIDATION REPORT"
$results += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$results += "Location: $(Convert-Path $workflowDir)"
$results += "═════════════════════════════════════════════════════════════════"
$results += ""

$workflowFiles = Get-ChildItem "$workflowDir" -Filter "*.yml" -ErrorAction SilentlyContinue | Sort-Object Name
$results += "Total workflows found: $($workflowFiles.Count)"
$results += ""

foreach ($file in $workflowFiles) {
    $fileName = $file.Name
    $filePath = $file.FullName
    
    Write-Host "📋 Validating: $fileName" -ForegroundColor Yellow
    
    $results += ""
    $results += "──────────────────────────────────────────────────────────────────"
    $results += "📄 FILE: $fileName"
    $results += "──────────────────────────────────────────────────────────────────"
    
    # Run validations
    $structureIssues = Validate-YamlStructure -FilePath $filePath -FileName $fileName
    $actionIssues = Validate-ActionVersions -FilePath $filePath
    $permissionIssues = Validate-Permissions -FilePath $filePath
    
    $allIssues = @($structureIssues) + @($actionIssues) + @($permissionIssues) | Where-Object { $_ }
    
    if ($allIssues.Count -eq 0) {
        $results += "✅ PASS — No issues detected"
        Write-Host "  ✅ PASS" -ForegroundColor Green
    } else {
        $results += "❌ ISSUES FOUND ($($allIssues.Count)):"
        foreach ($issue in $allIssues) {
            $results += "  • $issue"
        }
        Write-Host "  ❌ Found $($allIssues.Count) issue(s)" -ForegroundColor Red
        $allIssues | ForEach-Object { Write-Host "     └─ $_" -ForegroundColor Gray }
        $errors += @($allIssues | ForEach-Object { "$fileName : $_" })
    }
}

# ─────────────────────────────────────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────────────────────────────────────

$results += ""
$results += "═════════════════════════════════════════════════════════════════"
$results += "SUMMARY"
$results += "═════════════════════════════════════════════════════════════════"
$results += ""
$results += "Total files checked: $($workflowFiles.Count)"
$results += "Files with issues: $($errors.Count)"
$results += ""

if ($errors.Count -eq 0) {
    $results += "🎉 All workflows validated successfully!"
    $icon = "✅"
    $color = "Green"
} else {
    $results += "⚠️  Action required: Fix $($errors.Count) workflow(s) before deployment"
    $icon = "❌"
    $color = "Red"
    $results += ""
    $results += "Files requiring fixes:"
    $errors | Group-Object { $_.Split(':')[0] } | ForEach-Object { 
        $results += "  • $($_.Name)"
    }
}

$results += ""
$results += "═════════════════════════════════════════════════════════════════"

# Write report
$results | Out-File -FilePath $OutputFile -Encoding UTF8 -Force

Write-Host ""
Write-Host $icon " Report saved to: $OutputFile" -ForegroundColor $color
Write-Host ""
Write-Host ($results[-5..-1] -join "`n") -ForegroundColor $color

exit ($errors.Count -gt 0 ? 1 : 0)
