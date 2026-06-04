# Gap Scope Breakdown (2026-06-04)

- Source: ai_working/gap_scan_results.json
- Total gaps: 253903

## Scope Policy
- themis_core: src/, include/, tools/, scripts/, cmake/, docs/, examples/
- themis_tests: tests/
- themis_benchmarks: benchmarks/
- third_party: alle übrigen Pfade (u. a. vcpkg, externe Mirrors, build-Artefakte)

## Counts by Scope
- themis_core: 38582 (15.2%)
- themis_tests: 6808 (2.68%)
- themis_benchmarks: 1079 (0.42%)
- third_party: 207434 (81.7%)

## themis_core
### Severity
- MEDIUM: 14418
- HIGH: 10528
- LOW: 9452
- CRITICAL: 4184
### Top Types
- missing_doxygen_comment: 3987
- missing_doxygen_return: 3275
- docs_broken_markdown_link: 2766
- missing_doxygen_param: 2643
- missing_doxygen_brief: 2327
- resource_leaked_in_exception: 1394
- no_key_rotation: 1193
- string_concat_loop: 963
- data_race: 946
- hardcoded_output: 905
### Top Scanners
- Uniform::themis_cpp_doxygen_policy_rules: 12232
- Uniform::themis_docs_markdown_rules: 2789
- Uniform::container: 2197
- Uniform::exception_safety: 1526
- Uniform::key_failure: 1362
- Uniform::performance_patterns: 1272
- Uniform::raii: 1251
- Uniform::e2e_encryption: 1182
- Uniform::phase1_error_handling: 1105
- Uniform::concurrency: 1079
### Top Path Prefixes
- src: 20440
- include: 15256
- docs: 2789
- examples: 60
- tools: 37

## themis_tests
### Severity
- MEDIUM: 2815
- HIGH: 2326
- CRITICAL: 1421
- LOW: 246
### Top Types
- primitive_no_volatile: 1912
- resource_leaked_in_exception: 1115
- thread_join_no_timeout: 606
- unspecified_consistency: 371
- missing_audit_log: 257
- unstructured_log: 226
- explicit_delete: 205
- missing_consensus: 204
- missing_resource_limits: 182
- legacy_or_compat_path: 180
### Top Scanners
- Uniform::phase1_thread_safety: 2530
- Uniform::exception_safety: 1161
- Uniform::distributed_consistency: 654
- Uniform::observability: 466
- Uniform::performance_patterns: 463
- Uniform::audit_logging: 439
- Uniform::llm_ai_safety: 326
- Uniform::phase1_raii: 231
- Uniform::legacy_duplication: 196
- Uniform::gpu_memory: 96
### Top Path Prefixes
- tests: 6808

## themis_benchmarks
### Severity
- MEDIUM: 438
- HIGH: 331
- LOW: 230
- CRITICAL: 80
### Top Types
- hardcoded_output: 229
- primitive_no_volatile: 177
- missing_latency_metric: 101
- missing_trace_point: 98
- resource_leaked_in_exception: 69
- unchecked_cuda_call: 65
- unordered_container_iter: 33
- legacy_or_compat_path: 29
- missing_resource_limits: 27
- thread_join_no_timeout: 26
### Top Scanners
- Uniform::audit_logging: 234
- Uniform::observability: 204
- Uniform::phase1_thread_safety: 203
- Uniform::exception_safety: 72
- Uniform::gpu_memory: 69
- Uniform::performance_patterns: 62
- Uniform::llm_ai_safety: 47
- Uniform::determinism: 46
- Uniform::type_conversion: 38
- Uniform::legacy_duplication: 29
### Top Path Prefixes
- benchmarks: 1079

## third_party
### Severity
- HIGH: 159345
- CRITICAL: 28778
- MEDIUM: 19311
### Top Types
- shared_state_no_sync: 91967
- resource_leaked_in_exception: 19030
- pointer_arithmetic_unbounded: 14255
- array_bounds_violation: 12080
- explicit_delete: 7483
- delete_without_nullptr: 6608
- primitive_no_volatile: 5260
- unchecked_array_index: 4786
- blocking_no_timeout: 4326
- missing_move_constructor_defaulted: 3783
### Top Scanners
- Uniform::phase1_thread_safety: 98272
- Uniform::phase1_memory_safety: 39638
- Uniform::exception_safety: 23935
- Uniform::phase1_raii: 9837
- Uniform::uninitialized: 9052
- Uniform::input_validation: 8002
- Uniform::phase1_error_handling: 7120
- Uniform::type_conversion: 6983
- Uniform::virtual_oop: 4595
### Top Path Prefixes
- vcpkg: 146908
- vcpkg_installed: 15100
- vcpkg_installed_linux: 13526
- ffmpeg: 8764
- stable-diffusion.cpp: 6400
- llama.cpp: 5763
- whisper.cpp: 5319
- releases: 5131
- build-msvc-windows-release: 490
- fuzz: 13
