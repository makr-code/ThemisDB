@echo off
setlocal

set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207"
set "WINSKDROOT=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"

if not exist "%MSVCROOT%" (
  echo MSVCROOT not found: %MSVCROOT%
  exit /b 2
)
if not exist "%WINSKDROOT%" (
  echo Windows SDK lib path not found: %WINSKDROOT%
  exit /b 2
)

echo Adding MSVC + Windows SDK lib/include to environment
set "MSVC_LIB=%MSVCROOT%\lib\x64;%MSVCROOT%\lib\onecore\x64;%MSVCROOT%\lib\x64\store;%MSVCROOT%\lib\x64\uwp"
set "LIB=%MSVC_LIB%;%WINSKDROOT%;%LIB%"
set "MSVC_INC=%MSVCROOT%\include"
set "INCLUDE=%MSVC_INC%;%INCLUDE%"

echo LIB preview: %LIB:~0,300%

cmake -S . -B build-msvc-relwithdebinfo -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTHEMIS_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG
if errorlevel 1 (
  echo CMake configure failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

cmake --build build-msvc-relwithdebinfo --target test_self_rag_alce_focused --parallel 16
if errorlevel 1 (
  echo Build failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

echo Build success
endlocal
exit /b 0
