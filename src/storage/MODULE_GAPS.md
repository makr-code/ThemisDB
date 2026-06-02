# storage Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Remediation Log

| Batch | Commit | Files Fixed | Findings Addressed |
|---|---|---|---|
| W1-S01 | (columnar_format checksum) | columnar_format.cpp | 3 critical model_integrity_gap |
| W1-S02 | (nvme ring_mutex + O_CLOEXEC + WAL fixes) | nvme_manager.h, nvme_manager.cpp, wal_storage.cpp | 43 critical (37 data_race + 6 no_timeout) |
| W1-S03 | (history/compressed/HTD integrity + rocksdb/cache annotations) | history_manager.cpp, compressed_storage.cpp, hierarchical_tucker_decomposer.cpp, rocksdb_wrapper.cpp, columnar_cache.cpp | 9+5+5+41+11 critical |
| W1-S04 | (gpu_compression thread-safety) | include/storage/gpu_compression.h, src/storage/gpu_compression.cpp | 1 critical data_race (+ prompt_injection false positives annotated) |
| W1-S05 | (wal_storage no_timeout/array_bounds; HTD data_race; columnar_format prompt_injection; zero_copy no_timeout/missing_dtor; base_entity data_race/model_integrity; tt_quantizer model_integrity/array_bounds; blob_backend_s3 prompt_injection/no_timeout) | wal_storage.cpp, hierarchical_tucker_decomposer.cpp, columnar_format.cpp, zero_copy_blob_transfer.cpp, base_entity.cpp, tt_quantizer.cpp, blob_backend_s3.cpp | 12+9+8+5+5+4+4 critical |
| W1-S06 | (wom_tree data_race; tensor_network iterator_invalidation/data_race/model_integrity; storage_audit_logger no_timeout; database_connection_manager no_timeout/smart_ptr/iterator_invalidation; ggml_tensor_bridge data_race; tensor_compaction_filter model_integrity; streaming_ingest_manager smart_ptr/no_timeout; raft_mvcc_bridge missing_consensus; transaction_retry_manager data_race; security_signature model_integrity; compaction_manager uncategorized phantom) | wom_tree.cpp, tensor_network_storage_engine.cpp, storage_audit_logger.cpp, database_connection_manager.cpp, ggml_tensor_bridge.cpp, tensor_compaction_filter.cpp, streaming_ingest_manager.cpp, raft_mvcc_bridge.cpp, transaction_retry_manager.cpp, security_signature.cpp, compaction_manager.cpp | 3+3+3+4+4+4+3+1+1+1+1 critical |
| W1-S07 | (blob_redundancy_manager iterator_invalidation/no_timeout/uncategorized phantom; compression_strategy iterator_invalidation; concurrent_write_controller no_timeout; erasure_coder_factory uncategorized phantom; storage_engine no_timeout) | blob_redundancy_manager.cpp, compression_strategy.cpp, concurrent_write_controller.cpp, erasure_coder_factory.cpp, storage_engine.cpp | 3+2+3+2+2 critical |
| W1-S08 | (backup_manager data_race/iterator_invalidation; tensor_train_decomposer model_integrity_gap; gguf_metadata prompt_injection/model_integrity_gap; erasure_coding_backend data_race/iterator_invalidation; mvcc_store data_race; pitr_manager data_race; hamming_coder data_race/iterator_invalidation; distributed_transaction_manager missing_version_tracking; tensor_router uncategorized phantom; tiered_storage iterator_invalidation; blob_backend_azure uncategorized phantom) | backup_manager.cpp, tensor_train_decomposer.cpp, gguf_metadata.cpp, erasure_coding_backend.cpp, mvcc_store.cpp, pitr_manager.cpp, hamming_coder.cpp, distributed_transaction_manager.cpp, tensor_router.cpp, tiered_storage.cpp, blob_backend_azure.cpp | 2+2+2+2+2+2+2+1+1+1+1 critical |
| W1-S09 | (gpu_compression prompt_injection/unsanitized_llm_input/use_after_free_gpu/gpu_memory_leak/data_race false-positive annotations for binary compression paths and synchronized CUDA lifecycle) | gpu_compression.cpp | 11 critical scanner findings annotated |
| W1-S10 | (rocksdb_wrapper data_race/no_timeout scanner false-positive annotations for constructor-only option wiring, synchronized lifecycle barriers, and RocksDB-managed stats/backup probes) | rocksdb_wrapper.cpp | 41 critical scanner findings annotated |
| W1-S11 | (blob_backend_azure initialization hardening for graceful failure paths to eliminate null-client dereference risk when optional Azure SDK client bootstrap fails) | blob_backend_azure.cpp | 1 critical availability/null-deref risk fixed |
| W1-S12 | (HIGH false-positive scanner annotation pass: null_dereference/pointer_arithmetic/uncaught_exception/uninitialized_access/size_assumption/audit_logging/no_retry_logic/observability/db_connection_leak/range_temporary/posix_only_api/windows_only_api/lock_in_loop/repeated_search/unspecified_consistency/legacy_duplication/unvalidated_llm_output annotations; uncategorized Line-0 noise comments) | rocksdb_wrapper.cpp, distributed_transaction_manager.cpp, blob_backend_s3.cpp, backup_manager.cpp, wom_tree.cpp, ggml_tensor_bridge.cpp, nvme_manager.cpp, database_connection_manager.cpp, blob_redundancy_manager.cpp, erasure_coder_factory.cpp, hlc.cpp, merge_operators.cpp | 226 HIGH findings annotated as false positives |
| W1-S13 | (HIGH false-positive scanner annotation pass: size_assumption/o_n_squared/pointer_arithmetic/unsanitized_llm_input/unchecked_cuda_call/null_dereference/uncaught_exception/memory_order/db_connection_leak/lock_in_loop/lock_contention/range_temporary/observability/repeated_search/legacy_duplication/delete_no_nullptr/audit_logging/hardcoded_output annotations; uncategorized Line-0 noise comments) | columnar_format.cpp, gpu_compression.cpp, tensor_train_decomposer.cpp, online_schema_migration.cpp, storage_engine.cpp, encrypted_blob_backend.cpp, gguf_metadata.cpp, erasure_coding_backend.cpp, concurrent_write_controller.cpp, storage_audit_logger.cpp, index_maintenance.cpp | ~233 HIGH findings annotated as false positives |
| W1-S14 | (blob_redundancy_manager remediation slice: eliminate O(n²) datacenter dedupe in full scrub via hash-set, pre-reserve vectors during shard/location JSON conversion paths to reduce allocation-loop/missing-reserve findings) | blob_redundancy_manager.cpp | HIGH+MEDIUM performance/container findings reduced in one hotspot |
| W1-S15 | (blob_redundancy_manager follow-up: remove string-concat-in-loop hotspot in verifyBlob logging; switch GEO healthy-DC tracking to hash-set; add reserve() in blob/shard collection paths to reduce allocation-loop and copy-overhead findings) | blob_redundancy_manager.cpp | HIGH+MEDIUM performance/container findings reduced in verification and queueing paths |
| W1-S16 | (blob_redundancy_manager queueing/tier-down follow-up: pre-reserve metadata location vector at registration, avoid notifying repair CV while queue mutex is held, reserve full scrub degraded-id capacity from current blob cardinality, and precompute tier-down loop bound) | blob_redundancy_manager.cpp | Additional medium performance/contention findings reduced in queueing and maintenance paths |
| W1-S17 | (blob_redundancy_manager locking/invalidation follow-up: replace find+erase iterator path with key erase in unregisterBlob and refactor runRepairQueue to lock only around queue pop without explicit re-lock calls) | blob_redundancy_manager.cpp | In-scope critical iterator_invalidation/no_timeout lock-handling findings reduced in queue processing lifecycle |
| W1-S18 | (HIGH/CRITICAL annotation pass across 8 files: tensor_network_storage_engine.cpp lock_in_loop false positives; tensor_router.cpp llm_ai_safety/uncaught_exception/pointer_arithmetic/deadlock_risk/null_dereference false positives; index_analyzer.cpp uncaught_exception/lock_in_loop/lock_contention/range_temporary false positives; adaptive_compaction.cpp lock_in_loop cv::wait_for false positive; blob_backend_gcs.cpp null_dereference/pointer_arithmetic/delete_no_nullptr false positives in #ifdef blocks; disk_space_monitor.cpp deadlock_risk sequential-lock false positives; blob_backend_webdav.cpp uninitialized_access/null_dereference CURL callback false positives; vector_index_backend.cpp uncaught_exception intentional precondition guards) | tensor_network_storage_engine.cpp, tensor_router.cpp, index_analyzer.cpp, adaptive_compaction.cpp, blob_backend_gcs.cpp, disk_space_monitor.cpp, blob_backend_webdav.cpp, vector_index_backend.cpp | All remaining actionable HIGH/CRITICAL findings annotated as reviewed false positives |
| W1-S19 | (simd_filter remediation: detectSIMDLevel cache load/store upgraded to acquire/release semantics; SIMD/scalar kernels now pre-reserve output capacity and scanBatch pre-reserves aggregated row capacity to reduce missing-reserve/copy-overhead hotspots) | simd_filter.cpp | High memory_order finding addressed and medium performance reserve hotspots reduced across scalar/AVX2/NEON paths |
| W1-S20 | (HIGH false-positive annotation pass across 10 files: distributed_transaction_manager.cpp — extended uninitialized_access annotation (lines 149-252), lock_contention/no_retry_logic per-iteration shard copy, deadlock_risk/null_dereference in registerShard; tt_quantizer.cpp — pointer_arithmetic vector-element accesses and uncaught_exception for unsupported quant type; hlc.cpp — memory_order relaxed CAS-loop initialisation and db_connection_leak scanner artifact; backup_manager.cpp — range_temporary annotation extended to lines 2519/2550; storage_audit_logger.cpp — deadlock_risk sequential mutex acquisitions; pitr_manager.cpp — uncaught_exception constructor preconditions, repeated_search small-list find, delete_no_nullptr method call; hamming_coder.cpp — uncaught_exception preconditions, pointer_arithmetic shard index, o_n_squared inherent parity algorithm, uninitialized_access vector<bool>; tiered_storage.cpp — uncaught_exception sanitizeKey, delete_no_nullptr deleteFromTier method, lock_contention workerLoop wait_for; mvcc_store.cpp — uncaught_exception constructor null check, null_dereference string_view reinterpret_cast; streaming_ingest_manager.cpp — uncaught_exception factory precondition, db_connection_leak scanner artifact, lock_contention condition-variable worker) | distributed_transaction_manager.cpp, tt_quantizer.cpp, hlc.cpp, backup_manager.cpp, storage_audit_logger.cpp, pitr_manager.cpp, hamming_coder.cpp, tiered_storage.cpp, mvcc_store.cpp, streaming_ingest_manager.cpp | All remaining actionable HIGH findings annotated as reviewed false positives; no genuine bugs introduced |
| W1-S22 | (final annotation/remediation pass: nlp_metadata_extractor — add reserve() for keywords loop + annotate entity-dispatch push_backs; security_signature_manager — annotate snprintf hex-encode hardcoded_output FP + narrow all catch(...) to std::exception; storage_parquet_exporter — annotate Line-0 phantom + size_assumption FP for sizeof(int64_t); blob_backend_filesystem — annotate Line-7 header-comment scanner artifact + precondition throw + narrow catch; key_schema — annotate legacy_duplication FPs for intentional backward-compat paths; schema_dead_weight_detector + storage_layout_advisor — confirm stale reserve findings already fixed; mvcc_chain_pruner — annotate push_back inside scanVersions callback) | nlp_metadata_extractor.cpp, security_signature_manager.cpp, storage_parquet_exporter.cpp, blob_backend_filesystem.cpp, key_schema.cpp, schema_dead_weight_detector.cpp, storage_layout_advisor.cpp, mvcc_chain_pruner.cpp | All remaining unannotated HIGH/MEDIUM findings reviewed; 1 genuine reserve() fix applied; all other findings annotated as reviewed false positives |
| W1-S23 | (storage perf hotpath follow-up: backup_manager winQuoteForCreateProcess now pre-reserves and appends without loop concatenation, restoreCollections precomputes requested-collection log buffer capacity; compressed_storage stats/message builders now use append/reserve patterns; history_manager append_crc32 pre-reserves trailer bytes) | backup_manager.cpp, compressed_storage.cpp, history_manager.cpp | Reduced string_concat_loop/copy_overhead allocation churn in issue-scope storage utility paths with behavior preserved |
| W1-S24 | (storage perf follow-up: compression_strategy RLE and dictionary codecs now pre-reserve output buffers for worst-case/heuristic growth and dictionary working sets to reduce push_back reallocation churn in encode/decode loops) | compression_strategy.cpp | Reduced missing-reserve/copy_overhead pressure in issue-scope codec hotpaths with behavior preserved |
| W1-S25 | (final annotation pass across 6 remaining files: raft_mvcc_bridge — annotate uncaught_exception constructor preconditions (lines 33/36) and unspecified_consistency snapshotRead (line 100); transaction_retry_manager — annotate 2 Line-0 phantom uncategorized findings + narrow catch(...) to std::exception in invokeAlertCallback; compaction_manager — annotate lock_contention cv::wait_for pattern (line 125), hardcoded_output scanner misread of comment text (line 185), narrow catch(...) to std::exception for stats parsing; security_signature — annotate model_integrity_gap with explicit validation evidence (isValidResourceId/isHexLowerString/isSupportedAlgorithm) + annotate already-specific catch(std::exception&) at lines 51/64; blob_backend_webdav — annotate Line-7 header-comment uninitialized_access artifact + Line-0 uncategorized ReadData phantom; vector_index_backend — annotate missing_vector_reserve/copy_overhead findings as stale since reserve() already precedes push_back loop) | raft_mvcc_bridge.cpp, transaction_retry_manager.cpp, compaction_manager.cpp, security_signature.cpp, blob_backend_webdav.cpp, vector_index_backend.cpp | All remaining unannotated findings reviewed and annotated as reviewed false positives or false-alarm scanner artifacts; all genuine fixes (catch narrowing) applied |
| W1-S26 | (storage audit writer hardening: writeEntry now handles EINTR and partial writes with a full write loop before advancing sequence/segment counters) | storage_audit_logger.cpp | Eliminated partial-write data loss risk and improved robustness of local audit-log persistence path |
| W1-S27 | (nvme fallback I/O hardening: submitRead/submitWrite synchronous paths now retry on EINTR and require full transfer completion; added regression tests for short-read failure and full-buffer fallback writes) | nvme_manager.cpp, test_nvme_manager.cpp | Eliminated partial-transfer acceptance risk in non-io_uring fallback path and added focused regression coverage for strict transfer semantics |
| W1-S28 | (annotation + genuine-fix pass across 6 files; all false positives explicitly identified with reasoning): **hierarchical_tucker_decomposer.cpp** — uninitialized_access (expandNode return), fp_exact_comparison (fl==0.0f guard), legacy_duplication (toTTTrain bridge comment), model_integrity_gap (CRC32 verified path in deserializeNode/HTTrain::deserialize), null_dereference (readFloats after CRC guard), pointer_arithmetic (standard C++ iterators in modeKUnfolding/modeKProduct), and 2× Line-0 phantom FP — all **false positives**. **columnar_cache.cpp** — size_assumption (sizeof(int64_t)==8 target guarantee), lock_in_loop/deadlock_risk (clear() runs callback outside lock; pinnedCount() accesses shared range under single lock) — all **false positives**. **wom_tree.cpp** — null_dereference in splitLeaf (right->data.front() guaranteed non-empty by branch), Line-0 phantom — **false positives**. **database_connection_manager.cpp** — db_connection_leak/lock_contention in acquireConnection and getConnectionHealth (no resource opened; single lock held throughout), lock_in_loop in getStats (mutex held for full loop), pointer_arithmetic on structured-binding loops — all **false positives**. **ggml_tensor_bridge.cpp** — CRITICAL Line-0 uncategorized ×2 (phantom scanner artifacts score=0.85), null_dereference on handle.impl_ (assigned by make_unique immediately above) — all **false positives**. **tensor_compaction_filter.cpp** — Line-0 HIGH uncategorized ×8 (phantom), pointer_arithmetic (reinterpret_cast+bounded size) — **false positives**; `catch(...)` narrowed to `catch(std::exception&)` at lines 155 and 175 — **genuine fix** (prevented non-std exception types from being silently swallowed). | hierarchical_tucker_decomposer.cpp, columnar_cache.cpp, wom_tree.cpp, database_connection_manager.cpp, ggml_tensor_bridge.cpp, tensor_compaction_filter.cpp | All unannotated CRITICAL/HIGH findings across 6 files reviewed, explicitly classified as false positives with reasoning, and annotated inline; one genuine bug fixed (catch narrowing in tensor_compaction_filter) |
| W1-S29 | (exception-safety hardening pass: replace broad catch(...) handlers with typed std::exception/std::future_error handling in quantization, tensor routing pilot path, GGUF HMAC injection path, RocksDB stats parsing paths, and concurrent-write promise signaling) | tt_quantizer.cpp, tensor_router.cpp, gguf_metadata.cpp, index_maintenance.cpp, index_analyzer.cpp, concurrent_write_controller.cpp | Reduced generic exception swallowing in issue-scope storage paths while preserving fail-closed behavior and fallback semantics |
| W1-S30 | (exception-safety hardening follow-up: narrow broad catch(...) handlers in entity parsing, history/conflict deserialization, TT train deserialization, and NLP metadata serialization/parsing paths to typed std::exception handling) | base_entity.cpp, history_manager.cpp, tensor_train_decomposer.cpp, nlp_metadata_extractor.cpp | Further reduced generic exception swallowing in issue-scope storage paths while preserving existing nullopt/false fallback behavior |
| W1-S31 | (exception-safety hardening + cleanup RAII follow-up: encrypted_blob_backend now uses unique_ptr-managed EVP_CIPHER_CTX in encrypt/decrypt to remove generic cleanup catch blocks; concurrent_write_controller acquire() future-get rejection paths narrowed to std::exception; rocksdb_wrapper merge fallback and backup-count probe catches narrowed to std::exception) | encrypted_blob_backend.cpp, concurrent_write_controller.cpp, rocksdb_wrapper.cpp | Reduced additional generic catch(...) usage in issue-scope storage paths while preserving fallback/rethrow behavior |

## Scan Snapshot

- Module: storage
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 1071
- Actionable Findings (Critical + High): 678
- Affected Files: 61

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 212 |
| High | 466 |
| Medium | 381 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 168 |
| container | 136 |
| exception_safety | 120 |
| reliability | 108 |
| concurrency | 106 |
| llm_ai_safety | 79 |
| platform | 64 |
| memory | 56 |
| raii | 54 |
| security | 44 |
| gpu_memory_safety | 36 |
| audit_logging | 32 |
| performance | 26 |
| legacy_duplication | 21 |
| observability | 21 |
| type_conversion | 9 |
| input_validation | 8 |
| determinism | 6 |
| oop_design | 6 |
| distributed_consistency | 5 |
| deprecated_apis | 3 |
| uninitialized | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/storage/columnar_format.cpp | 84 | 11 | 52 | 21 | 0 |
| src/storage/rocksdb_wrapper.cpp | 82 | 41 | 21 | 20 | 0 |
| src/storage/hierarchical_tucker_decomposer.cpp | 67 | 14 | 49 | 4 | 0 |
| src/storage/gpu_compression.cpp | 60 | 11 | 35 | 14 | 0 |
| src/storage/nvme_manager.cpp | 59 | 43 | 10 | 6 | 0 |
| src/storage/backup_manager.cpp | 43 | 1 | 22 | 20 | 0 |
| src/storage/blob_redundancy_manager.cpp | 41 | 2 | 9 | 30 | 0 |
| src/storage/simd_filter.cpp | 40 | 0 | 1 | 39 | 0 |
| src/storage/wom_tree.cpp | 38 | 1 | 29 | 8 | 0 |
| src/storage/tt_quantizer.cpp | 35 | 4 | 8 | 23 | 0 |
| src/storage/tensor_train_decomposer.cpp | 34 | 2 | 25 | 7 | 0 |
| src/storage/database_connection_manager.cpp | 26 | 3 | 15 | 8 | 0 |
| src/storage/erasure_coder_factory.cpp | 26 | 2 | 3 | 21 | 0 |
| src/storage/distributed_transaction_manager.cpp | 23 | 1 | 21 | 1 | 0 |
| src/storage/storage_audit_logger.cpp | 23 | 3 | 10 | 3 | 7 |
| src/storage/blob_backend_s3.cpp | 21 | 4 | 17 | 0 | 0 |
| src/storage/index_maintenance.cpp | 21 | 0 | 6 | 15 | 0 |
| src/storage/wal_storage.cpp | 20 | 12 | 3 | 5 | 0 |
| src/storage/history_manager.cpp | 17 | 9 | 2 | 6 | 0 |
| src/storage/hlc.cpp | 17 | 0 | 16 | 1 | 0 |
| src/storage/online_schema_migration.cpp | 16 | 0 | 11 | 5 | 0 |
| src/storage/concurrent_write_controller.cpp | 15 | 2 | 9 | 4 | 0 |
| src/storage/base_entity.cpp | 14 | 5 | 2 | 7 | 0 |
| src/storage/gguf_metadata.cpp | 14 | 2 | 4 | 8 | 0 |
| src/storage/nlp_metadata_extractor.cpp | 14 | 0 | 0 | 14 | 0 |
| src/storage/tensor_compaction_filter.cpp | 14 | 4 | 8 | 2 | 0 |
| src/storage/merge_operators.cpp | 12 | 0 | 12 | 0 | 0 |
| src/storage/security_signature_manager.cpp | 12 | 0 | 0 | 12 | 0 |
| src/storage/tensor_router.cpp | 12 | 1 | 6 | 1 | 4 |
| src/storage/zero_copy_blob_transfer.cpp | 11 | 5 | 2 | 4 | 0 |
| src/storage/compressed_storage.cpp | 10 | 6 | 0 | 4 | 0 |
| src/storage/compression_strategy.cpp | 10 | 1 | 2 | 7 | 0 |
| src/storage/erasure_coding_backend.cpp | 10 | 1 | 2 | 7 | 0 |
| include/storage/examples/schema_layout_advisor_example.cpp | 9 | 0 | 1 | 8 | 0 |
| src/storage/adaptive_compaction.cpp | 9 | 0 | 8 | 0 | 1 |
| src/storage/blob_backend_azure.cpp | 9 | 1 | 8 | 0 | 0 |
| src/storage/columnar_cache.cpp | 8 | 2 | 1 | 5 | 0 |
| src/storage/index_analyzer.cpp | 8 | 0 | 4 | 4 | 0 |
| src/storage/pitr_manager.cpp | 8 | 2 | 3 | 3 | 0 |
| src/storage/storage_engine.cpp | 8 | 2 | 6 | 0 | 0 |
| src/storage/tensor_network_storage_engine.cpp | 8 | 2 | 2 | 4 | 0 |
| src/storage/mvcc_store.cpp | 7 | 0 | 0 | 7 | 0 |
| src/storage/tiered_storage.cpp | 7 | 0 | 2 | 5 | 0 |
| src/storage/streaming_ingest_manager.cpp | 6 | 3 | 2 | 1 | 0 |
| src/storage/ggml_tensor_bridge.cpp | 5 | 3 | 2 | 0 | 0 |
| src/storage/compaction_manager.cpp | 4 | 1 | 2 | 1 | 0 |
| src/storage/disk_space_monitor.cpp | 4 | 0 | 3 | 1 | 0 |
| src/storage/hamming_coder.cpp | 4 | 2 | 1 | 1 | 0 |
| src/storage/storage_parquet_exporter.cpp | 4 | 0 | 2 | 2 | 0 |
| src/storage/transaction_retry_manager.cpp | 4 | 1 | 2 | 1 | 0 |
| src/storage/encrypted_blob_backend.cpp | 3 | 0 | 0 | 3 | 0 |
| src/storage/security_signature.cpp | 3 | 1 | 0 | 2 | 0 |
| src/storage/blob_backend_filesystem.cpp | 2 | 0 | 1 | 1 | 0 |
| src/storage/blob_backend_webdav.cpp | 2 | 0 | 1 | 1 | 0 |
| src/storage/raft_mvcc_bridge.cpp | 2 | 1 | 1 | 0 | 0 |
| src/storage/blob_backend_gcs.cpp | 1 | 0 | 1 | 0 | 0 |
| src/storage/key_schema.cpp | 1 | 0 | 1 | 0 | 0 |
| src/storage/mvcc_chain_pruner.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/schema_dead_weight_detector.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/storage_layout_advisor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/vector_index_backend.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/storage/columnar_format.cpp
Total findings: 84

- Line 750: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.99
- Line 751: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.99
- Line 752: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 755: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.99
- Line 888: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.99
- Line 889: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.99
- Line 890: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 893: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.99
- Line 1258: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: Result<ColumnSegment> ColumnSegment::deserialize(const std::vector<uint8_t>& data) {
  Confidence: band=very_high; score=0.99
- Line 1262: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: insufficient data"
  Confidence: band=very_high; score=0.99
- Line 1288: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: truncated data"
  Confidence: band=very_high; score=0.99
- Line 82: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.reserve(data.size() * sizeof(int64_t) / 2);
- Line 97: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int64_t));
- Line 136: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (pos + 1 + sizeof(int64_t) > encoded.size()) {
- Line 146: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&value, &encoded[pos], sizeof(int64_t));
- Line 147: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 171: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dictionary.find(str);
- Line 423: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 449: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), min_bytes, min_bytes + sizeof(int64_t));
- Line 474: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 486: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), val_bytes, val_bytes + sizeof(int64_t));
- Line 517: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint8_t);
- Line 519: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint16_t);
- Line 543: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 545: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 546: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint16_t);
- Line 563: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (encoded.size() < sizeof(int64_t) + 1 + sizeof(uint32_t)) {
- Line 573: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&min_val, &encoded[pos], sizeof(int64_t));
- Line 574: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 586: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint8_t);
- Line 588: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint16_t);
- Line 592: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(int64_t);
- Line 612: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 614: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 615: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint16_t);
- Line 626: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(int64_t) <= encoded.size(); ++i) {
- Line 628: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(int64_t));
- Line 629: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 674: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), ref_bytes, ref_bytes + sizeof(int64_t));
- Line 679: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int64_t));
- Line 714: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (encoded.size() < sizeof(int64_t)) {
- Line 724: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&reference, &encoded[pos], sizeof(int64_t));
- Line 725: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 730: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: while (pos + sizeof(int64_t) <= encoded.size()) {
- Line 732: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&delta, &encoded[pos], sizeof(int64_t));
- Line 733: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 750: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.9
- Line 751: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 755: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.9
- Line 776: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "LZ4 compression: failed to allocate output buffer"
- Line 845: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "LZ4 decompression: failed to allocate output buffer"
- Line 888: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.9
- Line 889: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.9
- Line 890: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 893: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.9
- Line 907: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Snappy compression: failed to allocate output buffer"
- Line 958: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Snappy decompression: failed to allocate output buffer"
- Line 1058: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const void* data,
- Line 1244: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: serialized.insert(serialized.end(), bytes, bytes + sizeof(uint64_t));
- Line 1259: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (data.size() < 2 + 4 * sizeof(uint64_t)) {
- Line 1274: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&val, &data[pos], sizeof(uint64_t));
- Line 1275: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint64_t);
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> dictionary;
  Confidence: band=medium; score=0.66
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dict_values.push_back(str);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(it->second);
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(normalized + min_val);
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 622: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 630: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(normalized + min_val);
- Line 707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(reference + delta);
- Line 735: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(reference + delta);
- Line 1386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(segments[idx]);
  Confidence: band=high; score=0.74
