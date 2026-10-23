@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
where cl
where link
cd /d C:\Projects\ThemisDB
rmdir /s /q build-msvc-windows-debug-vsdev 2>nul
cmake -G Ninja -S . -B build-msvc-windows-debug-vsdev -DTHEMIS_ENABLE_COMPILER_CACHE=OFF -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_EDITION=COMMUNITY -DCMAKE_BUILD_TYPE=Debug
cmake --build build-msvc-windows-debug-vsdev --parallel 20 --target module_voice_test_voice_audit_logging_focused
