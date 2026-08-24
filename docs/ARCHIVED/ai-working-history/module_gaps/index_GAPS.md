# index Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: index
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 719
- Actionable Findings (Critical + High): 443
- Affected Files: 38

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 99 |
| High | 344 |
| Medium | 259 |
| Low | 17 |

## Category Summary

| Category | Count |
|---|---:|
| unordered_container_iter | 85 |
| resource_leaked_in_exception | 77 |
| data_race | 52 |
| string_concat_loop | 37 |
| db_connection_leak | 35 |
| o_n_squared | 27 |
| generic_catch | 26 |
| uncaught_exception | 26 |
| unchecked_cuda_call | 26 |
| legacy_or_compat_path | 25 |
| copy_overhead | 20 |
| explicit_delete | 20 |
| unnecessary_copy | 19 |
| delete_without_nullptr | 17 |
| fp_exact_comparison | 17 |
| iterator_invalidation | 17 |
| manual_cleanup | 14 |
| map_vs_unordered_map | 14 |
| size_assumption | 13 |
| uninitialized_access | 11 |
| delete_no_nullptr | 10 |
| hardcoded_output | 10 |
| range_temporary | 10 |
| nested_loop_find | 9 |
| model_integrity_gap | 8 |
| undefined_conflict_resolution | 8 |
| missing_latency_metric | 6 |
| unchecked_array_index | 6 |
| arithmetic_overflow | 5 |
| gpu_memory_leak | 5 |
| pointer_arithmetic_unbounded | 5 |
| user_controlled_size | 5 |
| allocation_loop | 4 |
| lock_contention | 4 |
| missing_consensus | 4 |
| unstructured_log | 4 |
| missing_version_tracking | 3 |
| shared_state_no_sync | 3 |
| stale_doc_section_reference | 3 |
| uninitialized_member_field | 3 |
| blocking_no_timeout | 2 |
| deadlock_risk | 2 |
| exception_in_destructor | 2 |
| module_doc_linkset_drift | 2 |
| no_timeout | 2 |
| primitive_no_volatile | 2 |
| repeated_search | 2 |
| thread_join_no_timeout | 2 |
| duplicate_qualified_signature | 1 |
| expensive_copy | 1 |
| hardcoded_path | 1 |
| manual_cleanup_in_destructor | 1 |
| memory_order | 1 |
| missing_adr_reference | 1 |
| null_dereference | 1 |
| smart_ptr_misuse | 1 |
| timestamp_sorting_unstable | 1 |
| use_after_free_gpu | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| index/secondary_index.cpp | 170 | 14 | 63 | 89 | 4 |
| index/vector_index.cpp | 83 | 18 | 27 | 38 | 0 |
| index/graph_index.cpp | 67 | 8 | 42 | 17 | 0 |
| index/spatial_index.cpp | 45 | 0 | 28 | 13 | 4 |
| index/cuda_hnsw_graph_traversal.cpp | 44 | 10 | 34 | 0 | 0 |
| index/gpu_vector_index.cpp | 36 | 11 | 20 | 5 | 0 |
| index/graph_analytics.cpp | 33 | 2 | 15 | 16 | 0 |
| index/distributed_vector_index.cpp | 29 | 7 | 17 | 5 | 0 |
| index/inverted_index.cpp | 25 | 0 | 8 | 16 | 1 |
| index/advanced_vector_index.cpp | 24 | 3 | 13 | 8 | 0 |
| index/process_graph.cpp | 23 | 1 | 3 | 19 | 0 |
| index/gnn_embeddings.cpp | 15 | 9 | 3 | 3 | 0 |
| index/multi_vector_search.cpp | 13 | 3 | 4 | 5 | 1 |
| index/product_quantizer.cpp | 11 | 0 | 11 | 0 | 0 |
| index/gpu_vector_index_vulkan.cpp | 9 | 4 | 4 | 1 | 0 |
| index/index_compression.cpp | 9 | 0 | 6 | 2 | 1 |
| index/multi_gpu_vector_index.cpp | 9 | 0 | 3 | 6 | 0 |
| index/property_graph.cpp | 8 | 0 | 4 | 4 | 0 |
| index/vector_auto_buffer.cpp | 7 | 4 | 3 | 0 | 0 |
| index/gpu_memory_oversubscription.cpp | 6 | 2 | 4 | 0 | 0 |
| index/graph_auto_buffer.cpp | 6 | 2 | 4 | 0 | 0 |
| index/hnsw_parameter_tuner.cpp | 6 | 0 | 5 | 1 | 0 |
| index/learnable_rope.cpp | 6 | 0 | 3 | 3 | 0 |
| index/ann_index.cpp | 5 | 0 | 5 | 0 | 0 |
| index/rotary_embeddings_hip.cpp | 5 | 0 | 3 | 2 | 0 |
| index/residual_quantizer.cpp | 4 | 0 | 0 | 4 | 0 |
| index/adaptive_index.cpp | 3 | 0 | 3 | 0 | 0 |
| index/rotary_embeddings.cpp | 3 | 0 | 3 | 0 | 0 |
| index/approximate_radius_search.cpp | 2 | 0 | 0 | 0 | 2 |
| index/hnsw_production_defaults.cpp | 2 | 0 | 1 | 0 | 1 |
| index/index_manager.cpp | 2 | 0 | 1 | 1 | 0 |
| index/learned_quantizer.cpp | 2 | 0 | 2 | 0 | 0 |
| index/workload_replay.cpp | 2 | 0 | 1 | 1 | 0 |
| index/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| index/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| index/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| index/edge_types.cpp | 1 | 1 | 0 | 0 | 0 |
| index/temporal_graph.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### index/secondary_index.cpp
Total findings: 170

