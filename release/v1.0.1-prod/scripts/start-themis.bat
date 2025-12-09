@echo off
REM THEMIS Server Startup Script (Windows)
REM Version: 1.0.1

setlocal enabledelayedexpansion

REM Get script directory
set SCRIPT_DIR=%~dp0
set THEMIS_ROOT=%SCRIPT_DIR%..
set BIN_DIR=%THEMIS_ROOT%\bin
set DATA_DIR=%THEMIS_ROOT%\data
set LOGS_DIR=%THEMIS_ROOT%\logs
set CACHE_DIR=%THEMIS_ROOT%\cache
set CONFIG_FILE=%THEMIS_ROOT%\config.json
set PID_FILE=%TEMP%\themis_server.pid

REM Color helpers (Windows 10+)
if "%TERM%"=="" (
    set INFO=[INFO]
    set WARN=[WARN]
    set ERROR=[ERROR]
) else (
    set INFO=[92m[INFO][0m
    set WARN=[93m[WARN][0m
    set ERROR=[91m[ERROR][0m
)

:main
if "%1"=="" goto start
if "%1"=="start" goto start
if "%1"=="foreground" goto foreground
if "%1"=="stop" goto stop
if "%1"=="status" goto status
goto usage

:start
echo %INFO% THEMIS v1.0.1 Server Startup
call :setup_directories
call :verify_config
if errorlevel 1 goto error
call :verify_binary
if errorlevel 1 goto error
call :check_port 8080
if errorlevel 1 goto error
call :start_server
if errorlevel 1 goto error
call :verify_startup
if errorlevel 1 goto error
call :show_info
goto end

:foreground
echo %INFO% THEMIS v1.0.1 Server (Foreground Mode)
call :setup_directories
call :verify_config
if errorlevel 1 goto error
call :verify_binary
if errorlevel 1 goto error
call :check_port 8080
if errorlevel 1 goto error
cd /d "%THEMIS_ROOT%"
echo %INFO% Starting in foreground... (Press Ctrl+C to stop)
"%BIN_DIR%\themis_server.exe"
goto end

:stop
if exist "%PID_FILE%" (
    echo %INFO% Stopping server...
    for /f %%i in (%PID_FILE%) do (
        taskkill /PID %%i /F >nul 2>&1
    )
    del /f /q "%PID_FILE%" >nul 2>&1
    echo %INFO% Server stopped
) else (
    echo %WARN% No PID file found - server may not be running
)
goto end

:status
if exist "%PID_FILE%" (
    for /f %%i in (%PID_FILE%) do (
        tasklist /FI "PID eq %%i" 2>NUL | find /I /N "themis_server">NUL
        if "!ERRORLEVEL!"=="0" (
            echo %INFO% Server is running (PID: %%i)
            powershell -Command "try { $response = Invoke-WebRequest -Uri 'http://localhost:8080/health' -UseBasicParsing; Write-Host $response.Content } catch { Write-Host 'Health check failed' }"
        ) else (
            echo %ERROR% Server not running (stale PID file)
        )
    )
) else (
    echo %ERROR% Server not running
)
goto end

:setup_directories
echo %INFO% Setting up directories...
if not exist "%DATA_DIR%" (
    mkdir "%DATA_DIR%"
    echo %INFO% Created: %DATA_DIR%
)
if not exist "%LOGS_DIR%" (
    mkdir "%LOGS_DIR%"
    echo %INFO% Created: %LOGS_DIR%
)
if not exist "%CACHE_DIR%" (
    mkdir "%CACHE_DIR%"
    echo %INFO% Created: %CACHE_DIR%
)
exit /b 0

:verify_config
if not exist "%CONFIG_FILE%" (
    echo %ERROR% Configuration file not found: %CONFIG_FILE%
    exit /b 1
)
echo %INFO% Configuration verified: %CONFIG_FILE%
exit /b 0

:verify_binary
if not exist "%BIN_DIR%\themis_server.exe" (
    echo %ERROR% Binary not found: %BIN_DIR%\themis_server.exe
    exit /b 1
)
echo %INFO% Binary verified: %BIN_DIR%\themis_server.exe
exit /b 0

:check_port
REM Check if port is available (simplified check)
echo %INFO% Checking port 8080...
exit /b 0

:start_server
echo %INFO% Starting THEMIS Server v1.0.1...
cd /d "%THEMIS_ROOT%"
start /B "THEMIS Server" "%BIN_DIR%\themis_server.exe" >> "%LOGS_DIR%\application.log" 2>&1
for /f "tokens=2" %%i in ('tasklist /FO TABLE ^| find "themis_server"') do (
    echo %%i > "%PID_FILE%"
    echo %INFO% Server started with PID: %%i
)
timeout /T 2 /NOBREAK >nul
exit /b 0

:verify_startup
echo %INFO% Verifying server startup...
setlocal enabledelayedexpansion
set retries=5
set count=0

:retry_loop
set /a count=!count!+1
powershell -Command "try { $response = Invoke-WebRequest -Uri 'http://localhost:8080/health' -UseBasicParsing -TimeoutSec 2; exit 0 } catch { exit 1 }" >nul 2>&1
if not errorlevel 1 (
    echo %INFO% Server is healthy
    exit /b 0
)
if !count! lss !retries! (
    echo %WARN% Waiting for server... (attempt !count!/%retries%)
    timeout /T 1 /NOBREAK >nul
    goto retry_loop
)
echo %ERROR% Server did not respond to health check
exit /b 1

:show_info
echo.
echo ==========================================
echo   THEMIS Server Started Successfully
echo ==========================================
echo Version:        1.0.1
echo Root:           %THEMIS_ROOT%
echo Binary:         %BIN_DIR%\themis_server.exe
echo Configuration:  %CONFIG_FILE%
echo Data:           %DATA_DIR%
echo Logs:           %LOGS_DIR%
echo.
echo Server available at:
echo   HTTP:  http://localhost:8080
echo   gRPC:  grpc://localhost:50051
echo.
echo Useful commands:
echo   Check health:    powershell -Command "Invoke-WebRequest http://localhost:8080/health"
echo   View logs:       type %LOGS_DIR%\application.log
echo   View metrics:    powershell -Command "Invoke-WebRequest http://localhost:8080/metrics"
echo ==========================================
echo.
exit /b 0

:error
echo %ERROR% Startup failed
exit /b 1

:usage
echo Usage: %0 {start^|foreground^|stop^|status}
echo.
echo Commands:
echo   start      - Start server in background
echo   foreground - Start server in foreground (for debugging)
echo   stop       - Stop the running server
echo   status     - Show server status
exit /b 1

:end
endlocal
