# Extract compile warnings and errors from CMake build output
# Filters to only show issues in src/ modules (not vcpkg, external)

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Continue
$PSNativeCommandUseErrorActionPreference = $false

$buildDir = "build-msvc-windows-release"
if (-not (Test-Path $buildDir)) {
    Write-Host "Build directory not found. Running CMake configure..."
    & cmake --preset windows-release 2>&1 | tail -20
}

Write-Host "Running clean build to capture all warnings..."
$buildOutput = & cmake --build $buildDir --parallel 16 2>&1
$warningsByModule = @{}
$errorsByModule = @{}

# Process output line by line
$currentModule = "unknown"
foreach ($line in $buildOutput) {
    $lineStr = "$line"
    
    # Skip noise
    if ($lineStr -eq "" -or $lineStr -match "^\[|^Built|^Linking|^Consolidating") {
        continue
    }
    
    # Detect module from file path
    if ($lineStr -match "src\\([a-z_]+)" ) {
        $currentModule = $matches[1]
    }
    
    # Detect warnings and errors in our source code (not vcpkg/external)
    if ($lineStr -match "warning:|error:" -and $lineStr -match "src\\" -and $lineStr -notmatch "vcpkg|external") {
        if ($lineStr -match "error:") {
            if (-not $errorsByModule[$currentModule]) {
                $errorsByModule[$currentModule] = @()
            }
            $errorsByModule[$currentModule] += $lineStr
            Write-Host "[ERROR] $lineStr" -ForegroundColor Red
        } else {
            if (-not $warningsByModule[$currentModule]) {
                $warningsByModule[$currentModule] = @()
            }
            $warningsByModule[$currentModule] += $lineStr
            Write-Host "[WARN] $lineStr" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "======== SUMMARY ========" -ForegroundColor Cyan

if ($errorsByModule.Count -gt 0) {
    Write-Host "ERRORS BY MODULE:" -ForegroundColor Red
    foreach ($module in ($errorsByModule.Keys | Sort-Object)) {
        Write-Host "  ${module}: $($errorsByModule[$module].Count) error(s)"
    }
}

if ($warningsByModule.Count -gt 0) {
    Write-Host "WARNINGS BY MODULE:" -ForegroundColor Yellow
    foreach ($module in ($warningsByModule.Keys | Sort-Object)) {
        Write-Host "  ${module}: $($warningsByModule[$module].Count) warning(s)"
    }
}

if ($errorsByModule.Count -gt 0) {
    Write-Error "Found errors in build. See details above."
    exit 1
}

Write-Host ""
Write-Host "[OK] Build completed." -ForegroundColor Green
if ($warningsByModule.Count -gt 0) {
    Write-Host "  $($warningsByModule.Values | Measure-Object -Sum).Sum warnings remain in $($warningsByModule.Count) module(s)."
}
exit 0