- Line 1713: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 1717: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 1721: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 1725: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 1729: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 1733: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 1737: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 4464: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 4468: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 4472: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 4476: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 4480: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 4484: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 4488: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 139: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['// static', 'std::string SecondaryIndexManager::makeCompositeIndexMetaKey(std::string_view table, const std::vector<std::string>& columns) {', '\tsize_t total = 8 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
- Line 168: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1 + pk.size();', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
- Line 202: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
- Line 254: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['\tstd::vector<std::string> encodedVals;', '\tencodedVals.reserve(values.size());', '\tsize_t total = 5 + table.size() + 1; // "uidx:" + table + ":"', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\tif (i > 0) total += 1; // "+"']
- Line 311: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compatibility API: createIndex with IndexType enum
- Line 769: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy format (just "fulltext" marker) - return default config
- Line 984: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case 0: return numericOk ? (fvNum == rhsNum) : (fv == rhs);
- Line 985: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case 1: return numericOk ? (fvNum != rhsNum) : (fv != rhs);
- Line 1301: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
- Line 1313: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
- Line 1322: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t pos = col.find('+', start);
- Line 1344: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (col.find('+') == std::string::npos) {
- Line 1347: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1353: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = regularUniqueCache.find(col);
- Line 1422: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = compositeUniqueCache.find(col);
- Line 1464: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1480: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1531: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1564: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1590: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1633: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = partialPredicatesCache.find(col);
- Line 1644: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1645: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1820: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t pos = col.find('+', start);
- Line 1906: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!maybeValue) continue;

			

			// We need to find the TTL index entry, but we don't know the exact timestamp

			// Scan the TTL index prefix and delete matching PKs

			std::string prefix = makeTTLIndexPrefix(table, tcol);

			db_.scanPrefix(prefix, [&pk, &batch](std::string_view key, std::string_view /*val*/) {

				// Extract PK from ttlidx:table:column:timestamp:PK
- Line 1906: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Scan the TTL index prefix and delete matching PKs
- Line 1930: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Use cached fulltext config to avoid db.get + JSON parse on every upsert/delete (v1.3.5)
- Line 1968: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto maybe = oldEntityOpt->extractField(pcol);

			if (!maybe) continue;

			// Only the entry was added if the predicate matched at insert time.

			// We attempt to delete it regardless (idempotent).

			const std::string encodedVal = encodeKeyComponent(*maybe);

			const std::string pidxKey = makePartialIndexKey(table, pcol, encodedVal, pk);

			batch.del(pidxKey);
- Line 1968: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // We attempt to delete it regardless (idempotent).
- Line 2342: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
- Line 2654: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2655: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2713: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto itLen = docLen.find(pk);
- Line 2714: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto itLen = docLen.find(pk);
- Line 2735: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

			double denom = tf + k1 * (1.0 - b + b * (dl / avgdl));

			if (denom <= 0.0) denom = tf + k1; // Guard

			double term = idf * ((tf * (k1 + 1.0)) / denom);

			s += term;

		}

		scored.emplace_back(FulltextResult{pk, s});
- Line 2759: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Public API: returns PKs only (deprecated, use scanFulltextWithScores for scores)
- Line 3477: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3499: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::microseconds(throttle_us));
- Line 3540: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& token : tokenize(*maybeVal, config))
- Line 3636: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



	// Step 6: Atomic swap – in one WriteBatch:

	//   a) delete all stale live entries

	//   b) promote each shadow entry to the live prefix

	//   c) delete each shadow entry

	// During this brief batch write the live index briefly transitions; concurrent
- Line 3636: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: //   a) delete all stale live entries
- Line 3638: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Step 6: Atomic swap – in one WriteBatch:

	//   a) delete all stale live entries

	//   b) promote each shadow entry to the live prefix

	//   c) delete each shadow entry

	// During this brief batch write the live index briefly transitions; concurrent

	// readers may observe either old or new entries, but never an empty index.

	auto batch = db_.createWriteBatch();
- Line 3638: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: //   c) delete each shadow entry
- Line 3640: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4041: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
- Line 4053: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
- Line 4062: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t pos = col.find('+', start);
- Line 4083: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (col.find('+') == std::string::npos) {
- Line 4086: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4092: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = regularUniqueCache.find(col);
- Line 4169: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = compositeUniqueCache.find(col);
- Line 4222: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4236: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4285: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4312: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4341: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4383: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = partialPredicatesCache.find(col);
- Line 4393: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4394: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 4571: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t pos = col.find('+', start);
- Line 4657: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!maybeValue) continue;

			

			// We need to find the TTL index entry, but we don't know the exact timestamp

			// Scan the TTL index prefix and delete matching PKs

			std::string prefix = makeTTLIndexPrefix(table, tcol);

			db_.scanPrefix(prefix, [&pk, &txn](std::string_view key, std::string_view /*val*/) {

				// Extract PK from ttlidx:table:column:timestamp:PK
- Line 4657: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Scan the TTL index prefix and delete matching PKs
- Line 4681: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Use cached fulltext config to avoid db.get + JSON parse on every upsert/delete (v1.3.5)
- Line 147: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += "idxmeta:";
- Line 151: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += "+";
- Line 152: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += "+";
- Line 180: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += "idx:";
- Line 184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += "+";
- Line 185: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += "+";
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += ":";
- Line 191: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key += ":";
- Line 214: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += "idx:";
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += "+";
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += "+";
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += ":";
- Line 225: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key += ":";
- Line 255: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) total += 1; // "+"
- Line 260: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: total += 1 + encodedVals.back().size(); // ":" + encoded
- Line 269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += "+";
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += "+";
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += ":";
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key += ":";
- Line 410: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // timestamp wird mit führenden Nullen auf 20 Zeichen padded für lexikografische Sortierung
- Line 510: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) colList += ", ";
- Line 511: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) colList += ", ";
- Line 547: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) colList += ", ";
- Line 548: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) colList += ", ";
- Line 1115: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_ptr<BaseEntity> oldEntity;

	if (oldBlob) {

		try { oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob)); }

		catch (...) { THEMIS_WARN("put(tx): alte Entity für PK={} nicht deserialisierbar", pk); }

	}



	// Serialize entity once and reuse for both entity write and geo hook
- Line 1115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { THEMIS_WARN("put(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
- Line 1191: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

					oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob));

				}

				catch (...) {

					THEMIS_WARN("putBatch: alte Entity für PK={} nicht deserialisierbar", pk);

				}

			}
