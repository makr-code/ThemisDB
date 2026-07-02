@echo off
REM Load Visual Studio Developer Command Prompt and perform clean configure + build

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
if exist build-msvc-windows-release rmdir /s /q build-msvc-windows-release
cmake --preset windows-release
cmake --build --preset windows-release -- -j 16
