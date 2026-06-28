@echo off
setlocal
set VCToolsVersion=14.44.35207
set VSCMD_DEBUG=3

echo Running VsDevCmd with VCToolsVersion=%VCToolsVersion%
rem find VsDevCmd.bat by common locations
set VSDEV=
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV (
  echo VsDevCmd.bat not found in common locations.
  exit /b 2
)

echo Calling %VSDEV%
call %VSDEV%
if errorlevel 1 (
  echo VsDevCmd failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

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