- Line 1408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matching_indices.push_back(i);
  Confidence: band=high; score=0.74

### src/storage/rocksdb_wrapper.cpp
Total findings: 82

- Line 88: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 89: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->write_buffer_size = config_.memtable_size_mb * 1024 * 1024;
- Line 265: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_write_buffer_number = config_.max_write_buffer_number;
- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->min_write_buffer_number_to_merge = config_.min_write_buffer_number_to_merge;
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_jobs = config_.max_background_jobs;
- Line 326: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_compactions = config_.max_background_compactions;
- Line 329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_flushes = config_.max_background_flushes;
- Line 332: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_subcompactions = config_.max_subcompactions;
- Line 339: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level_compaction_dynamic_level_bytes = config_.dynamic_level_bytes;
- Line 340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->target_file_size_base = config_.target_file_size_base_mb * 1024ull * 1024ull;
- Line 341: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_bytes_for_level_base = config_.max_bytes_for_level_base_mb * 1024ull * 1024ull;
- Line 344: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_file_num_compaction_trigger = config_.level0_file_num_compaction_trigger;
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_slowdown_writes_trigger = config_.level0_slowdown_writes_trigger;
- Line 346: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_stop_writes_trigger = config_.level0_stop_writes_trigger;
- Line 353: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->db_write_buffer_size = config_.db_write_buffer_size_mb * 1024ull * 1024ull;
- Line 369: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->compression = toCompression(config_.compression_default);
- Line 370: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->bottommost_compression = toCompression(config_.compression_bottommost);
- Line 382: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_concurrent_memtable_write = config_.allow_concurrent_memtable_write;
- Line 412: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: write_options_->disableWAL = config_.disable_wal_for_benchmark;  // Phase 2F: Benchmark optimization
- Line 414: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->wal_dir = config_.wal_dir;
- Line 452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_jobs = static_cast<int>(recommended_threads);
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->use_direct_reads = config_.use_direct_reads;
- Line 483: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->two_write_queues = config_.two_write_queues;
- Line 483: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->two_write_queues = config_.two_write_queues;
- Line 491: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: txn_options_->set_snapshot = true; // Automatically create snapshot on begin
- Line 516: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->paranoid_checks = config_.paranoid_checks;
- Line 519: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: read_options_->verify_checksums = config_.verify_checksums_on_read;
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->wal_bytes_per_sync = config_.wal_bytes_per_sync;
- Line 541: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_mmap_reads = false;
- Line 544: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_mmap_writes = false;
- Line 548: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool RocksDBWrapper::open() {
- Line 603: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: db_opts.create_missing_column_families = options_->create_missing_column_families;
- Line 684: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_WARN("Database already open during open() - closing existing connection first");
- Line 1907: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t block_cache_hit = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 1908: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t block_cache_miss = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2108: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!open()) {
- Line 2269: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s = backup_engine->RestoreDBFromLatestBackup(config_.db_path, config_.db_path);
- Line 2277: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!open()) {
- Line 2338: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats_obj["block_cache_miss"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2339: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats_obj["block_cache_hit"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4596 perf(storage): fix ~79x sus... (2026-04-13) | #4494 [PERF-D5] Streaming
- Line 89: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 90: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&base, existing_value->data(), sizeof(uint64_t));
- Line 103: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (value.size() == sizeof(uint64_t)) {
- Line 104: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&delta, value.data(), sizeof(uint64_t));
- Line 108: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: new_value->resize(sizeof(uint64_t));
- Line 109: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(new_value->data(), &result, sizeof(uint64_t));
- Line 132: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (is_being_moved_.load(std::memory_order_acquire)) {
- Line 269: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Prefer HyperClockCache if available; fallback to LRUCache for compatibility
  Confidence: band=high; score=0.8
- Line 272: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use LRU cache universally for maximum compatibility.
  Confidence: band=high; score=0.8
- Line 468: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Skip setting unavailable TransactionDBOptions fields to preserve compatibility.
  Confidence: band=high; score=0.8
- Line 735: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (active_operations_.load(std::memory_order_acquire) > 0) {
- Line 736: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 878: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
- Line 1297: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1362: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWithIndexWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1565: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::TransactionWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1642: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RocksDBWrapper::TransactionWrapper::prepare() {
- Line 96: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 177: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paths.emplace_back(p.path, static_cast<uint64_t>(p.target_size_bytes));
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_handles_.emplace_back(cf_handles[i]);
  Confidence: band=high; score=0.74
- Line 718: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void RocksDBWrapper::close() {
- Line 757: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1031: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.emplace_back(std::async(
  Confidence: band=high; score=0.74
- Line 1266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74
- Line 1419: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1446: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: txn_.release();  // Intentional leak in rare edge case (DB shutdown)
- Line 1464: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1486: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1639: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: return Ok(txn_->GetSnapshot());
  Confidence: band=high; score=0.74
- Line 1898: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (level > 0) num_files_at_levels += ", ";
  Confidence: band=high; score=0.74
- Line 1899: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (level > 0) num_files_at_levels += ", ";
- Line 1900: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: num_files_at_levels += "\"L" + std::to_string(level) + "\": " + std::to_string(num_files);
- Line 2179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 2316: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74

### src/storage/hierarchical_tucker_decomposer.cpp
Total findings: 67

- Line 248: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::unique_ptr<HTNode> deserializeNode(Reader& r) {
  Confidence: band=very_high; score=0.99
- Line 256: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->rank = static_cast<std::size_t>(rank_u);
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->mode_index = static_cast<std::size_t>(mi);
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->n_k        = static_cast<std::size_t>(nk);
- Line 271: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->r_left  = static_cast<std::size_t>(rl);
- Line 272: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->r_right = static_cast<std::size_t>(rr);
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->left  = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->right = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HTTrain> HTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: ht.root = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 614: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: leaf_left->U          = U_cache[L];
- Line 622: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: leaf_right->U          = U_cache[L + 1];
- Line 707: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);
- Line 713: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 94: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return node.U;  // already stored as [n_k × rank] row-major
- Line 120: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fl == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // toTTTrain — compatibility bridge with memoization (stub #286 resolved)
  Confidence: band=high; score=0.8
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->rank = static_cast<std::size_t>(rank_u);
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->is_leaf = true;
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->mode_index = static_cast<std::size_t>(mi);
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->n_k        = static_cast<std::size_t>(nk);
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!r.readFloats(node->U)) return nullptr;
- Line 267: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->is_leaf = false;
- Line 271: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_left  = static_cast<std::size_t>(rl);
- Line 272: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_right = static_cast<std::size_t>(rr);
- Line 273: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!r.readFloats(node->B)) return nullptr;
- Line 274: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left  = deserializeNode(r);
- Line 275: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = deserializeNode(r);
- Line 470: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: mat[j * N_other + col] = data[flat];
- Line 503: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: val += U[ik * r + alpha] * data[o * n_k * stride_k + ik * stride_k + s];
- Line 551: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   SVD-1: unfold core along [L..M-1] vs [M..R-1, out]
- Line 552: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_left  [phys_L,...,phys_{M-1}, r_inner]
- Line 553: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_right_raw  [n_right * r_out, r_inner]
- Line 554: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   SVD-2: unfold G_right_raw as [n_right, r_out * r_inner]
- Line 555: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_right  [phys_M,...,phys_{R-1}, r_23]
- Line 556: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → B_node   [r_inner, r_23, r_out]  (transfer tensor at this node)
- Line 579: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->is_leaf    = true;
- Line 580: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->mode_index = L;
- Line 581: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->n_k        = n_L;
- Line 582: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->rank       = r_out;
- Line 583: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->U.assign(n_L * r_out, 0.0f);
- Line 590: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->U[i * r_out + ao] = val;
- Line 602: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->is_leaf  = false;
- Line 603: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_left   = phys_L;
- Line 604: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_right  = phys_R;
- Line 605: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->rank     = r_out;
- Line 606: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->B        = core;  // [phys_L × phys_R × r_out]
- Line 624: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left  = std::move(leaf_left);
- Line 625: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = std::move(leaf_right);
- Line 697: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->is_leaf  = false;
- Line 698: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_left   = r_inner;
- Line 699: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->r_right  = r_23;
- Line 700: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->rank     = r_out;
- Line 701: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->B        = std::move(B_node);
- Line 707: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);
- Line 713: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);
- Line 750: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto Tk = modeKUnfolding(data, shape, k);  // [nk × n_other]
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 893: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: root_core_shape.push_back(1);  // r_out = 1 at root
  Confidence: band=high; score=0.74

### src/storage/gpu_compression.cpp
Total findings: 60

- Line 154: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input to device
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.99
- Line 297: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_in
  Context: static_cast<uint8_t*>(d_in), size, cfg, result,
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.99
- Line 381: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.99
- Line 624: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.99
- Line 632: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.99
- Line 971: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.99
- Line 998: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = impl_->compress(data, size, algorithm, config_);
- Line 1351: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['//', 'static constexpr size_t kGpuMagicSize = 8;', 'static constexpr uint8_t kGpuMagic[kGpuMagicSize] = {', '    \'T\', \'G\', \'C\', \'P\', \'R\', \'S\', 1, 0   // "TGCPRS" + version 1.0', '};']
  Confidence: band=high; score=0.81
- Line 154: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input to device
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: result.error_message = std::string("cudaMemcpyAsync H2D: ") +
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 377: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 381: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 459: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs_arr, d_in_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs_arr, d_out_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(results[i].data.data() + hdr,
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: h_in_ptrs[i]  = const_cast<uint8_t*>(d_in) + i * chunk;
- Line 571: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 574: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 624: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.9
- Line 632: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.9
- Line 638: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs, h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 683: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 746: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 754: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaError_t e = cudaMemcpyAsync(h_in_ptrs[i], chunk_data, cs,
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D chunk[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 816: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs,  h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 868: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(result.data() + off, h_out_ptrs[i],
  Confidence: band=very_high; score=0.9
- Line 971: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.9
- Line 1340: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: static constexpr size_t kLz4HeaderSize = sizeof(uint64_t);
- Line 1351: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(compress(ptrs[i], sizes[i], algorithm, cfg));
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 750: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 1182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ptrs.push_back(buffers[idx].data());
  Confidence: band=high; score=0.74
- Line 1243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(decompress(compressed_buffers[i], algorithm, orig));
  Confidence: band=high; score=0.74

### src/storage/nvme_manager.cpp
Total findings: 59

- Line 221: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int dfd = ::open(probe_path, O_WRONLY | O_DIRECT, 0600);
- Line 288: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_ && ring_->ring_fd >= 0;
- Line 316: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring->sqes[index];
- Line 320: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->off       = static_cast<uint64_t>(req.offset);
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len       = static_cast<uint32_t>(req.len);
- Line 323: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 326: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,
- Line 356: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring->sqes[index];
- Line 360: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->off       = static_cast<uint64_t>(req.offset);
- Line 362: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len       = static_cast<uint32_t>(req.len);
- Line 363: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 366: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,
- Line 393: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 0, min_complete,
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const struct io_uring_cqe* cqe = &ring->cqes[head & *ring->cq_mask];
- Line 409: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: r.user_data = static_cast<int64_t>(cqe->user_data);
- Line 433: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDWR);
- Line 435: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_ERROR("NVMeManager::resetZone: open('{}') failed: {}",
- Line 462: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDWR);
- Line 464: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_ERROR("NVMeManager::finishZone: open('{}') failed: {}",
- Line 491: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDONLY);
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 600: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mmap_size = params.sq_off.array +
- Line 602: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mmap = ::mmap(nullptr, ring->sq_mmap_size,
- Line 610: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 614: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* sq_base = static_cast<uint8_t*>(ring->sq_mmap);
- Line 617: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mask  = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.ring_mask);
- Line 621: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap_size = params.sq_entries * sizeof(struct io_uring_sqe);
- Line 621: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap_size = params.sq_entries * sizeof(struct io_uring_sqe);
- Line 622: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 632: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 635: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqes = static_cast<struct io_uring_sqe*>(ring->sqe_mmap);
- Line 635: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqes = static_cast<struct io_uring_sqe*>(ring->sqe_mmap);
- Line 639: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mmap_size = params.cq_off.cqes +
- Line 641: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 653: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 656: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* cq_base = static_cast<uint8_t*>(ring->cq_mmap);
- Line 659: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mask = reinterpret_cast<uint32_t*>(cq_base + params.cq_off.ring_mask);
- Line 660: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cqes    = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);
- Line 686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ring->ring_fd >= 0) {
- Line 688: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Allocate a report buffer for 1 zone', '    constexpr size_t BUF_SIZE = sizeof(struct blk_zone_report) + sizeof(struct blk_zone);', '    alignas(alignof(struct blk_zone_report)) char buf[BUF_SIZE];', '    std::memset(buf, 0, BUF_SIZE);', '']
  Confidence: band=high; score=0.78
- Line 127: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 137: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 229: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: ::unlink(probe_path);  // Always clean up, after the O_DIRECT test
- Line 286: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return initialized_.load(std::memory_order_acquire) &&
- Line 324: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_array[index] = index;
- Line 364: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_array[index] = index;
- Line 409: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r.user_data = static_cast<int64_t>(cqe->user_data);
- Line 622: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 641: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        if (f.is_open()) {', '            uint32_t count = 1;', '            f >> count;', '            if (count > 0) return count;', '        }']
  Confidence: band=medium; score=0.65
- Line 218: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(tmp_fd);
- Line 224: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(dfd);
- Line 506: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 550: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 687: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring->ring_fd);

### src/storage/backup_manager.cpp
Total findings: 43

- Line 1722: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats->rto_seconds = static_cast<uint32_t>(
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4746 Add Q2 2026 Waveâ€‘1 qualit... (2026-04-21) | #3810 feat(storage): Impl
- Line 318: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(src_dir)) {
- Line 721: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 772: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(checkpoint_dir)) {
- Line 861: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(raid_topology_dir)) {
- Line 960: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 963: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "fork() failed when invoking tar");
- Line 1000: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1039: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 1042: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "fork() failed when invoking tar");
- Line 1072: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1118: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1244: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1650: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 1729: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(checkpoint_dir)) {
- Line 1897: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //    puts.  This avoids overwriting CFs outside the requested scope.
  Confidence: band=very_high; score=0.9
- Line 2097: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2135: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2517: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(snapshot_dir)) {
- Line 2548: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(snap_base, ec)) {
- Line 52: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::string winQuoteForCreateProcess(const std::string& s) {
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') out += "\\\"";
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_array.push_back(shard_obj);
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 730: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (filename format ensures correct sort order)
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: Result<void> BackupManager::isBackupComplete(const std::string& backup_dir,
  Confidence: band=high; score=0.74
- Line 989: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -czf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 994: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: mutable_cmd.push_back('\0');
- Line 1061: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -xzf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 1653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: full_backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 1884: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) coll_list += ", ";
  Confidence: band=high; score=0.74
- Line 1885: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) coll_list += ", ";
- Line 1913: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> target_cfs;
  Confidence: band=medium; score=0.66
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 2271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(kv.second.schedule_id, kv.second.cron_expression);
  Confidence: band=high; score=0.74
- Line 2551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 2552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.path().string());

### src/storage/blob_redundancy_manager.cpp
Total findings: 41

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    if (size < 1024 * 1024) {', '        return BlobType::BLOB_SMALL;', '    } else if (size < 100 * 1024 * 1024) {', '        return BlobType::BLOB_MEDIUM;', '    } else {']
  Confidence: band=very_high; score=0.93
- Line 1228: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4336 docs(storage): correct SECU... (2026-03-19) | #4201 feat(base): async r
- Line 767: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(healthy_dcs.cbegin(), healthy_dcs.cend(), loc.datacenter)
- Line 1052: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [blob_id, metadata] : blobs_) {
- Line 1066: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [blob_id, metadata] : blobs_) {
- Line 1137: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(tier_candidates.size(), max_tier_ops); ++i) {
- Line 1181: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(healthy_dcs.cbegin(), healthy_dcs.cend(),
- Line 1294: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // files that have been superseded by new ones, and in those cases the
- Line 1360: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(repair_mutex_);
- Line 1363: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: repair_cv_.wait_for(lock, std::chrono::seconds(5), [this]() {
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(loc.shard_id);
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(loc.shard_id);
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locs.push_back(location_to_json(loc));
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locs.push_back(location_to_json(loc));
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.locations.push_back(location_from_json(lj));
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: m.locations.push_back(location_from_json(lj));
- Line 744: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) s += ", ";
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) s += ", ";
- Line 764: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::string> healthy_dcs;
- Line 768: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 769: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_dcs.push_back(loc.datacenter);
- Line 843: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deleted_shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74
- Line 986: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deleted_shards.push_back(location.shard_id);
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1053: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: critical.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: stats.healthy_blobs++;
- Line 1164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1178: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::string> healthy_dcs;
- Line 1182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 1183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_dcs.push_back(loc.datacenter);
- Line 1241: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "# HELP themis_blob_redundancy_healthy_blobs Number of healthy blobs\n";
- Line 1242: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "# TYPE themis_blob_redundancy_healthy_blobs gauge\n";
- Line 1243: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "themis_blob_redundancy_healthy_blobs " << stats.healthy_blobs << "\n";
- Line 1299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74
- Line 1477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(location.shard_id);

### src/storage/simd_filter.cpp
Total findings: 40

- Line 55: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: int v = cached.load(std::memory_order_relaxed);
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint32_t>(i));
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74
- Line 690: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(idx + row_offset);

### src/storage/wom_tree.cpp
Total findings: 38

- Line 423: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        stat_flush_passes.fetch_add(1, std::memory_order_relaxed);', '', '        uint32_t next_depth = depth + 1;', '', '        // Apply ops to each original child.']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (child.is_leaf) {', '                // Apply ops directly to leaf.', '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            } else {', "                // Push ops into child's buffer.", '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::atomic<uint64_t> stat_puts{0};
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node_ref->is_leaf) return false;
- Line 416: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (size_t i = 0; i < node_ref->children.size(); ++i) {
- Line 417: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (doOneInternalSplit(node_ref->children[i], node_ref.get(), i)) {
- Line 423: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 509: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (!node->is_leaf) {
- Line 510: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t ci = node->childIndex(key);
- Line 511: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node = node->children[ci].get();
- Line 538: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (!node->is_leaf) {
- Line 540: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (auto it = node->buffer.rbegin(); it != node->buffer.rend(); ++it) {
- Line 546: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t ci = node->childIndex(key);
- Line 547: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node = node->children[ci].get();
- Line 551: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto leaf_it = node->leafFind(key);
- Line 564: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (leaf_it != node->data.end()) {
- Line 682: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.fetch_add(1, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.store(0, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: s.total_puts          = impl_->stat_puts.load(std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child_ops[idx].push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child.buffer.push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->children.push_back(std::move(root));
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->pivot_keys.push_back(std::move(pivot));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void collectAllEntries(std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74

### src/storage/tt_quantizer.cpp
Total findings: 35

- Line 44: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedCore> QuantizedCore::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedTrain> QuantizedTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 147: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto oc = QuantizedCore::deserialize(cb);
  Confidence: band=very_high; score=0.99
- Line 165: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Fix loop condition or increase array size
  Context: float best_dist = std::abs(v - kNF4Table[0]);
- Line 250: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int8_t qi = static_cast<int8_t>(qc.data[i]);
- Line 251: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data[i] = static_cast<float>(qi) * qc.scale;
- Line 264: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t byte_val = qc.data[i / 2];
- Line 267: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data[i] = nf4_val * qc.scale + qc.mean;
- Line 318: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint32_t u; std::memcpy(&u, &core.data[i], 4);
- Line 320: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: qc.data[i*4+j] = static_cast<uint8_t>((u >> (j*8)) & 0xFF);
- Line 355: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: u |= static_cast<uint32_t>(qc.data[i*4+j]) << (j*8);
- Line 356: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&core.data[i], &u, 4);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            qc.data[i / 2] = idx & 0x0F;', '        else', '            qc.data[i / 2] |= (idx << 4) & 0xF0;', '    }', '    return qc;']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    for (std::size_t i = 0; i < nelems; ++i) {', '        uint8_t byte_val = qc.data[i / 2];', '        uint8_t idx = (i % 2 == 0) ? (byte_val & 0x0F) : ((byte_val >> 4) & 0x0F);', '        float nf4_val = kNF4Table[idx];', '        core.data[i] = nf4_val * qc.scale + qc.mean;']
  Confidence: band=medium; score=0.62
- Line 29: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 4; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 33: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 4; ++i) out.push_back((u >> (i*8)) & 0xFF);
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(quant_type));
- Line 71: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((u >> (i*8)) & 0xFF);
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(quant_type));
- Line 153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74

### src/storage/tensor_train_decomposer.cpp
Total findings: 34

- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<TTTrain> TTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (pos + 8 > bytes.size()) throw std::runtime_error("TTTrain::deserialize: underflow");
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 796: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (float v : res.cores[0].data) norm_sq += static_cast<double>(v) * v;
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((u >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((u >> (i*8)) & 0xFF));
- Line 207: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/database_connection_manager.cpp
Total findings: 26

- Line 90: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 103: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 103: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Check active connections (just update health check time)', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        if (conn->isValid()) {', '            health.last_health_check = std::chrono::system_clock::now();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Active connections will be reconnected when released', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        health.state = ConnectionState::RECONNECTING;', '    }']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 41: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DatabaseConnectionManager::acquireConnection(
- Line 50: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::warn("Circuit breaker open - cannot acquire connection");
- Line 54: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 122: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 210: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 211: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& health = connection_health_[ptr];
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DatabaseConnectionManager::getConnectionHealth() const {
- Line 290: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 291: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& health = connection_health_[ptr];
- Line 309: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 78: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 169: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 187: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::queue<std::shared_ptr<Connection>> healthy_connections;
- Line 197: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health_list.push_back(health);
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 305: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 310: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();

### src/storage/erasure_coder_factory.cpp
Total findings: 26

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
  Confidence: band=very_high; score=0.9
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    uint32_t parity_shards', ') {', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    uint32_t parity_shards', ') {', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
  Confidence: band=high; score=0.81
- Line 430: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: parity_byte ^= gf_mul(cauchy_matrix[parity_row][data_row], chunks[data_row][byte]);
- Line 36: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 228: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 243: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 247: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
  Confidence: band=medium; score=0.56
- Line 271: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 287: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 348: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
  Confidence: band=medium; score=0.56

### src/storage/distributed_transaction_manager.cpp
Total findings: 23

- Line 304: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference that outlives any concurrent unregisterShard() call.
  Confidence: band=very_high; score=0.99
- Line 75: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: unknown shard '" + shard_id + "'"
- Line 87: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: put() called on non-ACTIVE transaction"
- Line 107: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: del() called on non-ACTIVE transaction"
- Line 125: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<std::string> DistributedTransaction::get(std::string_view key) {
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return participant->get(logical_key);
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool DistributedTransaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransaction [{}]: commit() called in unexpected state", txn_id_);
- Line 147: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 1 — PREPARE to {} shard(s)",
- Line 153: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, ops] : pending_ops_) {
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(mgr_state_->shards_mutex);
- Line 160: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' no longer registered during prepare",
- Line 170: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: voted_commit = participant->prepare(txn_id_, ops);
- Line 172: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' prepare threw: {}",
- Line 189: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 2 — COMMIT to {} shard(s)",
- Line 200: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' commit threw: {}",
- Line 206: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: COMMITTED across {} shard(s)",
- Line 218: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort threw: {}",
- Line 224: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: ABORTED (prepare phase failed)", txn_id_);
- Line 245: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
- Line 250: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: rolled back", txn_id_);
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(shard_id);
  Confidence: band=high; score=0.74

### src/storage/storage_audit_logger.cpp
Total findings: 23

- Line 52: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode);
- Line 56: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 111: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: StorageAuditLogger::open(const Config& config) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 143: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 45: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 53: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: found.push_back(std::stoull(m[1].str()));
  Confidence: band=high; score=0.74
- Line 173: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: Result<void> StorageAuditLogger::log(Event event,
  Confidence: band=medium; score=0.6
- Line 182: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::PUT, key, extra);
  Confidence: band=medium; score=0.6
- Line 187: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::DEL, key, extra);
  Confidence: band=medium; score=0.6
- Line 191: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::CHECKPOINT, "", detail);
  Confidence: band=medium; score=0.6
- Line 195: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::RECOVERY, "", detail);
  Confidence: band=medium; score=0.6
- Line 199: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::COMPACTION, "", detail);
  Confidence: band=medium; score=0.6
- Line 203: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::SNAPSHOT, "", detail);
  Confidence: band=medium; score=0.6

### src/storage/blob_backend_s3.cpp
Total findings: 21

- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
- Line 120: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4227 feat(ingestion): S3-Compati... (2026-03-14) | #746 [Phase 4] Storage La
- Line 113: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 114: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 115: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256);
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBody(input_stream);
- Line 121: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetContentLength(data.size());
- Line 156: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 157: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 184: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[4096];
- Line 214: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 215: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 226: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "S3 delete failed: " + error.GetMessage()
- Line 241: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 242: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);

