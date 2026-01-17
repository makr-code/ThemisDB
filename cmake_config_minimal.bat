@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
cd C:\VCC\themis
cmake -S . -B build-ninja-tests-bench -G Ninja ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON ^
  -DTHEMIS_ENABLE_GPU=OFF ^
  -DTHEMIS_ENABLE_LLM=OFF ^
  -DTHEMIS_ENABLE_CONTENT=OFF ^
  -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=OFF ^
  -DTHEMIS_ENABLE_S3=OFF ^
  -DTHEMIS_ENABLE_AZURE=OFF ^
  -DTHEMIS_ENABLE_WEBDAV=OFF ^
  -DTHEMIS_ENABLE_OPENCL=OFF ^
  -DTHEMIS_ENABLE_ONEAPI=OFF
