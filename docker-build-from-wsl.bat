@echo off
REM ThemisDB Docker Build Script with Wire Protocol Support (Windows)
REM Dieses Skript kopiert das Binary aus WSL und baut das Docker Image

setlocal enabledelayedexpansion

echo === ThemisDB Docker Build Script (Wire Protocol) ===
echo.

set REPO_ROOT=%~dp0
set WSL_BUILD_PATH=/mnt/c/VCC/themis/build-wsl
set LOCAL_BUILD_PATH=%REPO_ROOT%build-wsl
set DOCKER_IMAGE_NAME=themis-db
set DOCKER_IMAGE_TAG=wire-protocol-latest

echo Schritt 1: Prüfe WSL Binary...
wsl bash -c "test -f %WSL_BUILD_PATH%/themis_server && echo '✓ WSL Binary gefunden' || exit 1"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: themis_server Binary nicht gefunden im WSL
    echo Bitte stelle sicher, dass der Build erfolgreich war:
    echo   wsl bash -c "cd /mnt/c/VCC/themis && export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg && cmake --build build-wsl --target themis_server -j8"
    exit /b 1
)

echo.
echo Schritt 2: Kopiere Binary aus WSL nach Windows...
if not exist "%LOCAL_BUILD_PATH%" mkdir "%LOCAL_BUILD_PATH%"
wsl bash -c "cp %WSL_BUILD_PATH%/themis_server /mnt/c/VCC/themis/build-wsl/themis_server && echo '✓ Binary kopiert'"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Binary konnte nicht kopiert werden
    exit /b 1
)

echo.
echo Schritt 3: Baue Docker Image...
docker build ^
    -t "%DOCKER_IMAGE_NAME%:%DOCKER_IMAGE_TAG%" ^
    -t "%DOCKER_IMAGE_NAME%:latest" ^
    -f "%REPO_ROOT%Dockerfile.prebuilt" ^
    "%REPO_ROOT%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo === Docker Image erfolgreich gebaut ===
    echo.
    docker images | findstr "%DOCKER_IMAGE_NAME%"
    echo.
    echo Zum Starten des Containers:
    echo   docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data %DOCKER_IMAGE_NAME%:%DOCKER_IMAGE_TAG%
    echo.
    echo Zum Prüfen des Status:
    echo   docker ps -a
    echo.
) else (
    echo.
    echo === Docker Image Build fehlgeschlagen ===
    exit /b 1
)
