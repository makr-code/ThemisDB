#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Diagnose VS Code CMake Setup for ThemisDB
.DESCRIPTION
    Checks all configurations and provides recommendations
#>

[CmdletBinding()]
param()

$ErrorActionPreference = 'Continue'
$ProgressPreference = 'SilentlyContinue'

Write-Host ""
Write-Host "CMake Setup Diagnostics" -ForegroundColor Cyan
Write-Host "=======================" -ForegroundColor Cyan
Write-Host ""

$WorkspaceRoot = 'C:\VCC\themis'
$VsCodeDir = Join-Path $WorkspaceRoot '.vscode'
$BuildDir = Join-Path $WorkspaceRoot 'build-msvc-windows-release'

$ErrorCount = 0
$WarningCount = 0
$SuccessCount = 0

function Test-File {
    param(
        [string]$Path,
        [string]$Description,
        [ValidateSet('Must', 'Should')]
        [string]$Requirement = 'Must'
    )
    
    if (Test-Path $Path) {
        Write-Host "[OK] $Description" -ForegroundColor Green
        $script:SuccessCount++
    } else {
        if ($Requirement -eq 'Must') {
            Write-Host "[ERROR] $Description - MISSING!" -ForegroundColor Red
            $script:ErrorCount++
        } else {
            Write-Host "[WARN] $Description - optional" -ForegroundColor Yellow
            $script:WarningCount++
        }
    }
}

function Test-Command {
    param(
        [string]$Command,
        [string]$Description
    )
    
    if (Get-Command $Command -ErrorAction SilentlyContinue) {
        Write-Host "[OK] $Description" -ForegroundColor Green
        $script:SuccessCount++
    } else {
        Write-Host "[ERROR] $Description - NOT FOUND!" -ForegroundColor Red
        $script:ErrorCount++
    }
}

# ============================================================================
# 1. VS Code Configuration Files
# ============================================================================

Write-Host "1. VS Code Configuration Files" -ForegroundColor Cyan
Test-File -Path (Join-Path $VsCodeDir 'settings.json') -Description 'settings.json'
Test-File -Path (Join-Path $VsCodeDir 'c_cpp_properties.json') -Description 'c_cpp_properties.json'
Test-File -Path (Join-Path $VsCodeDir 'tasks.json') -Description 'tasks.json'
Write-Host ""

# ============================================================================
# 2. CMake Preset Files
# ============================================================================

Write-Host "2. CMake Preset Files" -ForegroundColor Cyan
Test-File -Path (Join-Path $WorkspaceRoot 'CMakePresets.json') -Description 'CMakePresets.json'
Test-File -Path (Join-Path $WorkspaceRoot 'CMakeUserPresets.json') -Description 'CMakeUserPresets.json'
Test-File -Path (Join-Path $WorkspaceRoot 'CMakeLists.txt') -Description 'CMakeLists.txt'
Write-Host ""

# ============================================================================
# 3. Required Tools
# ============================================================================

Write-Host "3. Required Build Tools" -ForegroundColor Cyan
Test-Command -Command 'cmake' -Description 'CMake'
Test-Command -Command 'ninja' -Description 'Ninja'
Test-Command -Command 'cl' -Description 'MSVC Compiler (cl.exe)'
Write-Host ""

# ============================================================================
# 4. LoRA-Specific Files
# ============================================================================

Write-Host "4. LoRA Migration Files" -ForegroundColor Cyan
Test-File -Path (Join-Path $WorkspaceRoot 'src\llm\lora_framework\lora_adapter_manager.cpp') -Description 'lora_adapter_manager.cpp (compatibility wrapper)'
Test-File -Path (Join-Path $WorkspaceRoot 'tests\llm\test_lora_adapter_application.cpp') -Description 'test_lora_adapter_application.cpp'
Test-File -Path (Join-Path $WorkspaceRoot 'tests\test_lora_framework.cpp') -Description 'test_lora_framework.cpp'
Test-File -Path (Join-Path $WorkspaceRoot 'include\llm\multi_lora_manager.h') -Description 'multi_lora_manager.h'
Write-Host ""

# ============================================================================
# 5. Build Directory Status
# ============================================================================

Write-Host "5. Build Directory Status" -ForegroundColor Cyan
if (Test-Path $BuildDir) {
    $cacheFile = Join-Path $BuildDir 'CMakeCache.txt'
    if (Test-Path $cacheFile) {
        Write-Host "[OK] Build directory exists and configured" -ForegroundColor Green
        $script:SuccessCount++
    } else {
        Write-Host "[WARN] Build directory exists but not configured" -ForegroundColor Yellow
        $script:WarningCount++
    }
} else {
    Write-Host "[INFO] Build directory not yet created (will be created on first configure)" -ForegroundColor Blue
}
Write-Host ""

# ============================================================================
# 6. Settings Verification
# ============================================================================

Write-Host "6. VS Code Settings Verification" -ForegroundColor Cyan
$settingsPath = Join-Path $VsCodeDir 'settings.json'
if (Test-Path $settingsPath) {
    $settings = Get-Content $settingsPath | ConvertFrom-Json -ErrorAction SilentlyContinue
    
    if ($settings.'cmake.configurePreset' -eq 'vscode-windows-release') {
        Write-Host "[OK] cmake.configurePreset set correctly" -ForegroundColor Green
        $script:SuccessCount++
    } else {
        Write-Host "[ERROR] cmake.configurePreset not set correctly" -ForegroundColor Red
        $script:ErrorCount++
    }
    
    if ($settings.'cmake.generator' -eq 'Ninja') {
        Write-Host "[OK] cmake.generator set to Ninja" -ForegroundColor Green
        $script:SuccessCount++
    } else {
        Write-Host "[WARN] cmake.generator not set to Ninja" -ForegroundColor Yellow
        $script:WarningCount++
    }
}
Write-Host ""

# ============================================================================
# 7. Summary
# ============================================================================

Write-Host "SUMMARY" -ForegroundColor Cyan
Write-Host "=======" -ForegroundColor Cyan
Write-Host "Success:  $SuccessCount" -ForegroundColor Green
Write-Host "Warnings: $WarningCount" -ForegroundColor Yellow
Write-Host "Errors:   $ErrorCount" -ForegroundColor Red
Write-Host ""

if ($ErrorCount -eq 0) {
    Write-Host "[SUCCESS] All diagnostics passed! Ready to build in VS Code." -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. Reload VS Code: Ctrl+Shift+P -> 'Developer: Reload Window'"
    Write-Host "2. Configure: Ctrl+Shift+P -> 'CMake: Configure'"
    Write-Host "3. Build: Ctrl+Shift+P -> 'CMake: Build'"
    Write-Host ""
    exit 0
} else {
    Write-Host "[FAILURE] Some issues found. Please fix them before building." -ForegroundColor Red
    exit 1
}
