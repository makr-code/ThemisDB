$ErrorActionPreference = 'Stop'

Write-Host '==> Starting themis_server (LLM) build'

Push-Location 'C:\VCC\themis'

if (-not (Test-Path 'C:\VCC\themis\llama.cpp\CMakeLists.txt')) {
  Write-Host 'Cloning llama.cpp into root'
  git clone https://github.com/ggerganov/llama.cpp.git 'C:\VCC\themis\llama.cpp'
}

Write-Host 'Configuring CMake (Visual Studio 2022, x64, LLM enabled)'
cmake -S . -B build-msvc -G 'Visual Studio 17 2022' -A x64 `
  -DTHEMIS_ENABLE_LLM=ON `
  -DVCPKG_ROOT='C:\VCC\themis\vcpkg' `
  -DCMAKE_TOOLCHAIN_FILE='C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake'

if ($LASTEXITCODE -ne 0) {
  throw 'CMake configure failed'
}

Write-Host 'Building themis_server (Release)'
cmake --build build-msvc --config Release --target themis_server --parallel 8

if ($LASTEXITCODE -ne 0) {
  throw 'Build failed'
}

$exe = 'C:\VCC\themis\build-msvc\Release\themis_server.exe'
if (Test-Path $exe) {
  Write-Host '==> themis_server built successfully. Printing --help'
  & $exe --help | Out-String | Write-Host
} else {
  Write-Warning 'themis_server.exe not found in Release output.'
}

Pop-Location