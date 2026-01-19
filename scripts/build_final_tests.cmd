@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 || exit /b 1
cd /d C:\VCC\themis || exit /b 1
cmake --build build-ninja-final --config Release --parallel 8
exit /b %ERRORLEVEL%