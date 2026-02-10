@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
cd /d C:\VCC\themis\build-msvc-ninja-release
cmake --build . --target themis_server --parallel 8
exit /b %ERRORLEVEL%