### src/storage/index_maintenance.cpp
Total findings: 21

- Line 421: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 422: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(policy_.time_based_interval_ms),
- Line 580: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 629: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 671: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 704: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 104: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 127: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus job_status;
- Line 177: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 200: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus job_status;
- Line 250: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 277: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 304: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 348: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<MaintenanceJobStatus> jobs;
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jobs.push_back(status);
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { metrics.file_count = std::stoull(file_count_str); } catch (...) {}
- Line 600: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.after_metrics.fragmentation_percentage);
- Line 649: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.after_metrics.fragmentation_percentage);
- Line 842: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 858: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.job_id, status.index_name);
- Line 876: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: errors::ErrorCode::ERR_INDEX_REBUILD_FAILED, reindex_status.message);

### src/storage/wal_storage.cpp
Total findings: 20

- Line 55: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode);
- Line 59: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 117: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: buf[0] = static_cast<uint8_t>(v);
- Line 118: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Fix loop condition or increase array size
  Context: buf[1] = static_cast<uint8_t>(v >> 8);
- Line 119: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Fix loop condition or increase array size
  Context: buf[2] = static_cast<uint8_t>(v >> 16);
- Line 120: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: buf[3] = static_cast<uint8_t>(v >> 24);
- Line 130: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: return static_cast<uint32_t>(buf[0])
- Line 131: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[1]) << 8)
- Line 132: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[2]) << 16)
- Line 133: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[3]) << 24);
- Line 177: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<std::unique_ptr<WALStorage>> WALStorage::open(
- Line 324: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~stat() { /* cleanup */ }
  Context: class/struct stat
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4596 perf(storage): fix ~79x sus... (2026-04-13) | #4236 feat(storage): Zero
- Line 204: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 48: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 56: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(sid);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(1);
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(sid);
  Confidence: band=high; score=0.74

### src/storage/history_manager.cpp
Total findings: 17

- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HistoryRecord> HistoryManager::deserializeHistoryRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeHistoryRecord(it.value());
  Confidence: band=very_high; score=0.99
- Line 214: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeHistoryRecord(val);
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictRecord> ConflictManager::deserializeConflictRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 313: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictRecord(
  Confidence: band=very_high; score=0.99
- Line 321: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeConflictRecord(val);
  Confidence: band=very_high; score=0.99
- Line 342: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictSet> ConflictManager::deserializeConflictSet(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 377: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictSet(
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto set = deserializeConflictSet(val);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2766 [storage/transaction] Atomi... (2026-03-11)
- Line 40: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hi = hex[i];
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lo = hex[i + 1];
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 353: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/hlc.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 49: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = state_.load(std::memory_order_relaxed);
- Line 126: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return HLCTimestamp{state_.load(std::memory_order_acquire)};
- Line 78: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Context: HLCTimestamp HybridLogicalClock::now() {
  Confidence: band=medium; score=0.56

### src/storage/online_schema_migration.cpp
Total findings: 16

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 148: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult SchemaMigrator::migrate()
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(tables.begin(), tables.end(), op.table_name) == tables.end()) {
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables.push_back(op.table_name);
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tables.push_back(op.table_name);
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(msg);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(prop));
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(meta));
  Confidence: band=high; score=0.74

### src/storage/concurrent_write_controller.cpp
Total findings: 15

- Line 113: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 121: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 78: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: , acquire_timeout_(config.acquire_timeout) {
- Line 113: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 121: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 129: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: total_acquired_.fetch_add(1, std::memory_order_relaxed);
- Line 147: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (acquire_timeout_.count() > 0) {
- Line 148: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: got_slot = (f.wait_for(acquire_timeout_) == std::future_status::ready);
- Line 176: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "ConcurrentWriteController: acquire() timed out");
- Line 272: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.total_acquired = total_acquired_.load(std::memory_order_relaxed);
- Line 55: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WriteGuard::release() noexcept {
- Line 104: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 223: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { next.set_value(); } catch (...) {}

### src/storage/base_entity.cpp
Total findings: 14

- Line 104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return field_cache_->find(std::string(field_name)) != field_cache_->end();
- Line 109: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = field_cache_->find(std::string(field_name));
- Line 110: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != field_cache_->end()) {
- Line 253: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto last = token.find_last_not_of(" \t");
- Line 629: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity BaseEntity::deserialize(std::string_view pk, const Blob& blob) {
  Confidence: band=very_high; score=0.99
- Line 510: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown type tag encountered while parsing BaseEntity binary blob");
- Line 518: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Binary parse failed");
- Line 165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(el.get<std::string>());
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(static_cast<float>(elem.get<double>()));

### src/storage/gguf_metadata.cpp
Total findings: 14

- Line 76: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.99
- Line 345: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool GGUFMetadata::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::string toHex(const unsigned char* data, size_t len) {
- Line 66: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: oss << std::setw(2) << static_cast<unsigned int>(data[i]);
- Line 71: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::string computeHmacSha256(const std::string& data,
- Line 76: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.9
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>(v >>  0));
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >>  0));
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >>  8));
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 16));
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 24));
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(k);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 304: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/nlp_metadata_extractor.cpp
Total findings: 14

- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.keywords.push_back(keywords[i].text);
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.emails.push_back(entity.text);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.emails.push_back(entity.text);
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.urls.push_back(entity.text);
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.dates.push_back(entity.text);
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.measurements.push_back(entity.text);
- Line 186: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(kw.text);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keywords.push_back(kw.text);
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[entity.type].push_back(entity.text);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result[entity.type].push_back(entity.text);
- Line 363: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/tensor_compaction_filter.cpp
Total findings: 14

- Line 108: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize raw TTTrain
  Confidence: band=very_high; score=0.99
- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = TTTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99
- Line 139: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize QuantizedTrain header
  Confidence: band=very_high; score=0.99
- Line 144: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = QuantizedTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 173: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/merge_operators.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/storage/security_signature_manager.cpp
Total findings: 12

- Line 51: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 86: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signatures.push_back(*sig);
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: signatures.push_back(*sig);
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: signatures.push_back(*sig);
- Line 139: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
- Line 144: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 167: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.failed_resource_ids.push_back(sig->resource_id);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.failed_resource_ids.push_back(sig->resource_id);

### src/storage/tensor_router.cpp
Total findings: 12

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Force-LIFT for inference-bound data when policy says so
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (policy.force_lift_for_inference && hint.inference_use) {
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto pilot    = impl_->runPilot(data, mode_sizes);
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"force_lift_for_inference",     impl_->policy.force_lift_for_inference},
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_use", hint.inference_use},
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->template_topology_apply_fn = nullptr;
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 189: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_n = std::log(static_cast<double>(n_pilot));
  Confidence: band=medium; score=0.6
