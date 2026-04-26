#!/usr/bin/env powershell
# ThemisDB Config Migration Helper (Windows)
# This script helps reorganize configuration files according to the new structure

param(
    [string]$ConfigRoot = "config"
)

function Write-Success { Write-Host "✅ $args" -ForegroundColor Green }
function Write-Skip { Write-Host "⏭️  $args" -ForegroundColor Yellow }
function Write-Error { Write-Host "❌ $args" -ForegroundColor Red }
function Write-Info { Write-Host "ℹ️  $args" -ForegroundColor Cyan }

if (-not (Test-Path $ConfigRoot)) {
    Write-Error "Config directory not found: $ConfigRoot"
    exit 1
}

Write-Host "🔍 ThemisDB Configuration Reorganization Helper"
Write-Host "=" * 50
Write-Host "Root: $(Resolve-Path $ConfigRoot)"
Write-Host ""

$migrated = 0
$skipped = 0
$errors = 0

function Migrate-File {
    param(
        [string]$Source,
        [string]$Dest
    )

    $sourcePath = Join-Path $ConfigRoot $Source
    $destPath = Join-Path $ConfigRoot $Dest
    $destDir = Split-Path $destPath

    if (-not (Test-Path $sourcePath)) {
        return  # File doesn't exist, skip
    }

    # Create destination directory
    if (-not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }

    # Check if destination exists
    if (Test-Path $destPath) {
        $sourceHash = (Get-FileHash $sourcePath).Hash
        $destHash = (Get-FileHash $destPath).Hash
        
        if ($sourceHash -eq $destHash) {
            Write-Skip "$Source → $Dest (identical, keeping backup)"
            $script:skipped++
        } else {
            Write-Error "$Source → $Dest (destination exists with different content)"
            $script:errors++
            return 1
        }
    } else {
        Copy-Item $sourcePath $destPath -Force
       Write-Success "Migrated: $Source → $Dest"
        $script:migrated++
    }
}

function Create-Index {
    param(
        [string]$Category,
        [string]$Dir
    )

    $dirPath = Join-Path $ConfigRoot $Dir
    if (-not (Test-Path $dirPath)) {
        return
    }

    $indexPath = Join-Path $dirPath "README.md"
    if (-not (Test-Path $indexPath)) {
        $files = Get-ChildItem -Path $dirPath -Filter "*.yaml", "*.yml", "*.json" | 
                 Select-Object -ExpandProperty Name | 
                 ForEach-Object { "- ``$_``" } | 
                 Out-String

        $content = @"
# $Category Configuration

## Overview

This directory contains configuration files for $Category.

## Files

$files

## Usage

Import these configs in your main config.yaml.

---
Generated: $(Get-Date)
"@

        Set-Content -Path $indexPath -Value $content
        Write-Info "Created index: $indexPath"
    }
}

# ============================================================================
# Security Configs
# ============================================================================
Write-Host "📋 Security Configuration" -ForegroundColor Blue
Write-Host "---"
Migrate-File "pii_patterns.yaml" "security/pii_patterns.yaml"
Migrate-File "rbac_roles.json" "security/rbac_roles.json"
Migrate-File "rbac_roles.yaml" "security/rbac_roles.yaml"
Migrate-File "user_roles.json" "security/user_roles.json"
Migrate-File "graph_protection.yaml" "security/graph_protection.yaml"
Create-Index "Security" "security"
Write-Host ""

# ============================================================================
# AI/ML Configs
# ============================================================================
Write-Host "📋 AI/ML Configuration" -ForegroundColor Blue
Write-Host "---"
Migrate-File "lora_training_config.yaml" "ai_ml/lora_training_config.yaml"
Migrate-File "vision_config.yaml" "ai_ml/vision/config.yaml"
Migrate-File "llm_system_prompts.yaml" "ai_ml/llm/system_prompts.yaml"
Migrate-File "llm-models.yaml" "ai_ml/llm/models.yaml"
Create-Index "AI/ML" "ai_ml"
Write-Host ""

# ============================================================================
# Compliance
# ============================================================================
Write-Host "📋 Compliance & Governance" -ForegroundColor Blue
Write-Host "---"
Migrate-File "ethical_guidelines.yaml" "compliance/ethical_guidelines.yaml"
Migrate-File "governance.yaml" "compliance/governance.yaml"
Migrate-File "audit.yaml" "compliance/audit/audit.yaml"
Create-Index "Compliance" "compliance"
Write-Host ""

# ============================================================================
# Data Management
# ============================================================================
Write-Host "📋 Data Management" -ForegroundColor Blue
Write-Host "---"
Migrate-File "mime_types.yaml" "data_management/mime_types.yaml"
Migrate-File "retention_policies.yaml" "data_management/retention_policies.yaml"
Create-Index "Data Management" "data_management"
Write-Host ""

# ============================================================================
# Summary
# ============================================================================
Write-Host ""
Write-Host "📊 Migration Summary" -ForegroundColor Cyan
Write-Host ("=" * 50)
Write-Host "  Migrated: $migrated" -ForegroundColor Green
Write-Host "  Skipped:  $skipped" -ForegroundColor Yellow
Write-Host "  Errors:   $errors" -ForegroundColor Red
Write-Host ""

if ($errors -eq 0) {
    Write-Host "✅ Migration completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "📝 Next Steps:"
    Write-Host "  1. Review migrated files in each subdirectory"
    Write-Host "  2. Update your load paths to use new structure"
    Write-Host "  3. Test your application startup"
    Write-Host "  4. Check logs for deprecation warnings"
} else {
    Write-Host "⚠️  Migration completed with errors!" -ForegroundColor Red
}
