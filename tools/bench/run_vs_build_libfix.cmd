@echo off
setlocal
set VCToolsVersion=14.44.35207
set VSCMD_DEBUG=3

echo Init VsDevCmd (x64)
set VSDEV=
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set VSDEV="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV (
  echo VsDevCmd.bat not found.
  exit /b 2
)

call %VSDEV% -arch=x64 -host_arch=x64
if errorlevel 1 (
  echo VsDevCmd failed with %ERRORLEVEL%
  exit /b %ERRORLEVEL%
)

rem Prepend MSVC lib x64 path to LIB to ensure linker finds MSVCRTD.lib
set MSVC_LIB_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\lib\x64
if exist "%MSVC_LIB_PATH%" (
  echo Prepending %MSVC_LIB_PATH% to LIB
  set LIB=%MSVC_LIB_PATH%;%LIB%
) else (
  echo Warning: %MSVC_LIB_PATH% does not exist
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
