@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
cd C:\VCC\themis
cmake -S . -B build-ninja-tests-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_ENABLE_LLM=OFF -DTHEMIS_ENABLE_GPU=OFF -DTHEMIS_ENABLE_CUDA=OFF -DTHEMIS_ENABLE_HIP=OFF
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
cmake --build build-ninja-tests-bench --target themis_server --config Release --parallel 8
