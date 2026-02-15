@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
cd /d C:\VCC\themis

REM Clean problematic LLM build artifacts
echo Cleaning LLM build artifacts...
del /Q build-msvc-ninja-release\cmake\CMakeFiles\themis_core.dir\__\src\llm\lora_framework\*.obj 2>nul

echo.
echo ============================================
echo Rebuilding themis_tests (reduced parallelism)
echo ============================================
echo.
cmake --build build-msvc-ninja-release --config Release --target themis_tests --parallel 2
