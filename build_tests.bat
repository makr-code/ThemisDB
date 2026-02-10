@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
cd /d C:\VCC\themis
cmake --build build-ninja-llm-gpu --config Release --parallel 4 --target themis_tests 2>&1