- Line 1191: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 1446: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) valueStr += ", ";
- Line 1447: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) valueStr += ", ";
- Line 1610: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, uint32_t> tf;
- Line 1631: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> partialCols;
- Line 1689: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> indexedColsCache;
- Line 1690: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> rangeColsCache;
- Line 1691: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> sparseColsCache;
- Line 1692: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> geoColsCache;
- Line 1693: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> ttlColsCache;
- Line 1694: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> fulltextColsCache;
- Line 1695: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> partialColsCache;
- Line 1696: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache;
- Line 1729: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 1733: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 1737: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 1947: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
- Line 2073: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) colList += ", ";
- Line 2074: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) colList += ", ";
- Line 2161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: startKey += '\xFF'; // Skip to next value
- Line 2566: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
- Line 2610: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersectionSet = tokenResults.front();
- Line 2612: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection;
- Line 2655: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2673: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> universe;
- Line 2685: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> docLen;
- Line 2694: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: double dl = 0.0;

		if (v && !v->empty()) {

			std::string s(reinterpret_cast<const char*>(v->data()), v->size());

			try { dl = static_cast<double>(std::stoull(s)); } catch (...) { dl = 0.0; }

		}

		docLen.emplace(pk, dl);

		totalLen += dl;
- Line 2694: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) { dl = 0.0; }
- Line 2817: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
- Line 2839: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> candidates = tokenResults[0];
- Line 2841: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection;
- Line 2887: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: results.emplace_back(FulltextResult{pk, 1.0});

					}

				}

			} catch (...) {

				// Skip documents that fail to deserialize

			}

		}
- Line 2887: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2956: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> tokenToDocs;
- Line 2957: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> pkScores;
- Line 3704: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: totalLen += (cols.size() - 1) * 2; // ", " separators
- Line 3707: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) colList += ", ";
- Line 3708: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) colList += ", ";
- Line 3908: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try { 

			oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob)); 

		}

		catch (...) { 

			THEMIS_WARN("put(mvcc): alte Entity für PK={} nicht deserialisierbar", pk); 

		}

	}
- Line 3908: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) {
- Line 4181: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) valueStr += ", ";
- Line 4182: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) valueStr += ", ";
- Line 4204: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) valueStr += ", ";
- Line 4205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) valueStr += ", ";
- Line 4229: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> sparseCols;
- Line 4273: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> geoCols;
- Line 4305: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> ttlCols;
- Line 4334: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> fulltextCols;
- Line 4361: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, uint32_t> tf;
- Line 4381: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> partialCols;
- Line 4441: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> indexedColsCache;
- Line 4442: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> rangeColsCache;
- Line 4443: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> sparseColsCache;
- Line 4444: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> geoColsCache;
- Line 4445: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> ttlColsCache;
- Line 4446: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> fulltextColsCache;
- Line 4447: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> partialColsCache;
- Line 4448: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache;
- Line 4480: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 4484: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 4488: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 4698: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
- Line 412: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%020lld", (long long)expireTimestamp);
- Line 2342: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
- Line 2483: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(maxBuf, sizeof(maxBuf), "%020lld", (long long)currentTimestamp);
- Line 2721: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);

### index/vector_index.cpp
Total findings: 83

- Line 654: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("VectorIndexManager::init - Failed to load index: {}, creating new index", loadStatus.me
- Line 899: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator id_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto id_it = pkToId_.find(pk);
- Line 912: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: } else if (cache_it->second != new_vec) {
- Line 914: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cache_it->second = new_vec;
- Line 1078: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 1091: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 1098: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1100: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1160: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 1173: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 1180: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1182: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1309: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (by && by->size() == static_cast<size_t>(dim_)) {
- Line 1350: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (vecOpt->size() == static_cast<size_t>(dim_)) v = *vecOpt;
- Line 1357: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (qv && qv->size() == static_cast<size_t>(dim_)) {
- Line 1395: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& vec = cache_ptrs[i]->second;
- Line 1599: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto raw = ann_backend_->search(q.data(), static_cast<size_t>(dim_), static_cast<int>(k));
- Line 2449: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 153: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Try to load configuration from scaling_optimizations.yaml (new path first, then legacy)
- Line 153: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 268: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
- Line 268: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Release the HNSW index to avoid memory leaks

#ifdef THEMIS_HNSW_ENABLED

	if (hnswIndex_) {

		delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);

		hnswIndex_ = nullptr;

		useHnsw_ = false;

	}
- Line 268: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
- Line 273: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 273: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: useHnsw_ = false;

	}

	if (hnswSpace_) {

		delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);

		hnswSpace_ = nullptr;

	}

#endif
- Line 273: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 867: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (storage_vectors.find(pk) == storage_vectors.end())
- Line 875: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pkToId_.find(pk);
- Line 884: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 890: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1208: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
- Line 1208: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // RocksDB löschen

	std::string key = makeObjectKey(pk);

	if (!db_.del(key)) {

		THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);

	}



	// In-Memory Cache löschen
- Line 1208: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
- Line 1556: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (wl.find(pk) != wl.end()) {
- Line 2267: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
- Line 2267: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Release any previously allocated HNSW index before loading a new one

			// to prevent memory leaks when loadIndex() is called multiple times.

			if (hnswIndex_) {

				delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);

				hnswIndex_ = nullptr;

				useHnsw_ = false;

			}
- Line 2267: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
- Line 2272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 2272: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: useHnsw_ = false;

			}

			if (hnswSpace_) {

				delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);

				hnswSpace_ = nullptr;

			}
- Line 2272: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 2335: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Original plaintext load (backward compatibility)
- Line 2669: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t j = i + 1; j < std::min(i + 10, pks.size()); ++j) {
- Line 2919: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2986: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3031: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 170: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto hnsw_opt = config["hnsw_optimization"];
- Line 175: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto lp = hnsw_opt["layer_pruning"];
- Line 182: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto als = hnsw_opt["adaptive_layer_selection"];
- Line 189: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto bi = hnsw_opt["batch_insert"];
- Line 273: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 675: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: hnswSpace_ = static_cast<void*>(space.release());
- Line 677: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: hnswIndex_ = static_cast<void*>(appr.release());
- Line 878: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);

			auto it = pkToId_.find(pk);

			if (it != pkToId_.end()) {

				try { appr->markDelete(it->second); } catch (...) {}

			}

		}

#endif
- Line 878: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 908: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: id = id_it->second; // reuse label of a previously deleted entry

					idToPk_[id] = pk;   // update reverse mapping to the new PK

				}

				try { appr->addPoint(new_vec.data(), id); } catch (...) {}

			}

#endif

			++stats.added;
- Line 908: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { appr->addPoint(new_vec.data(), id); } catch (...) {}
- Line 920: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);

				auto id_it = pkToId_.find(pk);

				if (id_it != pkToId_.end()) {

					try { appr->addPoint(new_vec.data(), id_it->second); } catch (...) {}

				}

			}

