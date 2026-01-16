# Debug Crash Isolation Script
# Versucht, die Crash-Ursache durch Binär-Suche in abhängigen DLLs zu isolieren

param(
    [string]$BinaryPath = "C:\VCC\themis\build-vs\cmake\Release\themis_server.exe"
)

Write-Host "=== ThemisDB Crash Isolation Tool ===" -ForegroundColor Green
Write-Host "Binary: $BinaryPath`n" -ForegroundColor Cyan

# 1. Zeige alle abhängigen DLLs
Write-Host "[STEP 1] Abhängige DLLs:" -ForegroundColor Yellow
$dllList = @()
try {
    $output = & "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\..\..\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\dumpbin.exe" /dependents "$BinaryPath" 2>&1
    $dllList = $output | Where-Object { $_ -match '\.dll' } | ForEach-Object { $_.Trim() }
    if ($dllList.Count -eq 0) {
        Write-Host "  [!] dumpbin.exe konnte nicht verwendet werden. Versuche alternative Methode..." -ForegroundColor Yellow
    } else {
        $dllList | ForEach-Object { Write-Host "  - $_" }
    }
} catch {
    Write-Host "  [!] Fehler bei dumpbin: $_" -ForegroundColor Yellow
}

# 2. Test: Führe Binary aus und fange Fehler
Write-Host "`n[STEP 2] Starte Binary..." -ForegroundColor Yellow
cd (Split-Path $BinaryPath)

# Versuche mit -h (kurz)
Write-Host "`nTest 1: --help" -ForegroundColor Cyan
& $BinaryPath --help 2>&1 | Select-Object -First 5
$test1Code = $LASTEXITCODE
Write-Host "  Exit Code: $test1Code`n"

# Versuche mit --version
Write-Host "Test 2: --version" -ForegroundColor Cyan
& $BinaryPath --version 2>&1
$test2Code = $LASTEXITCODE
Write-Host "  Exit Code: $test2Code`n"

# Versuche mit --build-info
Write-Host "Test 3: --build-info" -ForegroundColor Cyan
& $BinaryPath --build-info 2>&1 | Select-Object -First 3
$test3Code = $LASTEXITCODE
Write-Host "  Exit Code: $test3Code`n"

# 3. Entferne DLLs eine nach der anderen und teste
Write-Host "[STEP 3] Entferne DLLs und teste Verhalten..." -ForegroundColor Yellow

$testDlls = @(
    "llama.dll",
    "zstd.dll",
    "lz4.dll"
)

foreach ($dll in $testDlls) {
    $dllPath = Join-Path (Split-Path $BinaryPath) $dll
    if (Test-Path $dllPath) {
        $backupPath = "$dllPath.bak"
        Move-Item $dllPath $backupPath -Force -ErrorAction SilentlyContinue
        
        Write-Host "  Entfernt: $dll" -ForegroundColor Cyan
        & $BinaryPath --version 2>&1 | Out-Null
        $code = $LASTEXITCODE
        Write-Host "    Exit Code ohne $dll : $code" -ForegroundColor $(if ($code -eq -1073741502) { 'Yellow' } else { 'Green' })
        
        Move-Item $backupPath $dllPath -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`n[STEP 4] Analyse-Ergebnis:" -ForegroundColor Yellow
Write-Host "  - Wenn alle Tests 0xC0000142 zeigen: Problem ist in statischer Initialisierung" -ForegroundColor Green
Write-Host "  - Wenn Exit Code sich ändert: Problem liegt in der entfernten DLL" -ForegroundColor Green
Write-Host "`nVerschiebe Binary in Verzeichnis ohne DLLs und teste..." -ForegroundColor Cyan

$testDir = "C:\VCC\themis\build-vs\cmake\Release\test-no-deps"
if (Test-Path $testDir) { Remove-Item $testDir -Recurse -Force }
New-Item $testDir -ItemType Directory | Out-Null
Copy-Item $BinaryPath "$testDir\themis_server.exe"

cd $testDir
Write-Host "  Test ohne externe DLLs:" -ForegroundColor Cyan
.\themis_server.exe --version 2>&1
$noDepsCode = $LASTEXITCODE
Write-Host "  Exit Code: $noDepsCode"

cd (Split-Path $BinaryPath)
Write-Host "`n[FERTIG]" -ForegroundColor Green
