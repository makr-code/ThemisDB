#!/usr/bin/env pwsh
# CMake wrapper that initializes VS2022 environment before running CMake
# This solves: "rc.exe not found", "mt.exe not found", linker issues

param(
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$CMakeArgs
)

$ErrorActionPreference = 'Stop'

# Initialize VS2022 environment
$vsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'

if (-not (Test-Path $vsDevCmd)) {
    Write-Error "VsDevCmd not found at: $vsDevCmd"
    exit 1
}

# Run VsDevCmd to set environment variables
$output = & cmd /c "`"$vsDevCmd`" -arch=x64 && set" 2>&1

# Parse and set environment variables
foreach ($line in $output -split "`n") {
    if ($line -match "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$") {
        $var_name = $matches[1]
        $var_value = $matches[2]
        if ($var_name -notmatch "^(PROMPT|ERRORLEVEL)$") {
            [System.Environment]::SetEnvironmentVariable($var_name, $var_value, "Process")
        }
    }
}

Write-Host "VS2022 environment initialized for:" -ForegroundColor Green
Write-Host "  MSVC: $(cl.exe 2>&1 | head -1)" -ForegroundColor Gray
Write-Host "  PATH includes: RC, MT, LIB, INCLUDE" -ForegroundColor Gray
Write-Host ""

# Now run CMake with all arguments
Write-Host "Running: cmake $($CMakeArgs -join ' ')" -ForegroundColor Cyan
Write-Host ""

& cmake.exe @CMakeArgs
$exitCode = $LASTEXITCODE

exit $exitCode
