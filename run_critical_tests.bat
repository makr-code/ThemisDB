@echo off
cd C:\VCC\themis\build-ninja-llm-gpu\cmake\tests

echo Running critical test suites...
echo.

REM Run only the tests we fixed
echo [1/3] Running LoRA Adapter Application Tests...
themis_tests.exe --gtest_filter="LoraAdapterApplicationTest.*" > lora_test_results.txt 2>&1
set lora_exit=%ERRORLEVEL%

echo [2/3] Running Content Policy / MIME Detector Tests...
themis_tests.exe --gtest_filter="MimeDetectorTest.*" >> mime_test_results.txt 2>&1
set mime_exit=%ERRORLEVEL%

echo [3/3] Running PII Detector Tests...
themis_tests.exe --gtest_filter="PIIDetectorTest.*" >> pii_test_results.txt 2>&1
set pii_exit=%ERRORLEVEL%

echo.
echo Test Results:
echo - LoRA Tests: Exit %lora_exit%
echo - MIME Tests: Exit %mime_exit%
echo - PII Tests: Exit %pii_exit%
echo.

if %lora_exit% equ 0 (echo ✓ LoRA tests PASSED) else (echo ✗ LoRA tests FAILED)
if %mime_exit% equ 0 (echo ✓ MIME tests PASSED) else (echo ✗ MIME tests FAILED)
if %pii_exit% equ 0 (echo ✓ PII tests PASSED) else (echo ✗ PII tests FAILED)

pause
