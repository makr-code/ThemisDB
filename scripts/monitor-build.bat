@echo off
REM Windows Build Monitor - Real-time status tracking

setlocal enabledelayedexpansion

set BUILD_DIR=C:\VCC\themis\build-msvc
set RELEASE_DIR=%BUILD_DIR%\Release

:loop
cls
echo.
echo ========================================
echo ThemisDB v1.4.0 Build Monitor
echo ========================================
echo Timestamp: %date% %time%
echo.

REM Count object files
for /f %%C in ('dir /s /b "%BUILD_DIR%\*.obj" 2^>nul ^| find /c /v ""') do (
    echo Object files compiled: %%C
)

REM Check for main executable
if exist "%RELEASE_DIR%\themis_server.exe" (
    echo.
    echo Build Status: ✅ COMPLETE
    echo.
    for %%F in ("%RELEASE_DIR%\themis_server.exe") do (
        echo themis_server.exe: %%~zF bytes (%%~fF)
    )
    
    REM List all release binaries
    echo.
    echo Release Binaries:
    for %%F in ("%RELEASE_DIR%\*.exe") do (
        echo   - %%~nF: %%~zF bytes
    )
    
    echo.
    echo Ready for packaging!
    echo Command: C:\VCC\themis\scripts\package-windows.bat
    exit /b 0
) else (
    echo Build Status: ⏳ IN PROGRESS
    
    REM Check for recent changes
    for /f "delims=" %%T in ('powershell -Command "Get-ChildItem '%BUILD_DIR%' -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty LastWriteTime"') do (
        echo Last activity: %%T
    )
)

REM Refresh every 10 seconds
timeout /t 10 /nobreak
goto loop
