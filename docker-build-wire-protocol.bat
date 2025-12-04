@echo off
REM ThemisDB Docker Build Script with Wire Protocol Support (Windows)
REM This script builds the Docker image using the pre-built themis_server binary

setlocal enabledelayedexpansion

REM Configuration
set REPO_ROOT=%~dp0
set DOCKER_IMAGE_NAME=themis-db
set DOCKER_IMAGE_TAG=wire-protocol-latest

REM Check if themis_server binary exists
if not exist "%REPO_ROOT%build-wsl\themis_server" (
    echo ERROR: themis_server binary not found at %REPO_ROOT%build-wsl\themis_server
    echo Please build the binary first in WSL:
    echo   wsl bash -c "cd /mnt/c/VCC/themis && export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg && cmake -S . -B build-wsl -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/mnt/c/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake -DTHEMIS_BUILD_WIRE_PROTOCOL=ON && cmake --build build-wsl --target themis_server -j8"
    exit /b 1
)

echo.
echo === Building ThemisDB Docker Image with Wire Protocol ===
echo Context: %REPO_ROOT%
echo Dockerfile: %REPO_ROOT%Dockerfile.prebuilt
echo Image: %DOCKER_IMAGE_NAME%:%DOCKER_IMAGE_TAG%
echo.

REM Build the Docker image
docker build ^
    --build-arg VCPKG_TRIPLET=x64-linux ^
    -t "%DOCKER_IMAGE_NAME%:%DOCKER_IMAGE_TAG%" ^
    -t "%DOCKER_IMAGE_NAME%:latest" ^
    -f "%REPO_ROOT%Dockerfile.prebuilt" ^
    "%REPO_ROOT%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo === Docker image built successfully ===
    echo.
    echo To run the container with Wire Protocol enabled:
    echo   docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data %DOCKER_IMAGE_NAME%:%DOCKER_IMAGE_TAG%
    echo.
    echo To check the container:
    echo   docker ps -a
    echo.
) else (
    echo.
    echo === Docker image build failed ===
    exit /b 1
)
