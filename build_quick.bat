@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul 2>&1
cd /d C:\VCC\themis
echo Building themis_tests...
cmake --build build-msvc-ninja-release --config Release --target themis_tests --parallel 2
echo.
echo Build completed with exit code: %ERRORLEVEL%
