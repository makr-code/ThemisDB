# Script to create GitHub issues from templates in batch
# Usage: .\create_issues.ps1 [phase]
#   phase: all, phase1, phase2, phase3, or specific issue number (01-06)

param(
    [string]$Phase = "all"
)

$ErrorActionPreference = "Stop"

$REPO = "makr-code/ThemisDB"
$TEMPLATE_DIR = ".github\ISSUE_TEMPLATE"

# Check if gh CLI is installed
if (!(Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Host "Error: GitHub CLI (gh) is not installed" -ForegroundColor Red
    Write-Host "Install from: https://cli.github.com/"
    exit 1
}

# Check if authenticated
$authStatus = gh auth status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Not authenticated with GitHub" -ForegroundColor Red
    Write-Host "Run: gh auth login"
    exit 1
}

Write-Host "Creating GitHub issues from templates..." -ForegroundColor Green
Write-Host ""

# Function to create labels if they don't exist
function Ensure-Label {
    param(
        [string]$Label,
        [string]$Color = "0E8A16",
        [string]$Description = ""
    )
    
    $existing = gh label list --repo $REPO --json name -q ".[].name" 2>&1 | Where-Object { $_ -eq $Label }
    
    if (!$existing) {
        Write-Host "  Creating label: $Label" -ForegroundColor Cyan
        gh label create $Label --repo $REPO --color $Color --description $Description 2>&1 | Out-Null
    }
}

# Create necessary labels
Write-Host "Ensuring labels exist..." -ForegroundColor Cyan
Ensure-Label -Label "enhancement" -Color "a2eeef" -Description "New feature or request"
Ensure-Label -Label "raid" -Color "d73a4a" -Description "RAID and erasure coding related"
Ensure-Label -Label "lora" -Color "0075ca" -Description "LoRA adapter related"
Ensure-Label -Label "gpu" -Color "e99695" -Description "GPU acceleration related"
Ensure-Label -Label "ml" -Color "d4c5f9" -Description "Machine learning related"
Ensure-Label -Label "high-priority" -Color "d93f0b" -Description "High priority"
Ensure-Label -Label "medium-priority" -Color "fbca04" -Description "Medium priority"
Ensure-Label -Label "strategic" -Color "0e8a16" -Description "Strategic long-term feature"
Ensure-Label -Label "v1.4.0" -Color "bfdadc" -Description "Target version 1.4.0"
Ensure-Label -Label "v1.5.0" -Color "c2e0c6" -Description "Target version 1.5.0"
Ensure-Label -Label "v1.6.0" -Color "c5def5" -Description "Target version 1.6.0"
Write-Host ""

# Function to create an issue
function Create-Issue {
    param(
        [string]$Title,
        [string]$Labels,
        [string]$TemplateFile
    )
    
    Write-Host "Creating issue: $Title" -ForegroundColor Yellow
    
    $templatePath = Join-Path $PSScriptRoot $TemplateFile
    
    if (!(Test-Path $templatePath)) {
        Write-Host "  Warning: Template file not found: $templatePath" -ForegroundColor Red
        return $false
    }
    
    try {
        $result = gh issue create --repo $REPO --title $Title --label $Labels --body-file $templatePath 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  Created: $result" -ForegroundColor Green
            return $true
        } else {
            Write-Host "  Failed: $result" -ForegroundColor Red
            return $false
        }
    }
    catch {
        Write-Host "  Error: $_" -ForegroundColor Red
        return $false
    }
}

# Define all issues
$issues = @(
    @{
        Number = "01"
        Phase = "phase1"
        Title = "[v1.4.0] Implement RAID 6 (Dual Parity) Support"
        Labels = "enhancement,raid,high-priority,v1.4.0"
        File = "01_raid6_dual_parity.md"
    },
    @{
        Number = "02"
        Phase = "phase1"
        Title = "[v1.4.0] Implement LoRA Quantization (INT8/INT4)"
        Labels = "enhancement,lora,high-priority,v1.4.0"
        File = "02_lora_quantization.md"
    },
    @{
        Number = "03"
        Phase = "phase1"
        Title = "[v1.4.0] Implement Hot Spare Management"
        Labels = "enhancement,raid,high-priority,v1.4.0"
        File = "03_hot_spare_management.md"
    },
    @{
        Number = "04"
        Phase = "phase1"
        Title = "[v1.4.0] Implement Multi-GPU LoRA Support"
        Labels = "enhancement,lora,gpu,high-priority,v1.4.0"
        File = "04_multi_gpu_lora.md"
    },
    @{
        Number = "05"
        Phase = "phase2"
        Title = "[v1.5.0] Implement GPU-Accelerated Erasure Coding"
        Labels = "enhancement,raid,gpu,medium-priority,v1.5.0"
        File = "05_gpu_erasure_coding.md"
    },
    @{
        Number = "06"
        Phase = "phase3"
        Title = "[v1.6.0] Implement Predictive Failure Detection"
        Labels = "enhancement,raid,ml,strategic,v1.6.0"
        File = "06_predictive_failure_detection.md"
    }
)

# Filter issues based on phase parameter
$issuesToCreate = switch ($Phase.ToLower()) {
    "all" { $issues }
    "phase1" { $issues | Where-Object { $_.Phase -eq "phase1" } }
    "phase2" { $issues | Where-Object { $_.Phase -eq "phase2" } }
    "phase3" { $issues | Where-Object { $_.Phase -eq "phase3" } }
    default {
        if ($Phase -match '^\d{2}$') {
            $issues | Where-Object { $_.Number -eq $Phase }
        } else {
            Write-Host "Invalid phase: $Phase" -ForegroundColor Red
            Write-Host "Valid options: all, phase1, phase2, phase3, or 01-06"
            exit 1
        }
    }
}

if ($issuesToCreate.Count -eq 0) {
    Write-Host "No issues found for phase: $Phase" -ForegroundColor Yellow
    exit 0
}

Write-Host "Creating $($issuesToCreate.Count) issue(s)..." -ForegroundColor Cyan
Write-Host ""

$created = 0
$failed = 0

foreach ($issue in $issuesToCreate) {
    if (Create-Issue -Title $issue.Title -Labels $issue.Labels -TemplateFile $issue.File) {
        $created++
    } else {
        $failed++
    }
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Summary:" -ForegroundColor Cyan
Write-Host "  Created: $created" -ForegroundColor Green
$failedColor = if ($failed -gt 0) { "Red" } else { "Gray" }
Write-Host "  Failed:  $failed" -ForegroundColor $failedColor
Write-Host "========================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
}
