@echo off
REM Build Status Monitor for ThemisDB v1.4.0

set REPO=%~dp0..
set BUILD_DIR=%REPO%\build-msvc
set RELEASE_DIR=%BUILD_DIR%\Release

echo.
echo ========================================
echo ThemisDB v1.4.0 Build Status Monitor
echo ========================================
echo.

REM Count .obj files as progress indicator
set OBJ_COUNT=0
for /r "%BUILD_DIR%" %%F in (*.obj) do set /a OBJ_COUNT+=1
echo Object files compiled: %OBJ_COUNT%

REM Check for main executable
if exist "%RELEASE_DIR%\themis_server.exe" (
    echo ✅ themis_server.exe: READY
    for %%F in ("%RELEASE_DIR%\themis_server.exe") do echo    Size: %%~zF bytes
) else (
    echo ⏳ themis_server.exe: BUILDING...
)

if exist "%RELEASE_DIR%\themis_tests.exe" (
    echo ✅ themis_tests.exe: READY
    for %%F in ("%RELEASE_DIR%\themis_tests.exe") do echo    Size: %%~zF bytes
) else (
    echo ⏳ themis_tests.exe: BUILDING...
)

REM Check for libraries
echo.
echo Libraries:
dir /b "%RELEASE_DIR%\*.lib" 2>nul | find /c /v "" >nul && (
    dir /b "%RELEASE_DIR%\*.lib" | find /c /v "" >nul && (for /f %%C in ('dir /b "%RELEASE_DIR%\*.lib" ^| find /c /v ""') do echo   ✅ Found %%C .lib files
    ) || echo   ⏳ Compiling...
) || echo   ⏳ Pending compilation...

REM Check build output
echo.
echo Recent build activity:
for /f "delims=" %%L in ('dir /od /b /s "%BUILD_DIR%" 2>nul ^| sort ^| tail -10') do echo   %%L

echo.
echo To wait for build to complete:
echo   1. Keep this window open
echo   2. Check again in 5 minutes
echo   3. Build progress: check build-msvc\CMakeFiles\*.dir
echo.
