# ThemisDB Source Code Audit Report
Generated: /home/runner/work/ThemisDB/ThemisDB
Total Issues: 7141

## Summary Statistics

- **Intrinsics No Fallback**: 80
- **Missing Export Macros**: 6866
- **Templates No Instantiation**: 39
- **Unguarded Platform Code**: 156

## HIGH Priority Issues (6866)

### Missing Export Macro (6866)

**include/raid_data_pusher.h:36**
- Description: Class 'RAIDDataPusher' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**include/raid_data_pusher.h:39**
- Description: Class 'ShardConfig' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**include/raid_data_pusher.h:45**
- Description: Class 'PushResult' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**include/raid_data_pusher.h:53**
- Description: Class 'MetricsSnapshot' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/rocksdb_writer.h:34**
- Description: Class 'RocksDBWriter' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/validator.h:34**
- Description: Class 'Validator' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/docs_builder.h:40**
- Description: Class 'BuilderConfig' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/docs_builder.h:90**
- Description: Class 'BuildStats' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/docs_builder.h:106**
- Description: Class 'DocsBuilder' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/document_parser.h:35**
- Description: Class 'Document' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**tools/themis_docs_builder/include/document_parser.h:43**
- Description: Class 'DocumentParser' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_llm_evaluator.h:41**
- Description: Class 'EvaluationResult' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_llm_evaluator.h:65**
- Description: Class 'IScraperLLMEvaluator' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_llm_evaluator.h:97**
- Description: Class 'ScraperLLMEvaluator' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_llm_evaluator.h:136**
- Description: Class 'InMemoryLLMEvaluator' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_plugin.h:54**
- Description: Class 'ScrapedDocument' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_plugin.h:81**
- Description: Class 'ScraperRunStats' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_plugin.h:102**
- Description: Class 'IScraperPlugin' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_plugin.h:132**
- Description: Class 'ScraperPlugin' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

**plugins/scraper/include/scraper_search_engine.h:41**
- Description: Class 'SearchForm' in public header lacks export macro
- Suggestion: Add THEMIS_*_API macro before class declaration

... and 6846 more

## MEDIUM Priority Issues (236)

### Intrinsic No Fallback (80)

**tests/test_replication_ha.cpp:1454**
- Description: Compiler intrinsic '_mm_writes_total' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/performance/intelligent_prefetcher.cpp:50**
- Description: Compiler intrinsic '__builtin_prefetch' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/performance/intelligent_prefetcher.cpp:51**
- Description: Compiler intrinsic '__builtin_prefetch' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/performance/intelligent_prefetcher.cpp:52**
- Description: Compiler intrinsic '__builtin_prefetch' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/performance/intelligent_prefetcher.cpp:53**
- Description: Compiler intrinsic '__builtin_prefetch' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/storage/simd_filter.cpp:105**
- Description: Compiler intrinsic '__builtin_ctz' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/storage/nvme_manager.cpp:341**
- Description: Compiler intrinsic '__atomic_store_n' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/storage/nvme_manager.cpp:381**
- Description: Compiler intrinsic '__atomic_store_n' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/storage/nvme_manager.cpp:420**
- Description: Compiler intrinsic '__atomic_load_n' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/storage/nvme_manager.cpp:431**
- Description: Compiler intrinsic '__atomic_store_n' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_mt.cpp:133**
- Description: Compiler intrinsic '_mm_add_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_mt.cpp:134**
- Description: Compiler intrinsic '_mm_hadd_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_mt.cpp:135**
- Description: Compiler intrinsic '_mm_hadd_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_mt.cpp:136**
- Description: Compiler intrinsic '_mm_cvtss_f32' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/vec_knn.cpp:138**
- Description: Compiler intrinsic '_mm_add_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/vec_knn.cpp:139**
- Description: Compiler intrinsic '_mm_hadd_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/vec_knn.cpp:140**
- Description: Compiler intrinsic '_mm_hadd_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/vec_knn.cpp:141**
- Description: Compiler intrinsic '_mm_cvtss_f32' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_tbb.cpp:113**
- Description: Compiler intrinsic '_mm_add_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

**src/acceleration/cpu_backend_tbb.cpp:114**
- Description: Compiler intrinsic '_mm_hadd_ps' without fallback
- Suggestion: Add preprocessor check and fallback implementation

... and 60 more

### Unguarded Platform Code (156)

**src/main_server.cpp:106**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:125**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:195**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:205**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:1533**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:2177**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**src/main_server.cpp:107**
- Description: Platform-specific code '<unistd.h>' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_new_aql_functions.cpp:351**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_new_aql_functions.cpp:511**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_legal_training_schema.cpp:397**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_encryption_e2e.cpp:451**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_shard_resource_manager.cpp:181**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_content_errors.cpp:252**
- Description: Platform-specific code 'win32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_wire_protocol_optimizations.cpp:577**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_directx_backend.cpp:432**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_content_security.cpp:111**
- Description: Platform-specific code 'Win32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_content_security.cpp:120**
- Description: Platform-specific code 'Win32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_module_sandbox.cpp:30**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_module_sandbox.cpp:33**
- Description: Platform-specific code '<unistd.h>' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

**tests/test_plugin_security_audit.cpp:368**
- Description: Platform-specific code 'WIN32' without preprocessor guard
- Suggestion: Add #ifdef for platform (e.g., #ifdef _WIN32)

... and 136 more

## LOW Priority Issues (39)

### Template No Instantiation (39)

**tests/integration/test_helpers.h:1**
- Description: Header has 2 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/feature_flags_examples.h:1**
- Description: Header has 2 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/rcu_hash_table.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/lockfree_metrics_buffer.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/rcu.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/lirs_cache.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/performance/lockfree_histogram.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/acceleration/compute_future.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/replication/crdt_types.h:1**
- Description: Header has 7 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/index/learned_index.h:1**
- Description: Header has 3 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/concurrent_cache.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/batch_operation_manager.h:1**
- Description: Header has 2 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/thread_safety.h:1**
- Description: Header has 4 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/expected.h:1**
- Description: Header has 3 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/memory_utils.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/utils/lossless_vector_compression.h:1**
- Description: Header has 2 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/auth/auth_metrics.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/auth/secure_memory.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/config/lru_cache.h:1**
- Description: Header has 1 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

**include/query/result_stream.h:1**
- Description: Header has 3 template(s) without explicit instantiation
- Suggestion: Consider adding explicit instantiations or .tpp file

... and 19 more

## Top 20 Problematic Files

| File | Issues |
|------|--------|
| include/replication/replication_manager.h | 65 |
| include/query/functions/date_functions.h | 54 |
| include/query/aql_parser.h | 40 |
| include/query/functions/collection_functions.h | 39 |
| include/query/query_engine.h | 30 |
| include/ingestion/ingestion_manager.h | 27 |
| include/analytics/cep_engine.h | 27 |
| include/query/functions/geo_functions.h | 26 |
| include/sharding/sharding_interfaces.h | 25 |
| include/query/functions/math_functions.h | 25 |
| include/acceleration/compute_backend.h | 23 |
| include/query/functions/vector_functions.h | 23 |
| include/utils/utils_interfaces.h | 22 |
| include/training/training_interfaces.h | 22 |
| src/replication/replication_manager.cpp | 21 |
| include/llm/ai_orchestrator.h | 21 |
| include/analytics/process_mining.h | 21 |
| include/query/functions/relational_functions.h | 21 |
| include/query/functions/document_functions.h | 21 |
| include/analytics/streaming_window.h | 20 |

