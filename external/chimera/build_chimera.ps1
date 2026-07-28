$paramBlock = @()
param(
  [string]$BuildDir
)

$ErrorActionPreference = 'Stop'

if (-not $BuildDir) { $BuildDir = Join-Path $PSScriptRoot 'build' }
$Generator = "Ninja"
$CMakeArgs = ""

if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
Push-Location $BuildDir

# Allow user to pass a toolchain file via environment variable (e.g. vcpkg toolchain)
$toolchain = $env:VCPKG_TOOLCHAIN_FILE
if ($null -ne $toolchain -and $toolchain -ne "") {
  & cmake .. -G $Generator -DCMAKE_TOOLCHAIN_FILE=$toolchain $CMakeArgs
} else {
  & cmake .. -G $Generator $CMakeArgs
}

if ($LASTEXITCODE -ne 0) { Pop-Location; throw "cmake configure failed" }

& cmake --build . --config Release --parallel
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "build failed" }

Pop-Location
Write-Host "Chimera built in $BuildDir"