- Line 190: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_r = std::log(static_cast<double>(res.pilot_rank));
  Confidence: band=medium; score=0.6
- Line 449: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void TensorRouter::setTemplateCatalog(
  Confidence: band=medium; score=0.6
- Line 471: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: TensorRouter::templateCatalog() const noexcept {
  Confidence: band=medium; score=0.6

### src/storage/zero_copy_blob_transfer.cpp
Total findings: 11

- Line 72: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 118: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: fd_ = ::open(file_path.c_str(), O_RDONLY);
- Line 124: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~stat() { /* cleanup */ }
  Context: class/struct stat
- Line 290: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int src_fd = ::open(source_path.c_str(), O_RDONLY);
- Line 482: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: part_stream->write(part_buf.data(), this_part);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    int64_t file_size = static_cast<int64_t>(fs::file_size(source_path));', '    if (length == 0) {', '        length = file_size - offset;', '    }', '    if (length <= 0 || offset < 0 || offset >= file_size) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 126: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 136: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 307: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(src_fd);
- Line 320: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(src_fd);

### src/storage/compressed_storage.cpp
Total findings: 10

- Line 39: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<CompressedValue> CompressedValue::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 47: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: result.method = static_cast<compression::CompressionMethod>(bytes[0]);
- Line 102: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "Column: " + pair.first + "\n";
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "Column: " + pair.first + "\n";
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "\n";

### src/storage/compression_strategy.cpp
Total findings: 10

- Line 594: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = value_to_index.find(value);
- Line 183: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {
- Line 593: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = value_to_index.find(value);
- Line 472: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<uint8_t>(value | 0x80));
- Line 475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<uint8_t>(value));
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(value);
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dictionary.push_back(value);
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(it->second);
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(dictionary[idx]);

### src/storage/erasure_coding_backend.cpp
Total findings: 10

- Line 165: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto recovered = coder_->decode(chunk_map, missing, k, m);
- Line 159: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (chunk_map.find(i) == chunk_map.end()) {
- Line 159: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (chunk_map.find(i) == chunk_map.end()) {
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, EncodedShard>& shards,
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::vector<uint8_t>> chunk_map;
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, EncodedShard> shard_map;
  Confidence: band=high; score=0.74

### include/storage/examples/schema_layout_advisor_example.cpp
Total findings: 9

- Line 176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [PLANNED — StorageLayoutAdvisor::adviseLayout() — IMPL-B10]\n"
- Line 89: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  " << f.field_name
- Line 104: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  " << candidate.field_name
- Line 131: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "    " << f.field_name << " → " << rec << "\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Write rate: " << ts_profile.rows_per_day_write_rate << " rows/day\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Read/write: " << ts_profile.read_write_ratio * 100.0 << " % reads\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Read/write: " << ts_profile.read_write_ratio * 100.0 << " % reads\n"
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Expected RETAIN:         3  (order_status, customer_email/GDPR, seasonal)\n"
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/ for implementation specs.\n";

### src/storage/adaptive_compaction.cpp
Total findings: 9

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 87: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(sample_mutex_);
- Line 102: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t reads  = window_reads_.exchange(0, std::memory_order_relaxed);
- Line 103: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t writes = window_writes_.exchange(0, std::memory_order_relaxed);
- Line 157: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: ? std::log(current_write_amp) / std::log(config_.urgent_write_amp_threshold)
  Confidence: band=medium; score=0.6

### src/storage/blob_backend_azure.cpp
Total findings: 9

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)
- Line 79: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: service_client.GetBlobContainerClient(container_name_)
- Line 114: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = blob_client.Upload(stream, options);
- Line 147: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = blob_client.Download();
- Line 201: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: blob_client.Delete();
- Line 207: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: THEMIS_ERROR("Azure delete failed: {}", e.what());
- Line 210: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "Azure delete failed: " + std::string(e.what())
- Line 225: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto properties = blob_client.GetProperties();

### src/storage/columnar_cache.cpp
Total findings: 8

- Line 105: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(key);
- Line 138: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(key);
- Line 31: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: case SegmentDType::Int64:  return n * sizeof(int64_t) + n;   // data + null bitmap
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 74: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PinGuard::release() noexcept {
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cfg_.on_evict) to_notify.push_back(k);
  Confidence: band=high; score=0.74

### src/storage/index_analyzer.cpp
Total findings: 8

- Line 243: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : snapshot) {
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 346: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::minutes(1),
- Line 356: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(60),
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.indices.push_back(std::move(ie));
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reports.push_back(std::move(report));
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { l0_files = std::stoull(l0_str); } catch (...) {}
- Line 485: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/pitr_manager.cpp
Total findings: 8

- Line 103: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 163: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(options.tables.begin(), options.tables.end(), table) != options.tables.end()) {
- Line 283: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(options.tables.begin(), options.tables.end(), table) == options.tables.end()) {
- Line 333: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: return Status::Error("Failed to delete key: " + event.key);
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preview.affected_keys.push_back(event.key);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preview.affected_keys.push_back(event.key);
- Line 309: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: event.sequence, status.message);

### src/storage/storage_engine.cpp
Total findings: 8

- Line 262: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> StorageEngine::open(const std::string& db_path) {
- Line 275: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!rocksdb_->open()) {
- Line 24: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // They are provided for testing, development, and backward compatibility only.
  Confidence: band=high; score=0.8
- Line 301: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 302: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: while (v < cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 306: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 307: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: while (v > cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 399: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: "Failed to delete key: " + key);

### src/storage/tensor_network_storage_engine.cpp
Total findings: 8

- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return (it != version_cache_.end()) ? it->second : 0;
- Line 193: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return QuantizedTrain::deserialize(*meta);
  Confidence: band=very_high; score=0.99
- Line 226: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
- Line 311: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.first);
- Line 351: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* ignore parse errors */ }
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: logical_keys.push_back(raw_key.substr(raw_prefix.size()));
  Confidence: band=high; score=0.74

### src/storage/mvcc_store.cpp
Total findings: 7

- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: seek_key.push_back('\x01');
- Line 266: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Keys are already in ascending timestamp order (big-endian sort).
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: uint64_t num_to_delete = 0;
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: num_to_delete = std::min(num_to_delete, max_deletable);
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(bk);
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(vkey.data(), vkey.size() - 9);
  Confidence: band=high; score=0.74

### src/storage/tiered_storage.cpp
Total findings: 7

- Line 295: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: source = nullptr;
  Context: THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
- Line 379: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lock(worker_mutex_);
- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
- Line 97: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
- Line 98: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||

### src/storage/streaming_ingest_manager.cpp
Total findings: 6

- Line 45: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new StreamingIngestManager(std::move(db), std::move(cfg)));
- Line 259: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 269: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 201: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 202: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mu_);
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buffer_.push_back({std::string(key), std::string(value)});

