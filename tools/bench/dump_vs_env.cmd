@echo off
setlocal
echo Calling VsDevCmd and dumping environment to artifacts\vs_env_full.txt
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul 2>&1
if errorlevel 1 (
  echo VSDEV_INIT_FAILED=%ERRORLEVEL%> artifacts\vs_env_full.txt
  exit /b %ERRORLEVEL%
)
set > artifacts\vs_env_full.txt
endlocal
exit /b 0
