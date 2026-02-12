@echo off
setlocal enabledelayedexpansion

REM Initialize Visual Studio Developer Command Prompt
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Build FAISS with vcpkg
cd /d C:\VCC\themis
.\vcpkg\vcpkg.exe install