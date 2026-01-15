param(
    [int]$Jobs = 8,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Push-Location "C:\VCC\themis"

try {
    # Load Visual Studio environment
    Write-Host "🔧 Loading Visual Studio environment..."
    $vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Professional"
    $vcvars = "$vsPath\VC\Auxiliary\Build\vcvarsall.bat"
    
    if (-not (Test-Path $vcvars)) {
        Write-Error "vcvarsall.bat not found at $vcvars"
        exit 1
    }

    # Build command with environment setup
    $buildCmd = @"
@echo off
call "$vcvars" x64 > nul
cd /d "C:\VCC\themis"
"@

    if ($Clean -and (Test-Path "build-ninja")) {
        Write-Host "🧹 Cleaning build-ninja..."
        Remove-Item -Recurse -Force "build-ninja"
    }

    Write-Host "📝 Configuring CMake with Ninja..."
    $buildCmd += "`ncmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake -DTHEMIS_BUILD_TESTS=OFF -DTHEMIS_BUILD_BENCHMARKS=OFF -DTHEMIS_BUILD_DOCS_DB=OFF -DTHEMIS_ENABLE_LLM=OFF"
    
    $buildCmd += "`nif errorlevel 1 exit /b 1"
    $buildCmd += "`necho ✅ CMake configuration successful"
    $buildCmd += "`necho 🏗️ Building with Ninja..."
    $buildCmd += "`ncd build-ninja"
    $buildCmd += "`nninja -j $Jobs"
    
    # Save and execute batch file
    $batchFile = "C:\VCC\themis\temp_build.bat"
    Set-Content -Path $batchFile -Value $buildCmd
    
    Write-Host "⏳ Building..."
    & cmd.exe /c $batchFile
    $buildResult = $LASTEXITCODE
    
    Remove-Item -Force $batchFile -ErrorAction SilentlyContinue
    
    if ($buildResult -eq 0) {
        Write-Host "✅ Build successful!"
        Get-ChildItem "build-ninja\Release" -Filter "*.exe" | Select-Object Name
    } else {
        Write-Error "Build failed with code $buildResult"
    }
    
    exit $buildResult

} finally {
    Pop-Location
}
