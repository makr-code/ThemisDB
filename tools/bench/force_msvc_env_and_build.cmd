@echo off
setlocal

rem Explicit MSVC toolset path (adjust if different)
set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207"

if not exist "%MSVCROOT%" (
  echo MSVCROOT not found: %MSVCROOT%
  exit /b 2
)

echo Prepending MSVC lib/include paths for x64
set "MSVC_LIB=%MSVCROOT%\lib\x64;%MSVCROOT%\lib\onecore\x64;%MSVCROOT%\lib\x64\store;%MSVCROOT%\lib\x64\uwp"
set "LIB=%MSVC_LIB%;%LIB%"
set "MSVC_INC=%MSVCROOT%\include"
set "INCLUDE=%MSVC_INC%;%INCLUDE%"

echo LIB starts with: %LIB:~0,200%

echo Running CMake configure (RelWithDebInfo)
cmake -S . -B build-msvc-relwithdebinfo -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG -DTHEMIS_BUILD_TESTS=ON
if errorlevel 1 (
  echo CMake configure failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

echo Building focused test target
cmake --build build-msvc-relwithdebinfo --target test_self_rag_alce_focused --parallel 16
if errorlevel 1 (
  echo Build failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

echo Build completed successfully.
endlocal
exit /b 0
