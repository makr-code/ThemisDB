param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $false)]
    [string]$Preset = "windows-release-gate",

    [Parameter(Mandatory = $false)]
    [string]$RequiredTestsCsv = "",

    [Parameter(Mandatory = $false)]
    [string]$ForbiddenTestsCsv = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $BuildDir)) {
    throw "Build directory not found: $BuildDir"
}
if (-not (Test-Path -LiteralPath $SourceDir)) {
    throw "Source directory not found: $SourceDir"
}

$requiredTests = @("SchemaManagerFocusedTests", "ServerSchemaEndpointSmoke")
if (-not [string]::IsNullOrWhiteSpace($RequiredTestsCsv)) {
    $requiredTests = $RequiredTestsCsv.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 }
}

$forbiddenTests = @("RuntimeLicenseGateTests", "GateResultFocusedTests")
if (-not [string]::IsNullOrWhiteSpace($ForbiddenTestsCsv)) {
    $forbiddenTests = $ForbiddenTestsCsv.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 }
}

Push-Location $SourceDir
try {
    $output = & ctest --preset $Preset -N 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "ctest preset inventory failed for '$Preset'."
    }
}
finally {
    Pop-Location
}

$text = ($output | Out-String)

foreach ($name in $requiredTests) {
    if ($text -notmatch [regex]::Escape($name)) {
        throw "Missing required std-gate test: $name"
    }
}

foreach ($name in $forbiddenTests) {
    if ($text -match [regex]::Escape($name)) {
        throw "Unexpected legacy gate test leaked into std-gate preset: $name"
    }
}

$totalLine = $text | Select-String -Pattern "Total Tests:\s*(\d+)" | Select-Object -Last 1
if (-not $totalLine) {
    throw "Could not determine total test count from ctest output."
}

$total = [int]$totalLine.Matches[0].Groups[1].Value
if ($total -lt $requiredTests.Count) {
    throw "std-gate preset too small: expected at least $($requiredTests.Count), got $total"
}

Write-Host "std-gate composition verified ($total tests)."
