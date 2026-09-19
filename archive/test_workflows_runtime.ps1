#!/usr/bin/env pwsh
<#
.SYNOPSIS
    GitHub Actions Workflow Runtime Test Suite
    Testet Workflows lokal mit act und validiert Laufzeitverhalten
.DESCRIPTION
    Führt eine Reihe von Workflow-Runtime-Tests durch:
    - Syntax-Validierung
    - Job-Parsing
    - Step-Execution
    - Error-Handling
#>

param(
    [ValidateSet('quick', 'full', 'debug')]
    [string]$Mode = 'quick',
    
    [string[]]$WorkflowFilter = @(),
    
    [switch]$Verbose
)

$ErrorActionPreference = 'Stop'
$VerbosePreference = if ($Verbose) { 'Continue' } else { 'SilentlyContinue' }

# ═════════════════════════════════════════════════════════════════════════════════
# KONFIGURATION
# ═════════════════════════════════════════════════════════════════════════════════

$WorkflowDir = '.github/workflows'
$ReportFile = 'WORKFLOW_RUNTIME_TEST_REPORT.txt'
$TestResults = @()

# Test-Konfiguration pro Workflow
$WorkflowTests = @{
    'automation-community.yml' = @{
        name = 'Community Automation'
        triggers = @('pull_request', 'issues')
        critical = $false
        expectedJobs = 3
    }
    'ci-build.yml' = @{
        name = 'Core CI Build'
        triggers = @('push')
        critical = $true
        expectedJobs = 2
    }
    'ci-pr-gates.yml' = @{
        name = 'PR Gates'
        triggers = @('pull_request')
        critical = $true
        expectedJobs = 4
    }
    'security-consolidated.yml' = @{
        name = 'Security Scanning'
        triggers = @('schedule')
        critical = $true
        expectedJobs = 5
    }
    'docker-image.yml' = @{
        name = 'Docker Image Build'
        triggers = @('push')
        critical = $false
        expectedJobs = 2
    }
}

# ═════════════════════════════════════════════════════════════════════════════════
# HILFSFUNKTIONEN
# ═════════════════════════════════════════════════════════════════════════════════

function Write-Report {
    param([string]$Message, [ValidateSet('INFO', 'PASS', 'FAIL', 'WARN')]$Level = 'INFO')
    
    $timestamp = Get-Date -Format 'HH:mm:ss'
    $prefix = switch ($Level) {
        'PASS' { '✅' }
        'FAIL' { '❌' }
        'WARN' { '⚠️ ' }
        default { 'ℹ️ ' }
    }
    
    $line = "[$timestamp] $prefix $Message"
    Write-Host $line
    $script:TestResults += $line
}

function Test-WorkflowFile {
    param(
        [string]$WorkflowPath,
        [hashtable]$Config
    )
    
    Write-Report "Teste: $($Config.name)"
    
    $testResult = @{
        file = Split-Path -Leaf $WorkflowPath
        name = $Config.name
        passed = $false
        errors = @()
        details = @{}
    }
    
    try {
        # Test 1: Datei existiert
        if (-not (Test-Path $WorkflowPath)) {
            $testResult.errors += "Datei nicht gefunden: $WorkflowPath"
            return $testResult
        }
        Write-Report "  ✓ Datei vorhanden" 'PASS'
        
        # Test 2: act kann Workflow parsen
        Write-Verbose "Führe act list aus..."
        $actOutput = & act list -W $WorkflowPath 2>&1
        if ($LASTEXITCODE -ne 0) {
            $testResult.errors += "act konnte Workflow nicht parsen: $actOutput"
            return $testResult
        }
        Write-Report "  ✓ act Parsing erfolgreich" 'PASS'
        
        # Test 3: Jobs extrahieren und zählen
        $jobCount = ($actOutput | Measure-Object -Line).Lines - 1
        if ($jobCount -le 0) {
            $testResult.errors += "Keine Jobs gefunden"
            return $testResult
        }
        Write-Report "  ✓ Jobs gefunden: $jobCount" 'PASS'
        $testResult.details['jobCount'] = $jobCount
        
        # Test 4: Trigger-Validierung
        $content = Get-Content $WorkflowPath -Raw
        $hasTriggers = $false
        foreach ($trigger in $Config.triggers) {
            if ($content -match "on:\s|$trigger") {
                $hasTriggers = $true
                break
            }
        }
        if (-not $hasTriggers) {
            Write-Report "  ⚠ Keine erwarteten Trigger gefunden" 'WARN'
        } else {
            Write-Report "  ✓ Trigger validiert" 'PASS'
        }
        
        # Test 5: YAML-Syntax
        Write-Verbose "Validiere YAML..."
        $pythonScript = @'
import yaml
import sys
try:
    with open(sys.argv[1], 'r') as f:
        yaml.safe_load(f)
    print("YAML_VALID")
except Exception as e:
    print(f"YAML_ERROR:{e}")
'@
        
        $pythonScript | Out-File -Encoding UTF8 /tmp/yaml_check.py -Force
        $yamlCheck = & python /tmp/yaml_check.py $WorkflowPath 2>&1
        
        if ($yamlCheck -match "YAML_VALID") {
            Write-Report "  ✓ YAML-Syntax korrekt" 'PASS'
            $testResult.passed = $true
        } else {
            $testResult.errors += "YAML-Fehler: $yamlCheck"
        }
        
    } catch {
        $testResult.errors += "Ausnahmefehler: $($_.Exception.Message)"
        Write-Report "  ❌ Fehler: $($_.Exception.Message)" 'FAIL'
    }
    
    return $testResult
}

