@echo off
REM ThemisDB v1.4.0 Windows Packaging Script
REM Creates ZIP archives and checksums for distribution

setlocal enabledelayedexpansion

set REPO_ROOT=%~dp0..
set BUILD_DIR=%REPO_ROOT%\build-msvc
set RELEASE_DIR=%REPO_ROOT%\release
set PACKAGE_DIR=%RELEASE_DIR%\v1.4.0-windows

echo ==========================================
echo ThemisDB v1.4.0 Windows Packaging
echo ==========================================
echo Repo: %REPO_ROOT%
echo Build: %BUILD_DIR%
echo Package: %PACKAGE_DIR%
echo.

REM Check if build exists
if not exist "%BUILD_DIR%\Release\themis_server.exe" (
    echo ERROR: themis_server.exe not found in %BUILD_DIR%\Release
    exit /b 1
)

REM Create package directory
if not exist "%PACKAGE_DIR%" mkdir "%PACKAGE_DIR%"

REM Verify Release binaries
set "BINARIES=themis_server.exe themis_cli.exe themis_tests.exe"
set "ALL_FOUND=1"
for %%B in (%BINARIES%) do (
    if exist "%BUILD_DIR%\Release\%%B" (
        echo [OK] Found %%B
    ) else (
        echo [WARN] Missing %%B (optional)
    )
)

REM Create ZIP archive
echo.
echo [1/3] Creating ZIP archive...
set "ZIP_FILE=%PACKAGE_DIR%\themisdb-1.4.0-windows-x64.zip"

if exist "%ZIP_FILE%" del "%ZIP_FILE%"

REM Add binaries
for %%B in (%BINARIES%) do (
    if exist "%BUILD_DIR%\Release\%%B" (
        echo   Adding %%B
        powershell -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; $zip = [System.IO.Compression.ZipFile]::Open('%ZIP_FILE%', 'Update'); [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, '%BUILD_DIR%\Release\%%B', '%%B') | Out-Null; $zip.Dispose()"
    )
)

REM Create checksums
echo.
echo [2/3] Creating SHA256 checksums...
set "CHECKSUMS_FILE=%PACKAGE_DIR%\SHA256SUMS"
if exist "%CHECKSUMS_FILE%" del "%CHECKSUMS_FILE%"

powershell -Command ^
    "Get-ChildItem '%PACKAGE_DIR%\themisdb-*.zip' -File | ForEach-Object { ^
        $hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash; ^
        Add-Content '%CHECKSUMS_FILE%' \"$hash  $($_.Name)\"; ^
        Write-Host \"  $($_.Name): $hash\" ^
    }"

REM Create signature file (requires openssl)
echo.
echo [3/3] Creating signature file...
set "SIGNATURE_FILE=%PACKAGE_DIR%\SHA256SUMS.gpg"

if exist "%SIGNATURE_FILE%" del "%SIGNATURE_FILE%"
echo (Signature skipped - requires GPG key setup)
echo Note: To sign, use:
echo   gpg --armor --sign "%CHECKSUMS_FILE%"

REM Summary
echo.
echo ==========================================
echo Packaging Complete
echo ==========================================
echo Package directory: %PACKAGE_DIR%
echo.
dir "%PACKAGE_DIR%"
echo.
echo Next steps:
echo   1. Upload to GitHub Release:
echo      - File: %ZIP_FILE%
echo      - Checksums: %CHECKSUMS_FILE%
echo   2. Create GitHub Release with:
echo      - Tag: v1.4.0
echo      - Title: ThemisDB v1.4.0
echo      - Description: (from CHANGELOG.md)
echo      - Attach: ZIP, SHA256SUMS
echo.
