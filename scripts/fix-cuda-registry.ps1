# CUDA-Erkennung Fix (Strategie H)
# Patch für cmake/CompilerOptions.cmake
# 
# Problem: CMake liest Windows Registry nicht automatisch aus für CUDA Toolkit Pfad
# Lösung: Explizit setzen VOR enable_language(CUDA)

Write-Host "=== CUDA Integration Fix (Strategie H) ===" -ForegroundColor Cyan

# Backup
$optionsFile = "C:\VCC\themis\cmake\CompilerOptions.cmake"
$backupFile = "$optionsFile.bak-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
Copy-Item $optionsFile $backupFile
Write-Host "✓ Backup erstellt: $backupFile" -ForegroundColor Green

# Patch anwenden
$oldText = @"
# CUDA support (if enabled)
if(THEMIS_ENABLE_CUDA)
    enable_language(CUDA)
"@

$newText = @"
# CUDA support (if enabled)
if(THEMIS_ENABLE_CUDA)
    # Fix for CMake ↔ VS2022 ↔ CUDA 13.1 Registry Lookup Issue
    # Explicitly set CUDA Toolkit paths BEFORE enable_language(CUDA)
    if(MSVC AND NOT CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES)
        set(_CUDA_TOOLKIT_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1")
        if(EXISTS "${_CUDA_TOOLKIT_ROOT}/include/cuda.h")
            set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES 
                "${_CUDA_TOOLKIT_ROOT}/include" CACHE PATH "CUDA Toolkit Include Path" FORCE)
            message(STATUS "CUDA Toolkit Include: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
        else()
            message(WARNING "CUDA Toolkit not found at ${_CUDA_TOOLKIT_ROOT}")
        endif()
    endif()
    
    enable_language(CUDA)
"@

$content = Get-Content $optionsFile -Raw
if ($content -contains $oldText) {
    Write-Host "✗ Exakte Text-Übereinstimmung nicht gefunden. Nutze Regex-Anpassung..." -ForegroundColor Yellow
} else {
    Write-Host "✓ Patching Datei: $optionsFile" -ForegroundColor Yellow
    $newContent = $content -replace [regex]::Escape($oldText), $newText
    
    if ($newContent -ne $content) {
        Set-Content $optionsFile $newContent -Encoding UTF8
        Write-Host "✓ Patch erfolgreich angewendet!" -ForegroundColor Green
        
        # Test
        Write-Host "`n=== Test CMake Konfiguration ===" -ForegroundColor Cyan
        cd C:\VCC\themis
        rm -r build-cuda-fix -Force -ErrorAction SilentlyContinue
        cmake -S . -B build-cuda-fix -G "Visual Studio 17 2022" `
          -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
          -DTHEMIS_ENABLE_CUDA=ON 2>&1 | Select-Object -Last 20
    } else {
        Write-Host "✗ Patch konnte nicht angewendet werden" -ForegroundColor Red
        Write-Host "`nErwarteter Text nicht gefunden. Manuelle Änderung nötig:"
        Write-Host "Datei: $optionsFile"
        Write-Host "Suche nach: '# CUDA support (if enabled)'"
        Write-Host "Backup vorhanden bei: $backupFile"
    }
}