function Test-WorkflowDryRun {
    param([string]$WorkflowPath)
    
    Write-Report "Starte Dry-Run: $(Split-Path -Leaf $WorkflowPath)" 'INFO'
    
    try {
        # Dry-Run mit act (ohne tatsächliche Ausführung)
        $dryRunOutput = & act --dry-run --quiet -W $WorkflowPath 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Report "  ✓ Dry-Run erfolgreich" 'PASS'
            return $true
        } else {
            Write-Report "  ⚠ Dry-Run mit Exit-Code $LASTEXITCODE" 'WARN'
            Write-Verbose "Output: $dryRunOutput"
            return $false
        }
    } catch {
        Write-Report "  ❌ Dry-Run Fehler: $($_.Exception.Message)" 'FAIL'
        return $false
    }
}

# ═════════════════════════════════════════════════════════════════════════════════
# HAUPTLOGIK
# ═════════════════════════════════════════════════════════════════════════════════

Write-Report "═══════════════════════════════════════════════════════════════" 'INFO'
Write-Report "GitHub Actions Workflow Runtime Test Suite" 'INFO'
Write-Report "Modus: $Mode | Zeitstempel: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" 'INFO'
Write-Report "═══════════════════════════════════════════════════════════════" 'INFO'

# Workflows filtern
$workflowsToTest = if ($WorkflowFilter.Count -gt 0) {
    $WorkflowTests.GetEnumerator() | Where-Object { $_.Key -in $WorkflowFilter }
} else {
    $WorkflowTests.GetEnumerator()
}

if ($workflowsToTest.Count -eq 0) {
    Write-Report "Keine Workflows zum Testen gefunden" 'FAIL'
    exit 1
}

$passCount = 0
$failCount = 0
$warnCount = 0

# Test Phase 1: Statische Analyse
Write-Report "`n▶ PHASE 1: Statische Workflow-Analyse" 'INFO'
Write-Report "─────────────────────────────────────" 'INFO'

foreach ($workflow in $workflowsToTest) {
    $workflowPath = Join-Path $WorkflowDir $workflow.Key
    $testResult = Test-WorkflowFile -WorkflowPath $workflowPath -Config $workflow.Value
    
    if ($testResult.passed) {
        $passCount++
    } else {
        if ($workflow.Value.critical) {
            $failCount++
        } else {
            $warnCount++
        }
    }
}

# Test Phase 2: Dry-Run (nur im Full-Modus)
if ($Mode -eq 'full') {
    Write-Report "`n▶ PHASE 2: Workflow Dry-Run Tests" 'INFO'
    Write-Report "─────────────────────────────────" 'INFO'
    
    foreach ($workflow in $workflowsToTest) {
        $workflowPath = Join-Path $WorkflowDir $workflow.Key
        $dryRunResult = Test-WorkflowDryRun -WorkflowPath $workflowPath
        
        if (-not $dryRunResult) {
            $warnCount++
        }
    }
}

# Test Phase 3: Trigger-Validierung
if ($Mode -eq 'debug') {
    Write-Report "`n▶ PHASE 3: Trigger-Validierung (DEBUG)" 'INFO'
    Write-Report "────────────────────────────────────" 'INFO'
    
    foreach ($workflow in $workflowsToTest) {
        $workflowPath = Join-Path $WorkflowDir $workflow.Key
        $content = Get-Content $workflowPath -Raw
        
        Write-Report "Datei: $($workflow.Key)" 'INFO'
        if ($content -match "^on:\s*$") {
            Write-Report "  on: [Block erkannt]" 'INFO'
        }
    }
}

# ═════════════════════════════════════════════════════════════════════════════════
# BERICHT
# ═════════════════════════════════════════════════════════════════════════════════

Write-Report "`n═══════════════════════════════════════════════════════════════" 'INFO'
Write-Report "ZUSAMMENFASSUNG" 'INFO'
Write-Report "─────────────────────────────────────────────────────────────" 'INFO'
Write-Report "Bestanden:  ✅ $passCount" 'PASS'
Write-Report "Fehlgeschlagen: ❌ $failCount" 'FAIL'
Write-Report "Warnungen:  ⚠️  $warnCount" 'WARN'
Write-Report "Gesamt:     $($passCount + $failCount + $warnCount)" 'INFO'

if ($failCount -eq 0) {
    Write-Report "Status: ✅ ALLE TESTS BESTANDEN" 'PASS'
} else {
    Write-Report "Status: ❌ EINIGE TESTS FEHLGESCHLAGEN" 'FAIL'
}

Write-Report "═══════════════════════════════════════════════════════════════" 'INFO'

# Bericht speichern
$script:TestResults | Out-File -Encoding UTF8 $ReportFile -Force
Write-Host "`n📄 Bericht gespeichert: $ReportFile" -ForegroundColor Cyan

exit if ($failCount -eq 0) { 0 } else { 1 }
