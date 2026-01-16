@echo off
echo === Building ThemisDB Tests with Ninja ===
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
cd C:\VCC\themis\build-ninja-tests-bench
cmake . -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build . --target test_snapshot_manager test_pitr_manager test_schema_manager test_snapshot_integration test_diff_engine --config Release --parallel 8
