$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Error "cl.exe not found. Open a Developer PowerShell for VS 2022."
    exit 1
}

$files = Get-ChildItem -Path "src" -Recurse -File -Include *.c,*.cpp,*.cc,*.cxx
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}

$common = @(
    "/nologo", "/std:c++20", "/permissive-", "/EHsc", "/W4", "/utf-8",
    "/Zs", "/c", "/DWIN32", "/D_WINDOWS", "/DWIN32_LEAN_AND_MEAN", "/DNOMINMAX",
    "/wd4068", "/wd4101"  # Ignore pragma-related warnings for cleaner output
)

# Build include paths - include vcpkg and standard directories
$vcpkgRoot = Join-Path $PWD "vcpkg_installed\x64-windows\include"
$includes = @(
    "/Iinclude",
    "/Isrc",
    "/I$vcpkgRoot",
    "/external:I$vcpkgRoot",
    "/external:W0"  # Suppress warnings from external headers
)

$failed = 0
$failedFiles = @()
$errorsByModule = @{}

foreach ($f in $files) {
    $relPath = ($f.FullName).Replace("$PWD\", "")
    $moduleName = ($relPath -split "\\")[1]  # src/{moduleName}/...
    
    Write-Host ("[syntax-check] $relPath")
    $output = & cl.exe @common @includes $f.FullName 2>&1
    
    # Filter for errors in our own code (not vcpkg, external, etc.)
    $ourErrors = @()
    foreach ($line in $output) {
        $lineStr = "$line"
        # Include lines from src/, include/, but not vcpkg_installed or external
        if (($lineStr -match "src\\" -or $lineStr -match "include\\") -and $lineStr -notmatch "vcpkg" -and $lineStr -notmatch "external") {
            $ourErrors += $lineStr
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        $failed++
        $failedFiles += $relPath
        
        if (-not $errorsByModule[$moduleName]) {
            $errorsByModule[$moduleName] = @()
        }
        $errorsByModule[$moduleName] += @{ File = $relPath; Errors = $ourErrors }
        
        if ($ourErrors.Count -gt 0) {
            Write-Host "  [ERRORS in $moduleName]:"
            $ourErrors | ForEach-Object { Write-Host "    $_" }
        }
    }
}

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "=== Summary by Module ===" -ForegroundColor Red
    foreach ($module in ($errorsByModule.Keys | Sort-Object)) {
        $moduleIssues = $errorsByModule[$module]
        Write-Host "`n$module ($($moduleIssues.Count) file(s)):"
        foreach ($issue in $moduleIssues) {
            Write-Host "  - $($issue.File)"
        }
    }
    Write-Error ("Syntax-only check failed for $failed file(s). See summary above.")
    exit 1
}

Write-Host ""
Write-Host "[OK] Syntax-only check completed successfully for all modules." -ForegroundColor Green
exit 0
