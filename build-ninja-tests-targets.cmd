@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
cd C:\VCC\themis\build-ninja-tests-bench
cmake --build . --target test_snapshot_manager test_pitr_manager test_schema_manager test_snapshot_integration test_diff_engine test_content_versioning test_geo_backend test_hnsw_index test_nlp_analyzer test_compression_manager test_backup_manager test_crypto_manager test_changefeed test_migration_manager test_rbac_manager test_wire_protocol --config Release --parallel 8
