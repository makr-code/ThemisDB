@echo off
setlocal enabledelayedexpansion

set "TEST_DIR=C:\VCC\themis\build-ninja-llm-gpu\cmake\tests"
set "BIN_DIR=C:\VCC\themis\build-ninja-llm-gpu\bin"

REM Copy DLLs
copy "!BIN_DIR!\llama.dll" "!TEST_DIR!\" /Y >nul 2>&1
copy "!BIN_DIR!\ggml*.dll" "!TEST_DIR!\" /Y >nul 2>&1

cd /d "!TEST_DIR!"

echo ============================================
echo Running Extended Test Suite
echo ============================================
echo.

REM Run all LoRA Adapter tests
echo [Test Suite 1: LoRA Adapter Application]
themis_tests.exe --gtest_filter="LoraAdapterApplicationTest.*" 2>&1 > lora_full_results.txt
set lora_exit=!ERRORLEVEL!
findstr /C:"PASSED" lora_full_results.txt
findstr /C:"FAILED" lora_full_results.txt

echo.
echo [Test Suite 2: MIME Detector]
themis_tests.exe --gtest_filter="MimeDetectorTest.*" 2>&1 > mime_full_results.txt
set mime_exit=!ERRORLEVEL!
findstr /C:"PASSED" mime_full_results.txt
findstr /C:"FAILED" mime_full_results.txt

echo.
echo [Test Suite 3: PII Detector]
themis_tests.exe --gtest_filter="PIIDetectorTest.*" 2>&1 > pii_full_results.txt
set pii_exit=!ERRORLEVEL!
findstr /C:"PASSED" pii_full_results.txt
findstr /C:"FAILED" pii_full_results.txt

echo.
echo ============================================
echo Final Summary:
echo ============================================
if !lora_exit! equ 0 echo [OK] LoRA Tests
if !mime_exit! equ 0 echo [OK] MIME Tests
if !pii_exit! equ 0 echo [OK] PII Tests

pause