#endif
- Line 920: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { appr->addPoint(new_vec.data(), id_it->second); } catch (...) {}
- Line 1086: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

			id = it->second;

		}

		try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }

	}

#endif

	// ScaNN / DiskANN alternative ANN backend
- Line 1086: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1168: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

			id = it->second;

		}

		try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }

	}

#endif

	// ScaNN / DiskANN alternative ANN backend
- Line 1168: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1320: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

						}

					}

				} catch (...) {}

			}

		}

	} else {
- Line 1320: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 1341: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

								auto enc_field = EncryptedField<std::vector<float>>::fromBase64(*enc_str);

								v = enc_field.decrypt();

							} catch (...) {

								// skip if decryption fails

							}

						}
- Line 1341: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1799: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> whitelistSet;
- Line 1849: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> inResults;
- Line 1909: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection;
- Line 1993: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({pk, dist});
- Line 2012: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!vecOpt) continue;

					cache_[pk] = *vecOpt;

					it = cache_.find(pk);

				} catch (...) { continue; }

			}

			if (it != cache_.end()) {

				float dist = distance(query, it->second);
- Line 2012: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { continue; }
- Line 2017: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({pk, dist});
- Line 2056: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> whitelistSet;
- Line 2065: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto j = nlohmann::json::parse(s);

			maxFilterScanSize = j.value("max_filter_scan_size", 100000);

		}

	} catch (...) {}



	bool isFirstFilter = true;

	for (const auto& filter : filters) {
- Line 2065: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 2088: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> inResults;
- Line 2127: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection;
- Line 2267: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
- Line 2272: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<hnswlib::SpaceInterface<float>*>(hnswSpace_);
- Line 2324: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: hnswIndex_ = static_cast<void*>(appr.release());
- Line 2523: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: quantMode = j.value("quantization", std::string("auto"));

			quantThreshold = j.value("auto_threshold", 1000000);

		}

	} catch (...) {}

	

	if (quantMode == "sq8") {

		shouldQuantize = true;
- Line 2523: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}

### index/graph_index.cpp
Total findings: 67

- Line 201: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto pos = s.find(',', start);
- Line 204: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 205: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 1373: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto pos = s.find(',', start);
- Line 1376: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 1377: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 1670: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto parseTemporalField = [&edge, &edgeId](std::string_view field) -> std::optional<int64_t> {
- Line 1732: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto parseTemporalField = [&edge, &edgeId](std::string_view field) -> std::optional<int64_t> {
- Line 179: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
- Line 216: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
- Line 282: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 307: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 322: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 384: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 413: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 453: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 532: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 702: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 726: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy: no graphId — LEGACY_COMPAT [INDEX-AUD-GI-03]: pre-v2.0 key format without graphId segment
- Line 963: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // - graph:out:<fromPk>:<edgeId>  (legacy)
- Line 965: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy: no graphId — LEGACY_COMPAT [INDEX-AUD-GI-03]: pre-v2.0 key format
- Line 984: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // LEGACY_COMPAT [INDEX-AUD-GI-03]: Support two formats — with graphId (v2.0+) or without (legacy)
- Line 1031: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 1042: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1042: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
- Line 1043: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1073: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1073: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
- Line 1074: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1142: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 1158: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1158: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
- Line 1159: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1185: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1185: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
- Line 1186: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1256: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 1351: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
- Line 1387: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
- Line 1454: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 1485: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 1784: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // - legacy: graph:out:<fromPk>:<edgeId>
- Line 1787: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // LEGACY_COMPAT [INDEX-AUD-GI-03]: fallback to legacy key format (pre-v2.0) for backward compatibility
- Line 1984: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 2098: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(order.begin(), order.end(), req) == order.end()) {
- Line 2171: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(current.path.begin(), current.path.end(), req) == current.path.end()) {
- Line 2200: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (topologyLoaded_.load(std::memory_order_acquire)) {
- Line 2251: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: encryptList.push_back("_weight");
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: encryptList.push_back("metadata");
- Line 446: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 692: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> nodes;
- Line 704: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 762: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> nodes;
- Line 1037: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: neighbors.push_back(adj.targetPk);
- Line 1126: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 1363: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 1378: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 1391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: encryptList.push_back("_weight");
- Line 1392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: encryptList.push_back("metadata");
- Line 1507: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 2131: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 2148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: start.path.push_back(std::string(startPk));

### index/spatial_index.cpp
Total findings: 45

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4145 feat(geo): Add SpatialIndex... (2026-03-13) | #3007 [geo] Implement R-t
- Line 218: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 224: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "%08d", z_bucket);
- Line 237: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 320: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "entries={}, geo_index_bytes_allocated={}",
- Line 385: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (cfg.total_bounds.minx == 0.0 && cfg.total_bounds.maxx == 0.0) {
- Line 391: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 481: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (entry.sidecar.z_min != 0.0 || entry.sidecar.z_max != 0.0) {
- Line 504: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Purge all spatial keys (legacy Morton buckets + per-PK keys) so that
- Line 542: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
- Line 571: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "geo_index_bytes_allocated={}",
- Line 611: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 612: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 613: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 614: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 617: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Save back to bucket (for backward compatibility)
- Line 635: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
- Line 696: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
- Line 741: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: update legacy Morton bucket key as well.
- Line 789: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy bucket key for backward compatibility and fallback query paths.
- Line 827: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: entries.end()

    );

    

    // Storage improvement: Also delete per-PK key

    std::string pk_key = makeSpatialPerPKKey(table, morton, primary_key);

    db_.del(pk_key); // Best effort
- Line 827: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Storage improvement: Also delete per-PK key
- Line 1067: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: result.z_min = entry.sidecar.z_min != 0.0
- Line 1069: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: result.z_max = entry.sidecar.z_max != 0.0
- Line 1162: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = candidate_mbrs.find(pk);
- Line 1170: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ── Fallback: tiny-bbox approach (legacy Morton-bucket data, no R-tree) ─
- Line 1350: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy data that pre-dates Z storage and is safe because the caller receives
- Line 1381: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));
- Line 345: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: config.total_bounds.miny = b.value("miny", -90.0);

                config.total_bounds.maxx = b.value("maxx", 180.0);

                config.total_bounds.maxy = b.value("maxy", 90.0);

            } catch (...) {

                // Use defaults if parsing fails

                config.total_bounds.minx = -180.0;

                config.total_bounds.miny = -90.0;
- Line 345: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 523: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, geo::MBR> local_cache;
- Line 924: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, geo::MBR> candidate_mbrs;
- Line 984: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: exact_match = exact_backend_->exactIntersects(entity_geom, query_geom);

                                if (exact_match) exact_passed_this_query++;

                            }

                        } catch (...) { exact_match = true; }

                    }

                } catch (...) { exact_match = true; }

            }
