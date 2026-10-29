@echo off
REM Temporarily remove coreutils from PATH for this session
set "PATH=%PATH:C:\Program Files\coreutils\bin;=%"

REM Unset VSINSTALLDIR so vcvars selects the correct installation, set debug
set "VSINSTALLDIR="
set VSCMD_DEBUG=3

REM Initialize MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
if errorlevel 1 (
  echo vcvarsall failed
  exit /b 1
)

echo --- LIB ---
echo %LIB%
echo --- INCLUDE ---
echo %INCLUDE%

REM Configure and build
cmake --preset windows-release
if errorlevel 1 (
  echo cmake configure failed
  exit /b 2
)

cmake --build --preset windows-release -- -j 16
if errorlevel 1 (
  echo build failed
  exit /b 3
)

echo Build completed successfully.
exit /b 0