### src/storage/ggml_tensor_bridge.cpp
Total findings: 5

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 221: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: raw = storage->getVersion(key, static_cast<std::size_t>(version));
- Line 142: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Pretend to be a ggml_tensor for pointer compatibility in tests.
  Confidence: band=high; score=0.8
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // not for llama.cpp inference until a real allocator is injected).
  Confidence: band=very_high; score=0.9

### src/storage/compaction_manager.cpp
Total findings: 4

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 123: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(bg_mutex_);
- Line 183: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // writes are memtable flush outputs; L1+ writes are compaction outputs.
  Confidence: band=very_high; score=0.9
- Line 218: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/storage/disk_space_monitor.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 617: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: size_t pos = path.find_last_of("/\\");

### src/storage/hamming_coder.cpp
Total findings: 4

- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t shard_size = static_cast<uint32_t>(available_chunks.begin()->second.size());
- Line 89: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = available_chunks.find(s);
- Line 88: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = available_chunks.find(s);
- Line 74: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74

### src/storage/storage_parquet_exporter.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    size_t n = seg.metadata().row_count;', '    // rawData() invariant: raw.size() == n * element_size (1 for BOOL).', '    size_t packed_bytes = (n + 7) / 8;', '    std::vector<uint8_t> values(packed_bytes, 0);', '    for (size_t i = 0; i < n; ++i) {']
  Confidence: band=high; score=0.78
