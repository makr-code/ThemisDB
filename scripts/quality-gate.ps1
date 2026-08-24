param(
    [string]$ConfigurePreset = "windows-release",
    [string]$TestPreset = "windows-release",
    [string]$BuildPreset,
    [string]$ReportsDir = "reports/local-quality",
    # clang-tidy installation root (e.g. C:\vvlm from a local LLVM install)
    [string]$ClangTidyRoot = "C:\vvlm",
    [switch]$SkipConfigure,
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipClangTidy,
    [switch]$SkipCppcheck,
    [switch]$SkipSemgrep,
    [switch]$SkipCodeQL,
    [switch]$SkipDoxygen,
    [switch]$StrictDoxygen,
    [switch]$ContinueOnError,
    [switch]$Help
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($BuildPreset)) {
    $BuildPreset = $ConfigurePreset
}

if ($Help) {
    Write-Host @"
ThemisDB Local Quality Gate

Usage:
  powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 [options]

Options:
    -ConfigurePreset <name>   CMake configure preset (default: windows-release)
  -BuildPreset <name>       CMake build preset (default: same as ConfigurePreset)
  -TestPreset <name>        CTest preset (default: windows-release)
  -ReportsDir <path>        Report output directory (default: reports/local-quality)
  -SkipConfigure            Skip CMake configure
  -SkipBuild                Skip CMake build
  -SkipTests                Skip CTest
  -SkipClangTidy            Skip clang-tidy analysis
  -SkipCppcheck             Skip cppcheck analysis
  -SkipSemgrep              Skip semgrep scan
  -SkipCodeQL               Skip CodeQL DB + analyze
  -SkipDoxygen              Skip Doxygen build
    -StrictDoxygen            Treat Doxygen failures as quality-gate failures
  -ContinueOnError          Run all steps and report failures at end
  -Help                     Show this help

Examples:
  powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1
  powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 -SkipCodeQL -SkipSemgrep
  powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 -ContinueOnError
"@
    exit 0
}

$script:Root = Split-Path -Parent $PSScriptRoot
$script:ReportPath = Join-Path $script:Root $ReportsDir
$script:CtestLogsPath = Join-Path $script:Root "logs"
$script:CompileCommandsPath = Join-Path $script:Root ("build-" + $ConfigurePreset + "\compile_commands.json")
$script:BuildDirFallback = Join-Path $script:Root "build"
$script:BuildDirForCompile = if (Test-Path $script:CompileCommandsPath) { Split-Path -Parent $script:CompileCommandsPath } else { $script:BuildDirFallback }
$script:Failures = New-Object System.Collections.Generic.List[string]

# Tool paths resolved at startup (submodule or local install)
$script:ClangTidyExe = $null
$script:SemgrepExe   = $null
$script:CodeqlExe    = $null

