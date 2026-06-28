@echo off
rem Call VS dev env, then ensure MSVC lib paths are in LIB before configuring
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207"
if not exist "%MSVCROOT%\lib\x64\msvcrtd.lib" (
  echo MSVCRTD.lib not found under %MSVCROOT%\lib\x64
  exit /b 2
)

echo Prepending MSVC lib path to LIB
set "LIB=%MSVCROOT%\lib\x64;%LIB%"
echo LIB now starts with: %LIB:~0,200%

echo Running CMake configure (RelWithDebInfo)
cmake -S . -B build-msvc-relwithdebinfo -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTHEMIS_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG
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
