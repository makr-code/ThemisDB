@echo off
REM Simple script to build Docker image from pre-built binary
REM Schritt 1: Copy themis_server binary from WSL to current directory
REM Schritt 2: Build Docker image using Dockerfile.runtime
REM Schritt 3: Clean up

setlocal enabledelayedexpansion

cd /d c:\VCC\themis

echo === ThemisDB Docker Build (Runtime) ===
echo.

echo Schritt 1: Kopiere Binary aus WSL...
wsl bash -c "cat /mnt/c/VCC/themis/build-wsl/themis_server" > themis_server_temp.bin
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Binary aus WSL konnte nicht gelesen werden
    exit /b 1
)
echo ✓ Binary kopiert (%CD%\themis_server_temp.bin)

echo.
echo Schritt 2: Baue Docker Image...
docker build -t themis-db:wire-protocol-latest -f Dockerfile.runtime . --build-arg BINARY_PATH=themis_server_temp.bin

if %ERRORLEVEL% EQU 0 (
    echo.
    echo === Docker Image erfolgreich gebaut ===
    echo.
    docker images | findstr themis-db
    echo.
    echo Zum Starten:
    echo   docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data themis-db:wire-protocol-latest
    echo.
) else (
    echo.
    echo ERROR: Docker Build fehlgeschlagen
)

echo.
echo Schritt 3: Räume auf...
del /f /q themis_server_temp.bin 2>nul

echo Fertig!