- Line 267: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: raw.data() + n * sizeof(int64_t));
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));

### src/storage/transaction_retry_manager.cpp
Total findings: 4

- Line 178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BackoffStrategy strategy = policy ? policy->backoff_strategy : config_.backoff_strategy;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 310: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/encrypted_blob_backend.cpp
Total findings: 3

- Line 209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 274: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 284: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/security_signature.cpp
Total findings: 3

- Line 58: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<SecuritySignature> SecuritySignature::deserialize(const std::string& data) {
  Confidence: band=very_high; score=0.99
- Line 49: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 62: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/blob_backend_filesystem.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)
- Line 152: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/blob_backend_webdav.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65

### src/storage/raft_mvcc_bridge.cpp
Total findings: 2

- Line 106: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: HLCTimestamp RaftMvccBridge::raftAwareWrite(
  Confidence: band=very_high; score=0.99
- Line 98: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RaftMvccBridge::snapshotRead(std::string_view key, HLCTimestamp ts) {
  Confidence: band=very_high; score=0.9

### src/storage/blob_backend_gcs.cpp
Total findings: 1

- Line 217: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "GCS delete failed: " + status.message());

### src/storage/key_schema.cpp
Total findings: 1

- Line 123: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Assume DOCUMENT for backward compatibility (was more common in early versions)
  Confidence: band=high; score=0.8

### src/storage/mvcc_chain_pruner.cpp
Total findings: 1

- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_versions.push_back({e.timestamp, e.value});

### src/storage/schema_dead_weight_detector.cpp
Total findings: 1

- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signal.push_back(static_cast<double>(count));
  Confidence: band=high; score=0.74

### src/storage/storage_layout_advisor.cpp
Total findings: 1

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(ts[i] - ts[i - 1]);
  Confidence: band=high; score=0.74

### src/storage/vector_index_backend.cpp
Total findings: 1

- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({id, dist, toScore(dist)});
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
