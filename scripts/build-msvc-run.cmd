call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\\Tools\\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
cd /d "C:\VCC\themis"
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="C:/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_CXX_FLAGS="/bigobj /EHsc"
cmake --build build-msvc --config Debug --target themis_tests
cd build-msvc
ctest -C Debug --output-on-failure -j1
