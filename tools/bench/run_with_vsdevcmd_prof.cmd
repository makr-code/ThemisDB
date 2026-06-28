@echo off
rem Initialize Visual Studio Professional developer environment for x64
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

where cl
where link

echo ---LIB---
set LIB
echo ---INCLUDE---
set INCLUDE

echo Running CMake configure
cmake -S . -B build-msvc-relwithdebinfo -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTHEMIS_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG

echo Done
