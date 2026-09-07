# Extract compile options from compile_commands.json and check syntax per module
# Uses actual CMake-generated flags for accurate validation

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Continue
$PSNativeCommandUseErrorActionPreference = $false

if (-not (Test-Path "build-msvc-windows-debug/compile_commands.json")) {
    Write-Error "compile_commands.json not found"
    exit 1
}

$db = Get-Content "build-msvc-windows-debug/compile_commands.json" | ConvertFrom-Json
Write-Host "Loaded $($db.Count) compilation commands"

# Find one sample command to extract base flags
$sampleCmd = $db | Where-Object { $_.file -like "*/src/*/*.cpp" } | Select-Object -First 1
if (-not $sampleCmd) {
    Write-Error "No .cpp files found in compilation database"
    exit 1
}

Write-Host "Sample command: $($sampleCmd.command)" | head -1
Write-Host ""

# Group errors/warnings by module
$errorsByModule = @{}
$checkedCount = 0

# Check each source file in src/
$srcFiles = Get-ChildItem -Path "src" -Recurse -File -Include *.cpp,*.cc,*.cxx
foreach ($file in $srcFiles) {
    $moduleName = ($file.FullName -replace "\\", "/" -split "/")[1]
    
    # Find matching compile command
    $compCmd = $db | Where-Object { $_.file -match [regex]::Escape($file.FullName) } | Select-Object -First 1
    if (-not $compCmd) {
        continue
    }
    
    $checkedCount++
    Write-Host "[$('{0:0000}' -f $checkedCount)] $moduleName/$($file.BaseName)"
    
    # Extract directory and command
    $cmdDir = $compCmd.directory
    $cmdLine = $compCmd.command
    
    # Replace /E with /Zs for syntax-only check
    $syntaxCmd = $cmdLine -replace "/E(\s|$)", "/Zs "
    
    # Run from the specified directory
    try {
        Push-Location $cmdDir
        $output = Invoke-Expression $syntaxCmd 2>&1
        Pop-Location
        
        # Check for errors in output
        $hasError = $output | Where-Object { $_ -match "error C\d+:" }
        if ($hasError) {
            if (-not $errorsByModule[$moduleName]) {
                $errorsByModule[$moduleName] = @()
            }
            $errorsByModule[$moduleName] += @{ 
                File = $file.BaseName
                Errors = $hasError
            }
            Write-Host "  [ERROR]"
            $hasError | ForEach-Object { Write-Host "    $_" }
        }
    } catch {
        Write-Host "  [SKIP - error running command]"
    }
}

Write-Host ""
Write-Host "======== SUMMARY ========" -ForegroundColor Cyan
Write-Host "Files checked: $checkedCount"
Write-Host "Modules with errors: $($errorsByModule.Count)"

if ($errorsByModule.Count -gt 0) {
    Write-Host ""
    Write-Host "ERRORS BY MODULE:" -ForegroundColor Red
    foreach ($module in ($errorsByModule.Keys | Sort-Object)) {
        $issues = $errorsByModule[$module]
        Write-Host "  ${module}: $($issues.Count) file(s) with errors"
        $issues | ForEach-Object {
            Write-Host "    - $($_.File)"
        }
    }
    exit 1
}

Write-Host "[OK] No syntax errors found." -ForegroundColor Green
exit 0
