#!/usr/bin/env pwsh
# CMake wrapper that initializes a selected Visual Studio environment before running CMake
# This solves: "rc.exe not found", "mt.exe not found", linker issues

param(
    [ValidateSet('vs2022', 'vs2026-insiders')]
    [string]$VisualStudio = 'vs2022',

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$CMakeArgs
)

$ErrorActionPreference = 'Stop'

$conflictingVsVars = @(
    'VSINSTALLDIR',
    'VCToolsVersion',
    'VCToolsInstallDir',
    'VisualStudioVersion',
    'DevEnvDir',
    'VS160COMNTOOLS',
    'VS170COMNTOOLS',
    'VS180COMNTOOLS',
    'VSCMD_VER'
)

$visualStudioOptions = @{
    'vs2022' = @{
        Label = 'VS2022'
        CandidatePaths = @(
            'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat',
            'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat',
            'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat',
            'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
        )
    }
    'vs2026-insiders' = @{
        Label = 'VS2026 Insiders'
        CandidatePaths = @(
            'C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat'
        )
    }
}

$selectedVisualStudio = $visualStudioOptions[$VisualStudio]
$vsDevCmd = $selectedVisualStudio.CandidatePaths | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $vsDevCmd) {
    Write-Error "$($selectedVisualStudio.Label) developer shell not found. Checked: $($selectedVisualStudio.CandidatePaths -join '; ')"
    exit 1
}

# Run VsDevCmd to set environment variables. Reset pre-existing VS shell markers first so
# a VS Insiders shell cannot poison the VS2022 toolchain selection.
$bootstrapScript = Join-Path ([System.IO.Path]::GetTempPath()) ("themis-vsdev-" + [System.Guid]::NewGuid().ToString('N') + ".cmd")
$bootstrapLines = @('@echo off')
$bootstrapLines += $conflictingVsVars | ForEach-Object { "set $_=" }
$bootstrapLines += @(
    "call `"$vsDevCmd`" -arch=x64",
    'set'
)
Set-Content -Path $bootstrapScript -Value $bootstrapLines -Encoding Ascii

try {
    $output = & cmd /d /c $bootstrapScript 2>&1
}
finally {
    Remove-Item -Path $bootstrapScript -Force -ErrorAction SilentlyContinue
}

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

$requiredVars = @('LIB', 'INCLUDE', 'LIBPATH', 'WindowsSdkDir', 'VCToolsInstallDir')
$missingVars = $requiredVars | Where-Object { [string]::IsNullOrWhiteSpace((Get-Item -Path "Env:$_" -ErrorAction SilentlyContinue).Value) }

if ($missingVars.Count -gt 0) {
    Write-Error "$($selectedVisualStudio.Label) developer environment is incomplete. Missing: $($missingVars -join ', '). VsDevCmd.bat did not export the linker/compiler paths required by Ninja/MSVC builds."
    exit 1
}

$compiler = Get-Command cl.exe -ErrorAction Stop
$compilerVersion = (Get-Item $compiler.Source).VersionInfo.ProductVersion

Write-Host "$($selectedVisualStudio.Label) environment initialized for:" -ForegroundColor Green
Write-Host "  MSVC: $($compiler.Source) ($compilerVersion)" -ForegroundColor Gray
Write-Host "  PATH includes: RC, MT, LIB, INCLUDE" -ForegroundColor Gray
Write-Host ""

# Now run CMake with all arguments
Write-Host "Running: cmake $($CMakeArgs -join ' ')" -ForegroundColor Cyan
Write-Host ""

& cmake.exe @CMakeArgs
$exitCode = $LASTEXITCODE

exit $exitCode