- Line 984: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { exact_match = true; }
- Line 986: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                        } catch (...) { exact_match = true; }

                    }

                } catch (...) { exact_match = true; }

            }



            if (exact_match) {
- Line 986: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { exact_match = true; }
- Line 1058: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: exact_match = exact_backend_->exactIntersects(entity_geom, query_geom);

                                    if (exact_match) exact_passed_this_query++;

                                }

                            } catch (...) { exact_match = true; }

                        }

                    } catch (...) { exact_match = true; }

                }
- Line 1058: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { exact_match = true; }
- Line 1060: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                            } catch (...) { exact_match = true; }

                        }

                    } catch (...) { exact_match = true; }

                }



                if (exact_match) {
- Line 1060: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { exact_match = true; }
- Line 1139: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, geo::MBR> candidate_mbrs;
- Line 218: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 224: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%08d", z_bucket);
- Line 237: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 1381: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));

### index/cuda_hnsw_graph_traversal.cpp
Total findings: 44

- Line 367: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
- Line 373: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const size_t new_pool_sz  = impl_->max_batch_size * vis_per_q;
- Line 375: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
- Line 375: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
- Line 377: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->visited_pool_bytes = new_pool_sz;
- Line 384: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->visited_pool_bytes = 0;
- Line 386: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
- Line 608: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_pass_ids
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_pass_ids = nullptr;
- Line 614: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
- Line 787: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: << ", visited_pool=" << (impl_ ? impl_->visited_pool_bytes : 0) << "B"
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5168 Complete GPU Vector Indexin... (2026-05-19) | #5145 research: fix and f
- Line 15: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: * the implementation allocates device memory and issues kernel launches via
- Line 240: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // cudaMalloc/cudaFree overhead.  Each kernel thread zeroes its own slice
- Line 328: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(vectors) failed");
- Line 334: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMemcpy(vectors) failed");
- Line 348: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(graph) failed");
- Line 358: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMemcpy(graph) failed");
- Line 367: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
- Line 374: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 379: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "allocated visited pool {} bytes "
- Line 386: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
- Line 470: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: //   pre-allocated pool — no per-launch cudaMalloc/cudaFree needed.
- Line 492: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: bool all_ok = (cudaMemcpy(d_queries_all, queries,
- Line 497: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMemcpy(queries H2D) failed — falling back to CPU");
- Line 512: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: this_chunk * k * sizeof(int64_t)) != cudaSuccess) {
- Line 519: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(impl_->d_result_ids);
- Line 556: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: h_ids.size() * sizeof(int64_t),
- Line 558: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_scores.data(), impl_->d_result_scores,
- Line 562: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMemcpy(results D2H) failed");
- Line 576: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_queries_all);
- Line 599: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: const cudaError_t e1 = cudaMalloc(&d_pass_ids,
- Line 600: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: mp_chunk * pass_k * sizeof(int64_t));
- Line 602: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: ? cudaMalloc(&d_pass_scores,
- Line 608: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_pass_ids);
- Line 614: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
- Line 618: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: queries_ok = (cudaMemcpy(d_queries_all, queries,
- Line 623: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMemcpy(queries H2D) failed");
- Line 672: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: h_ids.size() * sizeof(int64_t),
- Line 674: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_sc.data(),  d_pass_scores,
- Line 678: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: "cudaMemcpy(results D2H) failed");
- Line 696: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_queries_all);
- Line 710: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: return a.second == b.second;
- Line 745: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_pass_ids);
- Line 746: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_pass_scores);

### index/gpu_vector_index.cpp
Total findings: 36

- Line 1136: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1139: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1143: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1335: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int32_t metric = static_cast<int32_t>(pImpl->config.metric);
- Line 1428: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1434: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1439: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1468: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1471: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pImpl->oversubBulkLoading_ = false;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4186 feat(index): GPU Memory Ove... (2026-03-13) | #4138 feat(index): Implem
- Line 194: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return static_cast<uint64_t>(dimension) * sizeof(float);
- Line 223: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (Backend candidateBackend : getBackendPriorityOrder()) {
- Line 258: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::to_string(indexCounter.fetch_add(1, std::memory_order_relaxed));
- Line 317: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: mgr.DeallocateGPU(vramAllocatedBytes, vramBudgetTag);
- Line 474: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 488: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 547: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(bytes, vramBudgetTag);
- Line 1079: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: uint64_t allocatedBytes = 0;
- Line 1081: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocatedBytes = static_cast<uint64_t>(dim) * sizeof(float) * static_cast<uint64_t>(ids.size());
- Line 1081: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: allocatedBytes = static_cast<uint64_t>(dim) * sizeof(float) * static_cast<uint64_t>(ids.size());
- Line 1083: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!mgr.TryAllocateGPU(allocatedBytes, "vector_batch", pImpl->vramBudgetTag)) {
- Line 1101: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (allocatedBytes > 0) {
- Line 1102: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 1103: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocatedBytes, pImpl->vramBudgetTag);
- Line 1108: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (allocatedBytes > 0) {
- Line 1109: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: pImpl->vramAllocatedBytes += allocatedBytes;
- Line 1381: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Stored for future compatibility; callers set the metric via Config
- Line 1409: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 1577: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 997: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(Backend::CPU); // Always available
- Line 1003: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(Backend::CUDA);
- Line 1603: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(Backend::HIP);
- Line 1612: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(Backend::CUDA);
- Line 1619: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(Backend::VULKAN);

### index/graph_analytics.cpp
Total findings: 33

- Line 79: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: dr.out_degree = static_cast<int>(out_it->second.size());
- Line 84: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: dr.in_degree = static_cast<int>(in_it->second.size());
- Line 108: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return {Status::Error("Damping factor must be in [0, 1]"), {}};
- Line 144: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto out_it = topo.outgoing.find(pk);
- Line 145: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto out_it = topo.outgoing.find(pk);
- Line 166: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = new_ranks.find(neighbor);
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 243: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto out_it = topo.outgoing.find(v);
- Line 388: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (m == 0.0) m = 1.0;  // Avoid division by zero
- Line 455: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 456: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 460: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 655: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 655: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '                // Update if we found a better path', '                if (!best_dist.count(neighbor) || new_dist < best_dist[neighbor]) {', '                    best_dist[neighbor] = new_dist;', '']
- Line 658: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 661: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 769: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                }', '', '                total_path.length = root_length + spur_path.length;', '                total_path.hop_count = static_cast<int>(total_path.edges.size());', '']
- Line 60: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, GraphAnalytics::DegreeResult>>
- Line 72: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, DegreeResult> results;
- Line 95: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
- Line 126: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> ranks;
- Line 127: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> new_ranks;
- Line 198: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
- Line 212: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> betweenness;
- Line 221: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> predecessors; // predecessors on shortest paths
- Line 222: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int> distance;
- Line 223: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int> sigma; // number of shortest paths
- Line 224: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> delta; // dependency
- Line 288: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
- Line 301: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> closeness;
- Line 307: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, int> distance;
- Line 566: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += v + "|";
- Line 586: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields

