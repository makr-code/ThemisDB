@echo off
REM Enterprise Features Build Script (CMD version)
REM Requires Visual Studio 2022 Developer Command Prompt

echo ========================================
echo Enterprise Scalability Features Build
echo ========================================
echo.

REM Check if Visual Studio environment is loaded
cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio environment not loaded!
    echo Please run this script from "x64 Native Tools Command Prompt for VS 2022"
    echo Or run: "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    exit /b 1
)

echo Visual Studio environment: OK
echo.

echo Reconfiguring CMake...
cmake build-msvc-ninja-debug
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo.
echo Building themis_tests with enterprise features...
echo   - Token Bucket Rate Limiter
echo   - Per-Client Rate Limiter  
echo   - Adaptive Load Shedder
echo   - HTTP Client Pool (Boost.Beast)
echo   - Batch CRUD Endpoint
echo.

cmake --build build-msvc-ninja-debug --target themis_tests -- -j4

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo Build successful!
    echo ========================================
    echo.
    echo Running Enterprise Scalability Tests...
    echo.
    
    build-msvc-ninja-debug\themis_tests.exe --gtest_filter="*Enterprise*:TokenBucket*:PerClient*:LoadShedder*:HTTPClientPool*" --gtest_brief=1
    
    echo.
    echo ========================================
    echo Enterprise Features Implemented:
    echo ========================================
    echo   [OK] Token Bucket Rate Limiter
    echo   [OK] Per-Client Rate Limiter
    echo   [OK] Adaptive Load Shedder
    echo   [OK] HTTP Client Pool (Boost.Beast)
    echo   [OK] Batch CRUD Endpoint
    echo.
    echo Documentation:
    echo   - docs/ENTERPRISE_SCALABILITY.md
    echo   - docs/performance/ENTERPRISE_SCALABILITY_STRATEGY.md
    echo   - docs/ENTERPRISE_IMPLEMENTATION_STATUS.md
    echo.
    
) else (
    echo.
    echo ========================================
    echo Build FAILED!
    echo ========================================
    echo.
    echo Make sure you are running from VS Developer Command Prompt.
    exit /b 1
)
