param(
    [string]$InstallRoot = $PSScriptRoot,
    [switch]$Persist
)

$resolvedRoot = (Resolve-Path -Path $InstallRoot).Path
$binPath = Join-Path $resolvedRoot "bin"
$configPath = Join-Path $resolvedRoot "config"
$dataPath = Join-Path $resolvedRoot "data"
$modelsPath = Join-Path $resolvedRoot "models"

if (-not (Test-Path $binPath)) {
    throw "Bin directory not found: $binPath"
}

$env:THEMIS_HOME = $resolvedRoot
$env:THEMIS_CONFIG_DIR = $configPath
$env:THEMIS_DATA_DIR = $dataPath
$env:THEMIS_MODELS_DIR = $modelsPath

if (-not ($env:PATH -split ';' | Where-Object { $_ -eq $binPath })) {
    $env:PATH = "$binPath;$($env:PATH)"
}

if ($Persist) {
    [Environment]::SetEnvironmentVariable("THEMIS_HOME", $resolvedRoot, "User")
    [Environment]::SetEnvironmentVariable("THEMIS_CONFIG_DIR", $configPath, "User")
    [Environment]::SetEnvironmentVariable("THEMIS_DATA_DIR", $dataPath, "User")
    [Environment]::SetEnvironmentVariable("THEMIS_MODELS_DIR", $modelsPath, "User")

    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ([string]::IsNullOrEmpty($userPath)) {
        $userPath = $binPath
    } elseif (-not ($userPath -split ';' | Where-Object { $_ -eq $binPath })) {
        $userPath = "$binPath;$userPath"
    }
    [Environment]::SetEnvironmentVariable("Path", $userPath, "User")
}

Write-Host "Themis runtime environment configured"
Write-Host "  THEMIS_HOME=$env:THEMIS_HOME"
Write-Host "  THEMIS_CONFIG_DIR=$env:THEMIS_CONFIG_DIR"
Write-Host "  THEMIS_DATA_DIR=$env:THEMIS_DATA_DIR"
Write-Host "  THEMIS_MODELS_DIR=$env:THEMIS_MODELS_DIR"
Write-Host "  PATH includes: $binPath"
if ($Persist) {
    Write-Host "Persisted to user environment variables."
}