### index/distributed_vector_index.cpp
Total findings: 29

- Line 197: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
- Line 262: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: alive_ids_[old_shard].insert(old_id);
- Line 279: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
- Line 281: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return insert(pk, vec.data(), vec.size());
- Line 346: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<AnnSearchResult> merged;
- Line 347: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.reserve(best_by_global_id.size());
- Line 349: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back({id, candidate.distance});
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3136 [index] Wire distributed ve... (2026-03-12) | #3034 feat(index): Distri
- Line 249: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 250: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 253: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 255: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 257: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 326: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = local_to_global_id_[s].find(r.id);
- Line 346: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<AnnSearchResult> merged;
- Line 347: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.reserve(best_by_global_id.size());
- Line 349: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back({id, candidate.distance});
- Line 352: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Merge: sort by distance (ascending) and keep top-k.
- Line 353: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::sort(merged.begin(), merged.end(),
- Line 355: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (a.distance != b.distance) {
- Line 361: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (static_cast<int>(merged.size()) > k) {
- Line 362: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.resize(static_cast<size_t>(k));
- Line 364: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 220: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto old_version_it = local_to_global_version_[old_shard].find(old_id);
- Line 316: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int64_t, SearchCandidate> best_by_global_id;
- Line 321: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto partial = shards_[s]->search(query, dim, k);
- Line 326: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = local_to_global_id_[s].find(r.id);
- Line 331: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto version_it = local_to_global_version_[s].find(r.id);

### index/inverted_index.cpp
Total findings: 25

- Line 129: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return Status::Error("InvertedIndex::drop: failed to delete meta key");
- Line 129: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (table.empty() || column.empty())

        return Status::Error("InvertedIndex::drop: table/column must not be empty");

    if (!db_.del(makeMetaKey(table, column)))

        return Status::Error("InvertedIndex::drop: failed to delete meta key");

    THEMIS_INFO("InvertedIndex dropped: {}.{}", table, column);

    return Status::OK();

}
- Line 129: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: return Status::Error("InvertedIndex::drop: failed to delete meta key");
- Line 224: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string_view column,

                                    std::string_view pk) {

    // Read the reverse-index key to discover which tokens are currently stored

    // for this pk, then delete each posting + TF entry.

    auto revKey = makeRevKey(table, column, pk);

    auto val = db_.get(revKey);

    if (val && !val->empty()) {
- Line 224: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // for this pk, then delete each posting + TF entry.
- Line 489: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (field.find(phraseNorm) != std::string::npos)
- Line 490: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (field.find(phraseNorm) != std::string::npos)
- Line 560: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = pkScores.find(pk);
- Line 272: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, uint32_t> tf;
- Line 332: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> postings;
- Line 335: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> pks;
- Line 347: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection = postings[0];
- Line 349: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tmp;
- Line 357: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> universe;
- Line 363: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> docLen;
- Line 370: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: double dl = 0.0;

        if (v && !v->empty()) {

            std::string s(v->begin(), v->end());

            try { dl = static_cast<double>(std::stoull(s)); } catch (...) {}

        }

        docLen[pk]  = dl;

        totalLen   += dl;
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) {}
- Line 401: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: double tf = 1.0;

            if (tfv && !tfv->empty()) {

                std::string s(tfv->begin(), tfv->end());

                try { tf = static_cast<double>(std::stoul(s)); } catch (...) {}

            }

            double denom = tf + k1 * (1.0 - b + b * (dl / avgdl));

            if (denom <= 0.0) denom = tf + k1;
- Line 401: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { tf = static_cast<double>(std::stoul(s)); } catch (...) {}
- Line 447: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_set<std::string>> postings;
- Line 449: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> pks;
- Line 460: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> candidates = postings[0];
- Line 462: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tmp;
- Line 540: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> pkScores;
- Line 396: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);

### index/advanced_vector_index.cpp
Total findings: 24

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ivf_pq->nprobe = config_.nprobe;
- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ivf_pq->polysemous_ht = config_.polysemous_ht;
- Line 125: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ivf_flat->nprobe = config_.nprobe;
- Line 63: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: AdvancedVectorIndex::~AdvancedVectorIndex() {
- Line 66: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::Index*>(index_);
- Line 66: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: AdvancedVectorIndex::~AdvancedVectorIndex() {

#ifdef THEMIS_HAS_FAISS

    if (index_) {

        delete static_cast<faiss::Index*>(index_);

        index_ = nullptr;

    }

#endif
- Line 66: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::Index*>(index_);
- Line 91: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 91: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: faiss::METRIC_L2

                );

                auto* ivf_pq = ivf_pq_owner.get();

                ivf_pq->own_fields = true; // FAISS will delete the quantizer

                quantizer_owner.release(); // ownership transferred to ivf_pq

                ivf_pq->nprobe = config_.nprobe;
- Line 91: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 123: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 123: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: faiss::METRIC_L2

                );

                auto* ivf_flat = ivf_flat_owner.get();

                ivf_flat->own_fields = true; // FAISS will delete the quantizer

                quantizer_owner.release(); // ownership transferred to ivf_flat

                ivf_flat->nprobe = config_.nprobe;

                idx = ivf_flat_owner.release();
- Line 123: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 508: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::Index*>(index_);
- Line 508: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: #ifdef THEMIS_HAS_FAISS

    try {

        if (index_) {

            delete static_cast<faiss::Index*>(index_);

            index_ = nullptr;  // prevent dangling pointer if read_index throws

        }
- Line 508: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::Index*>(index_);
- Line 40: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'FAISS Integration (v1.5.0)' that was not found in 'src/index/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/index/FUTURE_ENHANCEMENTS.md § "FAISS Integration (v1.5.0)"
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: quantizer_owner.release(); // ownership transferred to ivf_pq
- Line 106: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: idx = ivf_pq_owner.release();
- Line 123: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 124: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: quantizer_owner.release(); // ownership transferred to ivf_flat
- Line 126: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: idx = ivf_flat_owner.release();
- Line 134: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: idx = hnsw_owner.release();

### index/process_graph.cpp
Total findings: 23

- Line 1016: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: fields["completed_at"] = static_cast<int64_t>(*token->completed_at_ms);
- Line 162: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: * @brief Serialize a ProcessGraphVisitLog to a JSON string (node_id -> ns since epoch).
- Line 1779: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Push new entry for neighbor
- Line 1779: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 307: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto val = variables[expr];
- Line 330: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto leftVal = variables[left];
- Line 617: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ProcessGraphManager::validateProcess(std::string_view process_id) const {
- Line 709: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> hasIncoming;
- Line 710: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> hasOutgoing;
- Line 888: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (varsStr) {

        try {

            instance.variables = nlohmann::json::parse(*varsStr);

        } catch (...) {

            instance.variables = nlohmann::json::object();

        }

    }
- Line 888: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1175: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ProcessGraphManager::Status ProcessGraphManager::suspendProcess(std::string_view instance_id) {
- Line 1192: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ProcessGraphManager::Status ProcessGraphManager::resumeProcess(std::string_view instance_id) {
- Line 1673: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> adjacency;
- Line 1674: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> nodeDurations;
- Line 1736: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 1829: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                    }

                }

            } catch (...) {}

        }

        

        auto targetsStr = entity.getFieldAsString("targets");
- Line 1829: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 1843: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                    }

                }

            } catch (...) {}

        }

        

        // Parse sync type
