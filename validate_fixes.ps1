#!/usr/bin/env pwsh
<#
.SYNOPSIS
Validiert dass die Compile-Fehler behoben wurden
#>

Write-Host "=== Validierung der Compile-Fehler-Fixes ===" -ForegroundColor Green

$errors = @()

# Check 1: ConjunctiveQuery Initialization (Zeile 5373)
Write-Host "`nCheck 1: ConjunctiveQuery Initialization..." -ForegroundColor Yellow
$line = Select-String -Path src/server/http_server.cpp -Pattern 'themis::ConjunctiveQuery q{table, preds, {}, {}, {}, {}};' | Select-Object -First 1
if ($line) {
    Write-Host "  ✓ ConjunctiveQuery hat alle 6 Parameter initialisiert" -ForegroundColor Green
} else {
    Write-Host "  ✗ ConjunctiveQuery Initialisierung NICHT KORREKT" -ForegroundColor Red
    $errors += "ConjunctiveQuery not properly initialized"
}

# Check 2: time_t Initialization (Zeile 6148)
Write-Host "`nCheck 2: time_t Variable Initialization..." -ForegroundColor Yellow
$line2 = Select-String -Path src/server/http_server.cpp -Pattern 'time_t ta = 0, tb = 0;' | Select-Object -First 1
if ($line2) {
    Write-Host "  ✓ time_t ta und tb sind mit 0 initialisiert" -ForegroundColor Green
} else {
    Write-Host "  ✗ time_t Initialisierung NICHT KORREKT" -ForegroundColor Red
    $errors += "time_t variables not properly initialized"
}

# Check 3: CMakeLists Content Prozessoren
Write-Host "`nCheck 3: CMakeLists.txt Content Processors..." -ForegroundColor Yellow
$required_sources = @(
    "src/content/mime_detector.cpp",
    "src/content/content_policy.cpp",
    "src/content/content_fs.cpp",
    "src/content/version_manager.cpp",
    "src/sharding/circuit_breaker.cpp",
    "src/sharding/gossip_protocol.cpp",
    "src/sharding/raft_configuration.cpp",
    "src/security/hsm_provider_pkcs11.cpp"
)

$cmake_content = Get-Content CMakeLists.txt -Raw
foreach ($source in $required_sources) {
    if ($cmake_content -match [regex]::Escape($source)) {
        Write-Host "  ✓ $source enthalten" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $source FEHLT" -ForegroundColor Red
        $errors += "$source missing from CMakeLists.txt"
    }
}

# Zusammenfassung
Write-Host "`n=== Validierungsergebnis ===" -ForegroundColor Cyan
if ($errors.Count -eq 0) {
    Write-Host "✓ Alle Fixes validiert! Build sollte erfolgreich sein." -ForegroundColor Green
    exit 0
} else {
    Write-Host "✗ Fehler gefunden:" -ForegroundColor Red
    $errors | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}
