# Syntax and warning check using CMake's compile_commands.json and clang-tidy
# This approach uses actual compilation database from CMake for accurate include paths

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Continue
$PSNativeCommandUseErrorActionPreference = $false

# Check for compile_commands.json
$dbPath = "build-msvc-windows-debug/compile_commands.json"
if (-not (Test-Path $dbPath)) {
    Write-Host "Attempting CMake configuration with compile_commands.json export..."
    & cmake --preset windows-release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | Select-Object -Last 20
    if (-not (Test-Path $dbPath)) {
        Write-Error "compile_commands.json not found after configure. Please run: cmake --preset windows-release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        exit 1
    }
}

Write-Host "Using compile_commands.json for syntax validation..."

# Try clang-tidy first (most reliable)
$clangTidy = $null
if (Get-Command clang-tidy -ErrorAction SilentlyContinue) {
    $clangTidy = "clang-tidy"
} else {
    # Try to find in Visual Studio LLVM bundle
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -property installationPath
        if ($vsInstall) {
            $tidyPath = Join-Path $vsInstall "VC\Tools\Llvm\x64\bin\clang-tidy.exe"
            if (Test-Path $tidyPath) {
                $clangTidy = $tidyPath
            }
        }
    }
}

if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install Visual Studio C++ Clang tools or LLVM."
    exit 1
}

Write-Host "Using clang-tidy: $clangTidy"

# Get all source files from src/
$files = Get-ChildItem -Path "src" -Recurse -File -Include *.c,*.cpp,*.cc,*.cxx
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}

$errorsByModule = @{}
$warningsByModule = @{}
$totalErrors = 0
$totalWarnings = 0
$checkedFiles = 0

foreach ($f in $files) {
    $relPath = ($f.FullName).Replace("$PWD\", "").Replace("\", "/")
    $moduleName = ($relPath -split "/")[1]
    
    $checkedFiles++
    Write-Host "[$('{0:0000}' -f $checkedFiles)] $relPath" -ForegroundColor Cyan
    
    # Run clang-tidy with quiet output
    $output = & $clangTidy $f.FullName -p build-msvc-windows-debug --header-filter="^(src|include)/" --quiet 2>&1
    
    # Parse output for errors and warnings
    $fileErrors = @()
    $fileWarnings = @()
    
    foreach ($line in $output) {
        $lineStr = "$line"
        # Skip external/vcpkg warnings
        if ($lineStr -match "vcpkg|external" -or $lineStr -eq "") {
            continue
        }
        
        if ($lineStr -match "error:") {
            $fileErrors += $lineStr
            $totalErrors++
            if (-not $errorsByModule[$moduleName]) {
                $errorsByModule[$moduleName] = @()
            }
            $errorsByModule[$moduleName] += @{ File = $relPath; Message = $lineStr }
        } elseif ($lineStr -match "warning:") {
            $fileWarnings += $lineStr
            $totalWarnings++
            if (-not $warningsByModule[$moduleName]) {
                $warningsByModule[$moduleName] = @()
            }
            $warningsByModule[$moduleName] += @{ File = $relPath; Message = $lineStr }
        }
    }
    
    # Show errors for this file
    if ($fileErrors.Count -gt 0) {
        Write-Host "  ERRORS: $($fileErrors.Count)" -ForegroundColor Red
        $fileErrors | ForEach-Object { Write-Host "    $_" }
    }
    
    # Show warnings summary (grouped by type)
    if ($fileWarnings.Count -gt 0) {
        Write-Host "  WARNINGS: $($fileWarnings.Count)" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "SUMMARY" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Files checked: $checkedFiles"
Write-Host "Total errors: $totalErrors" -ForegroundColor Red
Write-Host "Total warnings: $totalWarnings" -ForegroundColor Yellow

if ($errorsByModule.Count -gt 0) {
    Write-Host ""
    Write-Host "ERRORS BY MODULE:" -ForegroundColor Red
    foreach ($module in ($errorsByModule.Keys | Sort-Object)) {
        $issues = $errorsByModule[$module]
        Write-Host "  ${module}: $($issues.Count) error(s)"
        $issues | Group-Object { ($_.Message -split ":")[3] } | ForEach-Object {
            Write-Host "    - $($_.Name): $($_.Count) occurrences"
        }
    }
}

if ($warningsByModule.Count -gt 0) {
    Write-Host ""
    Write-Host "WARNINGS BY MODULE:" -ForegroundColor Yellow
    foreach ($module in ($warningsByModule.Keys | Sort-Object)) {
        $issues = $warningsByModule[$module]
        Write-Host "  ${module}: $($issues.Count) warning(s)"
    }
}

Write-Host ""
if ($totalErrors -gt 0) {
    Write-Error "Found $totalErrors error(s). Fix them to continue."
    exit 1
}

Write-Host "[OK] No errors found. $totalWarnings warning(s) remain." -ForegroundColor Green
exit 0
