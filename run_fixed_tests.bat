@echo off
setlocal enabledelayedexpansion

REM Set up environment for test execution
set "TEST_DIR=C:\VCC\themis\build-ninja-llm-gpu\cmake\tests"
set "BIN_DIR=C:\VCC\themis\build-ninja-llm-gpu\bin"
set "VCPKG_BIN=C:\VCC\themis\build-ninja-llm-gpu\vcpkg_installed\x64-windows\bin"

REM Add bin directories to PATH
set "PATH=!BIN_DIR!;!VCPKG_BIN!;!PATH!"

REM Copy DLLs to test directory for easier access
echo Copying DLLs to test directory...
copy "!BIN_DIR!\llama.dll" "!TEST_DIR!\" /Y >nul 2>&1
copy "!BIN_DIR!\ggml.dll" "!TEST_DIR!\" /Y >nul 2>&1
copy "!BIN_DIR!\ggml-base.dll" "!TEST_DIR!\" /Y >nul 2>&1
copy "!BIN_DIR!\ggml-cpu.dll" "!TEST_DIR!\" /Y >nul 2>&1

echo DLLs copied. Now running tests...
echo.

cd /d "!TEST_DIR!"

echo [1/3] Running LoRA Adapter Application Tests...
themis_tests.exe --gtest_filter="LoraAdapterApplicationTest.ApplyAdapterSucceedsWithLoadedAdapter" 2>&1 > lora_test_result.txt
set lora_result=!ERRORLEVEL!
type lora_test_result.txt

echo.
echo [2/3] Running MIME Detector Tests...
themis_tests.exe --gtest_filter="MimeDetectorTest.ValidateUpload_AllowedType_ValidSize" 2>&1 > mime_test_result.txt
set mime_result=!ERRORLEVEL!
type mime_test_result.txt

echo.
echo [3/3] Running PII Detector Tests...
themis_tests.exe --gtest_filter="PIIDetectorTest.DetectEmail" 2>&1 > pii_test_result.txt
set pii_result=!ERRORLEVEL!
type pii_test_result.txt

echo.
echo ============================================
echo Test Summary:
echo ============================================
if !lora_result! equ 0 (
  echo [PASS] LoRA Adapter Tests
) else (
  echo [FAIL] LoRA Adapter Tests - Exit Code !lora_result!
)

if !mime_result! equ 0 (
  echo [PASS] MIME Detector Tests
) else (
  echo [FAIL] MIME Detector Tests - Exit Code !mime_result!
)

if !pii_result! equ 0 (
  echo [PASS] PII Detector Tests
) else (
  echo [FAIL] PII Detector Tests - Exit Code !pii_result!
)

echo ============================================
pause
