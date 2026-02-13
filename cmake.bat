@echo off
REM CMake wrapper that initializes VS2022 environment before running CMake
REM Called by VS Code CMake Tools

setlocal EnableDelayedExpansion
set "VSCMD_START_DIR=%CD%"
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul 2>&1

REM Find cmake.exe path
for /f "tokens=*" %%i in ('where cmake.exe 2^>nul') do set "CMAKE_EXE=%%i"
if not defined CMAKE_EXE set "CMAKE_EXE=cmake.exe"

REM Forward all arguments to cmake.exe - properly preserving quotes
"%CMAKE_EXE%" %*

REM Propagate exit code
exit /b %ERRORLEVEL%
