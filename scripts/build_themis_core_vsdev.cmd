@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 || exit /b 1
cd /d C:\VCC\themis
cmake -S . -B build-msvc-windows-release-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DTHEMIS_BUILD_TESTS=OFF -DTHEMIS_BUILD_BENCHMARKS=OFF -DCMAKE_SYSTEM_VERSION=10.0.26100.0 || exit /b 1
cmake --build build-msvc-windows-release-static --target themis_core --parallel 8
exit /b %ERRORLEVEL%