- Line 1843: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 1921: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Process Graph Multi-Model Query Engine' that was not found in 'src/index/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/index/FUTURE_ENHANCEMENTS.md §"Process Graph Multi-Model Query Engine"
- Line 2552: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, NodeBaseline> baselines;
- Line 2600: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string> normalSet(normalNodes.begin(), normalNodes.end());

### index/gnn_embeddings.cpp
Total findings: 15

- Line 25: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<BaseEntity> deserializeEntitySafe(
- Line 30: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return BaseEntity::deserialize(std::string(entity_id), blob);
- Line 234: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto neighborEntity = deserializeEntitySafe(neighbor_ids[i], *blob);
- Line 441: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto nodeEntity = deserializeEntitySafe(node_pk, *blob);
- Line 536: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto edgeEntity = deserializeEntitySafe(edge_id, *blob);
- Line 606: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embEntity = deserializeEntitySafe(keyStr, blobBytes);
- Line 615: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: embedding_dim = static_cast<int>(embOpt->size());
- Line 674: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embEntity = deserializeEntitySafe(embKey, *blob);
- Line 711: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embEntity = deserializeEntitySafe(embKey, *blob);
- Line 160: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 161: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 302: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t j = 0; j < std::min(nf.size(), embedding.size()); ++j) {
- Line 125: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 4) result.entity_id += ":";
- Line 126: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 4) result.entity_id += ":";
- Line 143: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;

### index/multi_vector_search.cpp
Total findings: 13

- Line 209: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = score_map.find(doc_id);
- Line 391: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator vec_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto vec_it = vector_score_by_doc.find(doc_id);
- Line 401: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator kw_it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto kw_it = keyword_scores.find(doc_id);
- Line 208: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = score_map.find(doc_id);
- Line 209: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = score_map.find(doc_id);
- Line 390: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto vec_it = vector_score_by_doc.find(doc_id);
- Line 400: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto kw_it = keyword_scores.find(doc_id);
- Line 168: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::vector<std::unordered_map<std::string, std::pair<float, int>>> per_query_scores;
- Line 171: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::pair<float, int>> score_map;
- Line 184: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> all_docs;
- Line 360: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> all_docs;
- Line 361: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::pair<float, int>> vector_score_by_doc;
- Line 100: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // 1. Validate inputs

