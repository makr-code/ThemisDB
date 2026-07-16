<#
.SYNOPSIS
    Robust MSVC build helper for ThemisDB on Windows.

.DESCRIPTION
    This script prepares a clean MSVC build environment in the current PowerShell session,
    removing known interfering PATH entries (like GNU coreutils), invoking the Visual
    Studio "vcvarsall.bat" to initialize compiler environment, validating LIB/INCLUDE
    and tool locations, and then running CMake configure + build using the requested
    preset. If vcvars initialization fails the script writes a detailed trace file
    for diagnosis.

.USAGE
    # Use default preset (windows-release):
    .\msvc_build_helper.ps1

    # Use a different preset:
    .\msvc_build_helper.ps1 -CMakePreset "vscode-windows-release-hyperscaler"

#>

param(
    [string]$CMakePreset = 'windows-release',
    [int]$Jobs = 16,
    [switch]$KeepCoreutilsInPath,
    [switch]$TraceOnFail
)

function Write-ErrAndExit($msg, $code=1) {
    Write-Host "ERROR: $msg" -ForegroundColor Red
    exit $code
}

Write-Host "msvc_build_helper: preset=$CMakePreset jobs=$Jobs"

# 1) Temporarily sanitise PATH in this session to avoid coreutils/link.exe conflicts
if (-not $KeepCoreutilsInPath) {
    $origPath = $env:Path
    $pathParts = $env:Path -split ';' | Where-Object { ($_ -ne '') -and ($_ -notmatch 'coreutils') }
    $env:Path = ($pathParts -join ';')
    Write-Host "Sanitised PATH for this session (coreutils removed)."
}

# 2) Locate vcvarsall
$vcvars = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat'
if (-not (Test-Path $vcvars)) {
    Write-ErrAndExit "vcvarsall.bat not found at $vcvars. Please adjust path or open 'x64 Native Tools Command Prompt for VS 2022'."
}

# 3) Run vcvarsall inside cmd and capture LIB/INCLUDE and tool locations
$cmd = "call `"$vcvars`" amd64 && echo __LIB__ && echo %LIB% && echo __INCLUDE__ && echo %INCLUDE% && where cl && where link"
$envOutput = & cmd /c $cmd 2>&1

Write-Host "--- vcvars output (truncated) ---"
$envOutput | Select-Object -First 200 | ForEach-Object { Write-Host $_ }
Write-Host "--- end vcvars output ---"

# Parse LIB/INCLUDE lines
$libLine = ($envOutput | Where-Object { $_ -match '^__LIB__$' } )
if ($libLine) {
    # LIB appears after the marker
    $idx = [Array]::IndexOf($envOutput, '__LIB__')
    $libVal = $envOutput[$idx + 1]
} else {
    $libVal = ''
}

$incLine = ($envOutput | Where-Object { $_ -match '^__INCLUDE__$' } )
if ($incLine) {
    $idx2 = [Array]::IndexOf($envOutput, '__INCLUDE__')
    $incVal = $envOutput[$idx2 + 1]
} else {
    $incVal = ''
}

if ([string]::IsNullOrWhiteSpace($libVal) -or [string]::IsNullOrWhiteSpace($incVal)) {
    Write-Host "vcvars did not populate LIB/INCLUDE in this automation session." -ForegroundColor Yellow
    if ($TraceOnFail) {
        Write-Host "Generating detailed vsdevcmd.trace.txt in C:\temp for diagnosis..."
        if (-not (Test-Path 'C:\temp')) { New-Item -Path 'C:\temp' -ItemType Directory | Out-Null }
        # produce trace
        & cmd /c "set VSCMD_DEBUG=3 && call `"$vcvars`" amd64 > C:\temp\vsdevcmd.trace.txt 2>&1"
        Write-Host "Wrote C:\temp\vsdevcmd.trace.txt"
    }
    Write-ErrAndExit "vcvars initialization incomplete. Open 'x64 Native Tools Command Prompt for VS 2022' and retry or run this script with -TraceOnFail to gather diagnostics." 2
}

Write-Host "LIB and INCLUDE appear set. Continuing with CMake configure/build."
Write-Host "LIB: $libVal"
Write-Host "INCLUDE: $incVal"

# 4) Run cmake configure and build with the chosen preset
Write-Host "Running: cmake --preset $CMakePreset"
& cmake --preset $CMakePreset
if ($LASTEXITCODE -ne 0) { Write-ErrAndExit "cmake configure failed (preset=$CMakePreset)." 3 }

Write-Host "Running: cmake --build --preset $CMakePreset -- -j $Jobs"
& cmake --build --preset $CMakePreset -- -j $Jobs
if ($LASTEXITCODE -ne 0) { Write-ErrAndExit "Build failed. Inspect output above." 4 }

Write-Host "Build finished successfully." -ForegroundColor Green
