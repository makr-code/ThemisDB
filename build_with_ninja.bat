@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
cd C:\VCC\themis\build-ninja-tests-bench
ninja -j 8