### index/product_quantizer.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #1005 [REFACTOR] Quantizer analys... (2026-03-11) | #1072 Add Vector Indexing
- Line 68: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("num_centroids must be in range [1, 256]");
- Line 199: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '        // Run K-means to find centroids', '        codebooks_[sq_index] = runKMeans(subvector_data);', '', '        THEMIS_DEBUG("ProductQuantizer::train - Subquantizer {}/{} trained",']
- Line 268: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        );', '', '        uint8_t code = findNearestCentroid(subvec, codebooks_[sq_index]);', '        codes.push_back(code);', '    }']
- Line 424: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: !checkedMultiply(static_cast<size_t>(config_.num_subquantizers), sizeof(std::vector<uint8_t>::value_
- Line 504: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (int d = 0; d < subvector_dim_; ++d) {

                    const size_t centroid_index =

                        static_cast<size_t>(i) * static_cast<size_t>(subvector_dim_) + static_cast<size_t>(d);

                    centroid[d] = centroid_data[centroid_index];

                }

                centroids.push_back(std::move(centroid));

            }
- Line 541: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: float dist = l2Distance(subvector_data[j], centroid);
- Line 567: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: int best_cluster = 0;

            

            for (int j = 0; j < k; ++j) {

                float dist = l2Distance(subvector_data[i], centroids[j]);

                if (dist < min_dist) {

                    min_dist = dist;

                    best_cluster = j;
- Line 578: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 590: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: counts[cluster]++;

            

            for (int d = 0; d < subvector_dim_; ++d) {

                new_centroids[cluster][d] += subvector_data[i][d];

            }

        }
- Line 603: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### index/gpu_vector_index_vulkan.cpp
Total findings: 9

- Line 645: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const uint32_t workgroupsX = (static_cast<uint32_t>(num_vectors_) + 15u) / 16u;

            const uint32_t workgroupsY = (static_cast<uint32_t>(numQueries) + 15u) / 16u;

            pipeline->dispatch(workgroupsX, workgroupsY, 1);

            if (!pipeline->wait()) {

                THEMIS_ERROR("VulkanVectorIndexBackend: pipeline->wait() timed out in batchSearch (GPU hang?)");

                return {};

            }
- Line 645: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!pipeline->wait()) {
- Line 646: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const uint32_t workgroupsY = (static_cast<uint32_t>(numQueries) + 15u) / 16u;

            pipeline->dispatch(workgroupsX, workgroupsY, 1);

            if (!pipeline->wait()) {

                THEMIS_ERROR("VulkanVectorIndexBackend: pipeline->wait() timed out in batchSearch (GPU hang?)");

                return {};

            }
- Line 646: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: THEMIS_ERROR("VulkanVectorIndexBackend: pipeline->wait() timed out in batchSearch (GPU hang?)");
- Line 146: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: a.first == b.first &&
- Line 147: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: a.middle == b.middle &&
- Line 148: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: a.last == b.last;
- Line 713: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (avg_query_time_ms_ == 0.0) {
- Line 930: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'GPU Vector Index (Vulkan)' that was not found in 'src/index/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/index/FUTURE_ENHANCEMENTS.md §"GPU Vector Index (Vulkan)"

### index/index_compression.cpp
Total findings: 9

- Line 62: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: double m_dbl = -static_cast<double>(expected_elements)
- Line 200: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Need to rebuild existing suffixes with new (shorter) prefix
- Line 200: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 201: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 221: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Start a new block
- Line 221: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 100: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void BloomFilter::clear() {
- Line 114: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> freq;
- Line 61: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double ln2   = std::log(2.0);

### index/multi_gpu_vector_index.cpp
Total findings: 9

- Line 304: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 340: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> topologyLock(topologyMutex);
- Line 392: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> topologyLock(topologyMutex);
- Line 273: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stats = gpuIndices[i]->getStatistics();
- Line 353: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto gpuResults = gpuIndices[gpuIdx]->search(query, k);
- Line 412: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto res = gpuIndices[gpuIdx]->searchBatch(queries, k);
- Line 517: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto gpuStats = gpuIndices[i]->getStatistics();
- Line 599: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stats = gpuIndices[i]->getStatistics();
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vectorsPerGPU.push_back(stats.numVectors);

### index/property_graph.cpp
Total findings: 8

- Line 51: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward-compatible fallback below (legacy comma-separated encoding).
- Line 54: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy format fallback: comma-separated string.
- Line 79: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // current JSON-array serialization and the legacy comma-separated format.
- Line 1247: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Compute new PageRank scores
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: labels.push_back(std::string(label));
- Line 730: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: PropertyGraphManager::federatedQuery(const std::vector<FederationPattern>& patterns) const {
- Line 1212: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> outgoing_count;
- Line 1213: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> incoming_nodes;

### index/vector_auto_buffer.cpp
Total findings: 7

- Line 54: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 87: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 336: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: status = vectorIndex_->addBatch(compressed_adds, config_.vector_field);
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: status = vectorIndex_->updateBatch(compressed_updates, config_.vector_field);
- Line 124: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 250: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 410: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);

### index/gpu_memory_oversubscription.cpp
Total findings: 6

- Line 42: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
- Line 219: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = lru_map.find(partition_id);
- Line 42: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
- Line 188: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: .allocate(bytes, alloc_tag);
- Line 512: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pImpl_->partitions.find(pid);
- Line 529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pImpl_->partitions.find(pid);

### index/graph_auto_buffer.cpp
Total findings: 6

- Line 40: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 74: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 104: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 163: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 220: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(30))) {
- Line 337: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);

### index/hnsw_parameter_tuner.cpp
Total findings: 6

- Line 242: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 247: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 252: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 257: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 432: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WorkloadClassifier::recordQuery(size_t k) {

### index/learnable_rope.cpp
Total findings: 6

- Line 85: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 88: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 243: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: double m_hat = adam_m_[i] / (1.0 - std::pow(config.adam_beta1, adam_t_));
- Line 509: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                            double value = std::stod(token);

                            loaded_theta.push_back(value);

                        } catch (...) {

                            // Skip invalid values

                        }

                    }
- Line 509: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 522: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();

### index/ann_index.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3136 [index] Wire distributed ve... (2026-03-12) | #2946 feat(index): DiskAN
- Line 161: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 176: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 221: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (bits >= sizeof(size_t) * 8) {

### index/rotary_embeddings_hip.cpp
Total findings: 5

- Line 113: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t allocated_batch_size = 0;
- Line 254: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_resources_->allocated_batch_size = 0;
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_resources_->allocated_batch_size = batch_size;
- Line 273: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int threads_per_block = 256;
- Line 329: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int threads_per_block = 256;

### index/residual_quantizer.cpp
Total findings: 4

- Line 139: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stage_codes = stage_quantizers_[stage]->encode(residual);
- Line 144: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto approx = stage_quantizers_[stage]->decode(stage_codes);
- Line 181: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
- Line 229: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);

### index/adaptive_index.cpp
Total findings: 3

- Line 204: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // No need to delete - unique_ptr handles cleanup automatically
- Line 375: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> analyzerLock(analyzerMutex_);
- Line 556: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> analyzerLock(analyzerMutex_);

### index/rotary_embeddings.cpp
Total findings: 3

- Line 205: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 208: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 259: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (norm_squared == 0.0) {

### index/approximate_radius_search.cpp
Total findings: 2

- Line 48: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs
- Line 261: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs

### index/hnsw_production_defaults.cpp
Total findings: 2

- Line 367: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: double m_factor = params.M / 16.0;
- Line 103: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: params.ml = 1.0 / std::log(static_cast<double>(params.M));

### index/index_manager.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4587 feat(index): add IndexManag... (2026-04-13) | #3174 [index] Implement S
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(key.substr(prefix.size()));

### index/learned_quantizer.cpp
Total findings: 2

- Line 163: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 164: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### index/workload_replay.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2517 [index] Automated index adv... (2026-03-11)
- Line 83: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void WorkloadCapture::recordQuery() {

### index/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=missing_adr_reference
  Description: Architecture doc missing ADR references: adr_001
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Add explicit ADR links/references for module-critical design decisions

### index/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### index/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### index/edge_types.cpp
Total findings: 1

- Line 354: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = category_index_.find(category);

### index/temporal_graph.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3114 [graph] Add time-range trav... (2026-03-12)

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