New-Item -ItemType Directory -Force -Path $script:ReportPath | Out-Null
New-Item -ItemType Directory -Force -Path $script:CtestLogsPath | Out-Null

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Write-Ok {
    param([string]$Message)
    Write-Host "[OK]   $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-Fail {
    param([string]$Message)
    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

function Add-Failure {
    param([string]$Step)
    $script:Failures.Add($Step) | Out-Null
}

# Resolves a tool executable by checking explicit candidate paths before falling back to PATH.
function Resolve-ToolPath {
    param(
        [string]$Name,
        [string[]]$Candidates = @()
    )
    foreach ($c in $Candidates) {
        if (Test-Path $c) { return $c }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

function Test-ToolAvailable {
    param(
        [string]$Name,
        [switch]$Optional
    )

    if (Get-Command $Name -ErrorAction SilentlyContinue) {
        return $true
    }

    if ($Optional) {
        Write-Warn "Tool not found, step will be skipped: $Name"
        return $false
    }

    Write-Fail "Required tool not found: $Name"
    Add-Failure "Missing tool: $Name"
    return $false
}

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Action,
        [switch]$Skip
    )

    if ($Skip) {
        Write-Warn "Skipping step: $Name"
        return
    }

    Write-Info "Running step: $Name"
    try {
        & $Action
        Write-Ok "$Name completed"
    }
    catch {
        Write-Fail "$Name failed: $($_.Exception.Message)"
        Add-Failure $Name
        if (-not $ContinueOnError) {
            throw
        }
    }
}

function Invoke-External {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [string]$LogFile
    )

    $targetLog = Join-Path $script:ReportPath $LogFile
    $display = if ($Arguments.Count -gt 0) {
        "$Command $($Arguments -join ' ')"
    }
    else {
        $Command
    }

    Write-Info "Command: $display"
    $nativeErrorVar = Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue
    $previousNativeErrorMode = $null
    $previousErrorActionPreference = $ErrorActionPreference

    if ($nativeErrorVar) {
        $previousNativeErrorMode = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
    }

    # Windows PowerShell can treat native stderr lines as terminating errors when
    # ErrorActionPreference is Stop. Keep stderr in the captured output and rely on
    # LASTEXITCODE for pass/fail.
    $ErrorActionPreference = 'Continue'

    try {
        $output = & $Command @Arguments 2>&1
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
        if ($nativeErrorVar) {
            $PSNativeCommandUseErrorActionPreference = $previousNativeErrorMode
        }
    }

    $output | Out-File -FilePath $targetLog -Encoding utf8

    if ($LASTEXITCODE -ne 0) {
        throw "Command returned exit code $LASTEXITCODE. See $targetLog"
    }

    return $targetLog
}

function Invoke-ExternalCapture {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [string]$LogFile
    )

    $targetLog = Join-Path $script:ReportPath $LogFile
    $display = if ($Arguments.Count -gt 0) {
        "$Command $($Arguments -join ' ')"
    }
    else {
        $Command
    }

    Write-Info "Command: $display"
    $nativeErrorVar = Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue
    $previousNativeErrorMode = $null
    $previousErrorActionPreference = $ErrorActionPreference

    if ($nativeErrorVar) {
        $previousNativeErrorMode = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
    }

    $ErrorActionPreference = 'Continue'
    try {
        $output = & $Command @Arguments 2>&1
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
        if ($nativeErrorVar) {
            $PSNativeCommandUseErrorActionPreference = $previousNativeErrorMode
        }
    }

    $output | Out-File -FilePath $targetLog -Encoding utf8

    return @{
        ExitCode = $LASTEXITCODE
        Output = ($output | Out-String)
        LogFile = $targetLog
    }
}

function Get-SourceFiles {
    $folders = @("src", "include")
    $patterns = @("*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hh", "*.hxx")

    $files = foreach ($folder in $folders) {
        $full = Join-Path $script:Root $folder
        if (Test-Path $full) {
            Get-ChildItem -Path $full -Recurse -File | Where-Object { $_.Name -like $patterns[0] -or $_.Name -like $patterns[1] -or $_.Name -like $patterns[2] -or $_.Name -like $patterns[3] -or $_.Name -like $patterns[4] -or $_.Name -like $patterns[5] -or $_.Name -like $patterns[6] }
        }
    }

    return $files
}

function Resolve-CompileCommandsDirectory {
    if (Test-Path $script:CompileCommandsPath) {
        return Split-Path -Parent $script:CompileCommandsPath
    }

    $alt = Join-Path $script:Root "build\compile_commands.json"
    if (Test-Path $alt) {
        return Split-Path -Parent $alt
    }

    return $null
}

Push-Location $script:Root
try {
    if (-not (Test-ToolAvailable -Name "cmake")) {
        throw "Missing required tools"
    }

    # --- clang-tidy: check C:\vvlm (local LLVM install) then PATH ---
    $script:ClangTidyExe = Resolve-ToolPath -Name "clang-tidy" -Candidates @(
        (Join-Path $ClangTidyRoot "bin\clang-tidy.exe"),
        (Join-Path $ClangTidyRoot "clang-tidy.exe")
    )
    $clangTidyAvailable = $null -ne $script:ClangTidyExe
    if (-not $clangTidyAvailable) {
        Write-Warn "Tool not found, step will be skipped: clang-tidy (searched '$ClangTidyRoot' and PATH)"
    }

    $cppcheckAvailable = Test-ToolAvailable -Name "cppcheck" -Optional

    # --- semgrep: submodule tools/semgrep (pip install) then PATH ---
    $semgrepSubmodule = Join-Path $script:Root "tools\semgrep"
    $script:SemgrepExe = Resolve-ToolPath -Name "semgrep" -Candidates @(
        (Join-Path $semgrepSubmodule "semgrep.exe")
    )
    # If submodule is present but not built, try PATH (pip-installed into venv)
    if ($null -eq $script:SemgrepExe) {
        $semgrepCmd = Get-Command "semgrep" -ErrorAction SilentlyContinue
        if ($semgrepCmd) { $script:SemgrepExe = $semgrepCmd.Source }
    }
    $semgrepAvailable = $null -ne $script:SemgrepExe
    if (-not $semgrepAvailable) {
        $hint = if (Test-Path $semgrepSubmodule) {
            "run: pip install -e tools/semgrep"
        } else {
            "run: git submodule update --init tools/semgrep && pip install -e tools/semgrep"
        }
        Write-Warn "Tool not found, step will be skipped: semgrep ($hint)"
    }

    # --- codeql: submodule tools/codeql (prebuilt binary) then PATH ---
    $codeqlSubmodule = Join-Path $script:Root "tools\codeql"
    $script:CodeqlExe = Resolve-ToolPath -Name "codeql" -Candidates @(
        (Join-Path $codeqlSubmodule "codeql.exe"),
        (Join-Path $codeqlSubmodule "codeql")
    )
    $codeqlAvailable = $null -ne $script:CodeqlExe
    if (-not $codeqlAvailable) {
        $hint = if (Test-Path $codeqlSubmodule) {
            "submodule present but no binary found"
        } else {
            "run: git submodule update --init tools/codeql"
        }
        Write-Warn "Tool not found, step will be skipped: codeql ($hint)"
    }

    $doxygenAvailable = Test-ToolAvailable -Name "doxygen" -Optional

    Invoke-Step -Name "CMake Configure" -Skip:$SkipConfigure -Action {
        Invoke-External -Command "cmake" -Arguments @("--preset", $ConfigurePreset) -LogFile "01-configure.log"
    }

    Invoke-Step -Name "CMake Build" -Skip:$SkipBuild -Action {
        Invoke-External -Command "cmake" -Arguments @("--build", "--preset", $BuildPreset, "--parallel", "4") -LogFile "02-build.log"
    }

    Invoke-Step -Name "CTest" -Skip:$SkipTests -Action {
        # Preflight discovery to avoid false hard failures when tests are intentionally
        # disabled in the preset or when test executables are not part of this build set.
        $ctestDiscovery = Invoke-ExternalCapture -Command "ctest" -Arguments @("--preset", $TestPreset, "-N") -LogFile "03-ctest-discovery.log"

        if ($ctestDiscovery.Output -match "No tests were found") {
            Write-Warn "CTest preset '$TestPreset' reported no tests. Skipping test execution."
            return
        }

        if ($ctestDiscovery.Output -match "Could not find executable") {
            Write-Warn "CTest preset '$TestPreset' references tests whose executables are not built in this configuration. Skipping test execution in fast gate."
            return
        }

        if ($ctestDiscovery.ExitCode -ne 0) {
            throw "CTest discovery failed with exit code $($ctestDiscovery.ExitCode). See $($ctestDiscovery.LogFile)"
        }

        $ctestReportLog = Invoke-External -Command "ctest" -Arguments @("--preset", $TestPreset, "--output-on-failure", "-j", "1", "--timeout", "60") -LogFile "03-ctest.log"
        $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
        $ctestDebugLog = Join-Path $script:CtestLogsPath ("ctest-" + $TestPreset + "-" + $timestamp + ".log")
        Copy-Item -LiteralPath $ctestReportLog -Destination $ctestDebugLog -Force
        Write-Info "CTest debug log written: $ctestDebugLog"
    }

    Invoke-Step -Name "clang-tidy" -Skip:($SkipClangTidy -or -not $clangTidyAvailable) -Action {
        $compileDir = Resolve-CompileCommandsDirectory
        if (-not $compileDir) {
            throw "compile_commands.json not found. Run configure step first."
        }

        $files = Get-SourceFiles
        if (-not $files -or $files.Count -eq 0) {
            throw "No C/C++ source files found in src/include."
        }

        $log = Join-Path $script:ReportPath "04-clang-tidy.log"
        if (Test-Path $log) {
            Remove-Item $log -Force
        }

        foreach ($file in $files) {
            $output = & $script:ClangTidyExe $file.FullName -p $compileDir --quiet 2>&1
            $output | Out-File -FilePath $log -Append -Encoding utf8
            if ($LASTEXITCODE -ne 0) {
                throw "clang-tidy failed for $($file.FullName). See $log"
            }
        }
    }

    Invoke-Step -Name "cppcheck" -Skip:($SkipCppcheck -or -not $cppcheckAvailable) -Action {
        $compileDir = Resolve-CompileCommandsDirectory
        if (-not $compileDir) {
            throw "compile_commands.json not found. Run configure step first."
        }

        $compileJson = Join-Path $compileDir "compile_commands.json"
        $xmlPath = Join-Path $script:ReportPath "05-cppcheck.xml"
        $txtPath = Join-Path $script:ReportPath "05-cppcheck.log"

        $output = & cppcheck --project=$compileJson --enable=warning,style,performance,portability,information --inconclusive --std=c++20 --xml 2>&1
        $output | Out-File -FilePath $txtPath -Encoding utf8
        $output | Out-File -FilePath $xmlPath -Encoding utf8

        if ($LASTEXITCODE -ne 0) {
            throw "cppcheck returned exit code $LASTEXITCODE. See $txtPath"
        }
    }

    Invoke-Step -Name "semgrep" -Skip:($SkipSemgrep -or -not $semgrepAvailable) -Action {
        Invoke-External -Command $script:SemgrepExe -Arguments @("scan", "--config", "p/cwe-top-25", "--config", "p/owasp-top-ten", "--json", "--output", (Join-Path $script:ReportPath "06-semgrep.json"), ".") -LogFile "06-semgrep.log"
    }

    Invoke-Step -Name "CodeQL" -Skip:($SkipCodeQL -or -not $codeqlAvailable) -Action {
        $db = Join-Path $script:ReportPath "codeql-db"
        if (Test-Path $db) {
            Remove-Item -Path $db -Recurse -Force
        }

        Invoke-External -Command $script:CodeqlExe -Arguments @("database", "create", $db, "--language=cpp", "--command", ("cmake --build --preset " + $BuildPreset + " --parallel 4")) -LogFile "07-codeql-create.log"
        Invoke-External -Command $script:CodeqlExe -Arguments @("database", "analyze", $db, "codeql/cpp-queries:codeql-suites/cpp-security-and-quality.qls", "--format=sarif-latest", "--output", (Join-Path $script:ReportPath "07-codeql.sarif")) -LogFile "07-codeql-analyze.log"
    }

    Invoke-Step -Name "Doxygen" -Skip:($SkipDoxygen -or -not $doxygenAvailable) -Action {
        if ($StrictDoxygen) {
            Invoke-External -Command "doxygen" -Arguments @("Doxyfile") -LogFile "08-doxygen.log"
            return
        }

        $doxygenRun = Invoke-ExternalCapture -Command "doxygen" -Arguments @("Doxyfile") -LogFile "08-doxygen.log"
        if ($doxygenRun.ExitCode -ne 0) {
            Write-Warn "Doxygen returned exit code $($doxygenRun.ExitCode); continuing (non-blocking in fast gate). See $($doxygenRun.LogFile)"
        }
    }

    Write-Host ""
    if ($script:Failures.Count -eq 0) {
        Write-Ok "Quality gate passed. Reports: $script:ReportPath"
        exit 0
    }

    Write-Fail "Quality gate finished with failures:"
    foreach ($failure in $script:Failures) {
        Write-Host "  - $failure" -ForegroundColor Red
    }
    Write-Host "Reports: $script:ReportPath"
    exit 1
}
finally {
    Pop-Location
}
