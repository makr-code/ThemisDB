# index Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: index
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 1103
- Actionable Findings (Critical + High): 501
- Affected Files: 39

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 195 |
| High | 306 |
| Medium | 598 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 301 |
| reliability | 134 |
| concurrency | 133 |
| container | 127 |
| determinism | 94 |
| exception_safety | 80 |
| gpu_memory_safety | 41 |
| memory | 40 |
| raii | 40 |
| performance | 38 |
| audit_logging | 33 |
| legacy_duplication | 26 |
| platform | 15 |
| distributed_consistency | 13 |
| observability | 10 |
| input_validation | 9 |
| llm_ai_safety | 8 |
| type_conversion | 5 |
| uninitialized | 4 |
| security | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/index/secondary_index.cpp | 260 | 66 | 64 | 129 | 1 |
| src/index/vector_index.cpp | 128 | 42 | 22 | 64 | 0 |
| src/index/process_graph.cpp | 84 | 2 | 4 | 78 | 0 |
| src/index/cuda_hnsw_graph_traversal.cpp | 60 | 14 | 40 | 6 | 0 |
| src/index/graph_index.cpp | 60 | 8 | 18 | 34 | 0 |
| src/index/gpu_vector_index.cpp | 54 | 13 | 19 | 22 | 0 |
| src/index/spatial_index.cpp | 46 | 4 | 21 | 21 | 0 |
| src/index/graph_analytics.cpp | 43 | 2 | 18 | 23 | 0 |
| src/index/inverted_index.cpp | 34 | 0 | 4 | 29 | 1 |
| src/index/gnn_embeddings.cpp | 31 | 8 | 4 | 19 | 0 |
| src/index/advanced_vector_index.cpp | 27 | 11 | 5 | 11 | 0 |
| src/index/distributed_vector_index.cpp | 27 | 6 | 13 | 8 | 0 |
| src/index/ann_index.cpp | 25 | 0 | 10 | 15 | 0 |
| src/index/index_compression.cpp | 24 | 0 | 5 | 18 | 1 |
| src/index/multi_vector_search.cpp | 23 | 1 | 8 | 14 | 0 |
| src/index/vector_auto_buffer.cpp | 19 | 5 | 3 | 11 | 0 |
| src/index/multi_gpu_vector_index.cpp | 18 | 0 | 1 | 17 | 0 |
| src/index/gpu_vector_index_vulkan.cpp | 17 | 2 | 4 | 11 | 0 |
| src/index/product_quantizer.cpp | 17 | 0 | 9 | 8 | 0 |
| src/index/property_graph.cpp | 11 | 0 | 1 | 10 | 0 |
| src/index/gpu_memory_oversubscription.cpp | 10 | 2 | 5 | 3 | 0 |
| src/index/index_manager.cpp | 10 | 0 | 3 | 7 | 0 |
| src/index/graph_auto_buffer.cpp | 8 | 4 | 3 | 1 | 0 |
| src/index/learnable_rope.cpp | 8 | 0 | 2 | 6 | 0 |
| src/index/hnsw_parameter_tuner.cpp | 7 | 0 | 6 | 1 | 0 |
| src/index/learned_quantizer.cpp | 7 | 0 | 2 | 5 | 0 |
| src/index/residual_quantizer.cpp | 7 | 0 | 0 | 7 | 0 |
| src/index/rotary_embeddings_hip.cpp | 6 | 3 | 3 | 0 | 0 |
| src/index/workload_replay.cpp | 6 | 0 | 1 | 5 | 0 |
| src/index/approximate_radius_search.cpp | 5 | 0 | 2 | 3 | 0 |
| src/index/rotary_embeddings.cpp | 5 | 0 | 3 | 2 | 0 |
| src/index/tiered_index_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/index/adaptive_index.cpp | 2 | 1 | 0 | 1 | 0 |
| src/index/edge_types.cpp | 2 | 1 | 0 | 1 | 0 |
| src/index/hnsw_layer_optimizer.cpp | 2 | 0 | 0 | 2 | 0 |
| src/index/lora_rope.cpp | 2 | 0 | 0 | 2 | 0 |
| src/index/rotary_embeddings_gpu_cpu.cpp | 2 | 0 | 2 | 0 | 0 |
| src/index/hnsw_production_defaults.cpp | 1 | 0 | 0 | 0 | 1 |
| src/index/temporal_graph.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/index/secondary_index.cpp
Total findings: 260

- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 1234: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 1234: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 1324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 1325: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uniqueIndex = (it != cachedMetadata->regular_unique.end()) ? it->second : isUniqueIndex_(table, col)
- Line 1393: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 1394: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: compositeUnique = (it != cachedMetadata->composite_unique.end()) && it->second;
- Line 1443: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseCols = cachedMetadata->sparse_indexes;
- Line 1458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->sparse_unique.find(scol);
- Line 1459: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseUnique = (it != cachedMetadata->sparse_unique.end()) ? it->second : isSparseIndexUnique_(table
- Line 1489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: geoCols = cachedMetadata->geo_indexes;
- Line 1523: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ttlCols = cachedMetadata->ttl_indexes;
- Line 1539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->ttl_seconds.find(tcol);
- Line 1540: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->ttl_seconds.end()) ttlSeconds = it->second;
- Line 1554: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: fulltextCols = cachedMetadata->fulltext_indexes;
- Line 1566: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 1567: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 1603: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (size_t i = 0; i < cachedMetadata->partial_indexes.size(); ++i) {
- Line 1604: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& col = cachedMetadata->partial_indexes[i];
- Line 1605: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1606: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->partial_predicates.end())
- Line 1622: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_unique.find(pcol);
- Line 1623: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: partialUnique = (it != cachedMetadata->partial_unique.end()) ? it->second : isPartialIndexUnique_(ta
- Line 1659: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 1663: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 1667: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 1671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 1675: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 1679: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 1683: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 1687: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1688: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result[col] = (it != cachedMetadata->partial_predicates.end()) ? it->second : "";
- Line 1886: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 1887: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 3902: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 3902: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 3903: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 3903: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 3992: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 3993: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uniqueIndex = (it != cachedMetadata->regular_unique.end()) ? it->second : isUniqueIndex_(table, col)
- Line 4069: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 4070: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: compositeUnique = (it != cachedMetadata->composite_unique.end()) && it->second;
- Line 4145: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->sparse_unique.find(scol);
- Line 4146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseUnique = (it != cachedMetadata->sparse_unique.end()) ? it->second : isSparseIndexUnique_(table
- Line 4226: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->ttl_seconds.find(tcol);
- Line 4227: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->ttl_seconds.end()) ttlSeconds = it->second;
- Line 4254: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 4255: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 4290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (size_t i = 0; i < cachedMetadata->partial_indexes.size(); ++i) {
- Line 4291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& col = cachedMetadata->partial_indexes[i];
- Line 4292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4293: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->partial_predicates.end())
- Line 4309: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_unique.find(pcol);
- Line 4310: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: partialUnique = (it != cachedMetadata->partial_unique.end()) ? it->second : isPartialIndexUnique_(ta
- Line 4348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 4352: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 4356: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 4360: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 4364: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 4368: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 4372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 4376: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4377: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result[col] = (it != cachedMetadata->partial_predicates.end()) ? it->second : "";
- Line 4575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 4576: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['// static', 'std::string SecondaryIndexManager::makeCompositeIndexMetaKey(std::string_view table, const std::vector<std::string>& columns) {', '\tsize_t total = 8 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1 + pk.size();', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encodedVals;', '\tencodedVals.reserve(values.size());', '\tsize_t total = 5 + table.size() + 1; // "uidx:" + table + ":"', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\tif (i > 0) total += 1; // "+"']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
  Confidence: band=high; score=0.81
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
- Line 311: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compatibility API: createIndex with IndexType enum
  Confidence: band=high; score=0.8
- Line 984: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case 0: return numericOk ? (fvNum == rhsNum) : (fv == rhs);
  Confidence: band=very_high; score=0.9
- Line 985: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case 1: return numericOk ? (fvNum != rhsNum) : (fv != rhs);
  Confidence: band=very_high; score=0.9
- Line 1247: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.regular_unique[col] = isUniqueIndex_(table, col);
- Line 1254: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
- Line 1265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_predicates[col] = pred;
- Line 1266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
- Line 1271: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
- Line 1283: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.fulltext_configs[fcol] = std::move(cached);
- Line 1292: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 1292: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 1314: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (col.find('+') == std::string::npos) {
- Line 1323: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 1392: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 1604: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1773: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 2295: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
- Line 2592: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2593: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
  Confidence: band=very_high; score=0.9
- Line 2651: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itLen = docLen.find(pk);
- Line 2651: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itLen = docLen.find(pk);
- Line 2652: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto itLen = docLen.find(pk);
  Confidence: band=very_high; score=0.9
- Line 2697: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Public API: returns PKs only (deprecated, use scanFulltextWithScores for scores)
  Confidence: band=high; score=0.8
- Line 3435: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::microseconds(throttle_us));
- Line 3476: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& token : tokenize(*maybeVal, config))
- Line 3915: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.regular_unique[col] = isUniqueIndex_(table, col);
- Line 3922: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
- Line 3933: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_predicates[col] = pred;
- Line 3934: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
- Line 3940: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
- Line 3952: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.fulltext_configs[fcol] = std::move(cached);
- Line 3961: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 3961: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 3982: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (col.find('+') == std::string::npos) {
- Line 3991: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 4068: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 4291: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4462: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 147: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idxmeta:";
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 180: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 190: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 214: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 224: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 255: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) total += 1; // "+"
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: total += 1 + encodedVals.back().size(); // ":" + encoded
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 273: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(c));
- Line 410: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // timestamp wird mit führenden Nullen auf 20 Zeichen padded für lexikografische Sortierung
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 547: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 762: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.emplace_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 768: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 864: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 981: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("put(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
- Line 1147: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("erase(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
- Line 1191: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1228: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* indexedColsPtr = nullptr;
  Confidence: band=medium; score=0.66
- Line 1229: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
  Confidence: band=medium; score=0.66
- Line 1230: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
  Confidence: band=medium; score=0.66
- Line 1263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 1416: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 1417: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 1513: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1580: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 1601: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> partialCols;
  Confidence: band=medium; score=0.66
- Line 1667: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1671: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1675: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1679: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1683: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
  Confidence: band=medium; score=0.66
- Line 1685: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> result;
  Confidence: band=medium; score=0.66
- Line 1775: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.emplace_back(col.substr(start));
  Confidence: band=high; score=0.74
- Line 1791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 1844: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1900: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 1987: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
- Line 1989: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2026: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 2027: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 2038: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.emplace_back(rest);
  Confidence: band=high; score=0.74
- Line 2061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
- Line 2063: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2114: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: startKey += '\xFF'; // Skip to next value
- Line 2210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(*it);
  Confidence: band=high; score=0.74
- Line 2261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2519: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
  Confidence: band=medium; score=0.66
- Line 2550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokenResults.emplace_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 2559: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersectionSet = tokenResults[0];
  Confidence: band=medium; score=0.66
- Line 2561: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2593: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2597: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!keep) toErase.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2611: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> universe;
  Confidence: band=medium; score=0.66
- Line 2623: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> docLen;
  Confidence: band=medium; score=0.66
- Line 2632: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) { dl = 0.0; }
- Line 2666: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { tf = static_cast<double>(std::stoul(sTF)); } catch (...) { tf = 1.0; }
- Line 2712: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.emplace_back(result.pk);
  Confidence: band=high; score=0.74
- Line 2755: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
  Confidence: band=medium; score=0.66
- Line 2767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokenResults.emplace_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 2776: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates = tokenResults[0];
  Confidence: band=medium; score=0.66
- Line 2778: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2824: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2893: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> tokenToDocs;
  Confidence: band=medium; score=0.66
- Line 2894: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> pkScores;
  Confidence: band=medium; score=0.66
- Line 2935: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(FulltextResult{pk, score});
  Confidence: band=high; score=0.74
- Line 2968: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.emplace_back(std::move(current));
  Confidence: band=high; score=0.74
- Line 3062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allStats.emplace_back(getIndexStats(table, column));
  Confidence: band=high; score=0.74
- Line 3225: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybeVal);
  Confidence: band=high; score=0.74
- Line 3495: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*mv);
  Confidence: band=high; score=0.74
- Line 3637: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 3638: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 3837: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 3872: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 3897: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* indexedColsPtr = nullptr;
  Confidence: band=medium; score=0.66
- Line 3898: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
  Confidence: band=medium; score=0.66
- Line 3899: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
  Confidence: band=medium; score=0.66
- Line 3931: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 3931: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 3931: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 4059: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 4080: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4080: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4081: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 4103: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4104: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 4128: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sparseCols;
  Confidence: band=medium; score=0.66
- Line 4174: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> geoCols;
  Confidence: band=medium; score=0.66
- Line 4200: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4208: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ttlCols;
  Confidence: band=medium; score=0.66
- Line 4239: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> fulltextCols;
  Confidence: band=medium; score=0.66
- Line 4268: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 4288: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> partialCols;
  Confidence: band=medium; score=0.66
- Line 4356: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4360: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4364: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4368: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4372: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
  Confidence: band=medium; score=0.66
- Line 4374: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> result;
  Confidence: band=medium; score=0.66
- Line 4464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.emplace_back(col.substr(start));
  Confidence: band=high; score=0.74
- Line 4480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 4533: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4589: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 2659: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
  Confidence: band=medium; score=0.6

### src/index/vector_index.cpp
Total findings: 128

- Line 639: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("VectorIndexManager::init - Failed to load index: {}, creating new index", loadStatus.me
- Line 648: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::L2Space(dim);
- Line 651: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::InnerProductSpace(dim);
- Line 653: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::InnerProductSpace(dim);
- Line 655: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, 1000 /*initial*/, M, efConstruction);
- Line 732: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt && vecOpt->size() == static_cast<size_t>(dim_)) {
- Line 740: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!qv || qv->size() != static_cast<size_t>(dim_)) return true;
- Line 816: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt && vecOpt->size() == static_cast<size_t>(dim_)) {
- Line 823: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!qv || qv->size() != static_cast<size_t>(dim_)) return true;
- Line 877: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator id_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto id_it = pkToId_.find(pk);
- Line 890: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: } else if (cache_it->second != new_vec) {
- Line 892: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_it->second = new_vec;
- Line 1055: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1068: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1075: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1089: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity: Vektordimension passt n
- Line 1136: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1149: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1158: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1158: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache_.end() && it->second.size() == static_cast<size_t>(dim_)) {
- Line 1266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache_.end() && it->second.size() == static_cast<size_t>(dim_)) {
- Line 1275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vec && vec->size() == static_cast<size_t>(dim_)) {
- Line 1282: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (by && by->size() == static_cast<size_t>(dim_)) {
- Line 1323: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt->size() == static_cast<size_t>(dim_)) v = *vecOpt;
- Line 1330: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (qv && qv->size() == static_cast<size_t>(dim_)) {
- Line 1368: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& vec = cache_ptrs[i]->second;
- Line 1440: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto topk = appr->searchKnn(q.data(), static_cast<size_t>(k));
- Line 1571: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto raw = ann_backend_->search(q.data(), static_cast<size_t>(dim_), static_cast<int>(k));
- Line 2234: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: if (metric_ == Metric::L2) space = new hnswlib::L2Space(dim_);
- Line 2235: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: else space = new hnswlib::InnerProductSpace(dim_);
- Line 2280: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, tempPath, false);
- Line 2301: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, indexPath, false);
- Line 2360: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity(mvcc): Vektordimension p
- Line 2407: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 2902: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto rope_stats = rotary_embedding_->getStats();
- Line 2928: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated = rotary_embedding_->rotate(*vec_opt, position);
- Line 2972: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated = rotary_embedding_->rotateRelational(*vec_opt, relation_type);
- Line 3011: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated_query = rotary_embedding_->rotate(query, query_position);
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
- Line 845: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (storage_vectors.find(pk) == storage_vectors.end())
- Line 853: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pkToId_.find(pk);
- Line 1183: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: fehlgeschlagen = nullptr;
  Context: THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
- Line 1232: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto prefetch = [](const void* ptr) {
- Line 1528: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (wl.find(pk) != wl.end()) {
- Line 2243: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2250: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2290: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2294: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Original plaintext load (backward compatibility)
  Confidence: band=high; score=0.8
- Line 2297: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2625: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t j = i + 1; j < std::min(i + 10, pks.size()); ++j) {
- Line 169: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hnsw_opt = config["hnsw_optimization"];
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lp = hnsw_opt["layer_pruning"];
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto als = hnsw_opt["adaptive_layer_selection"];
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bi = hnsw_opt["batch_insert"];
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 332: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 662: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 680: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 766: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 837: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_delete.push_back(pk);
  Confidence: band=high; score=0.74
- Line 856: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 880: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 886: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(new_vec.data(), id); } catch (...) {}
- Line 898: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(new_vec.data(), id_it->second); } catch (...) {}
- Line 970: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1063: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1144: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1193: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 1212: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 1252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: heap.push_back({pk, dist});
- Line 1293: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1314: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_ptrs.push_back(&entry);
  Confidence: band=high; score=0.74
- Line 1465: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(r);
  Confidence: band=high; score=0.74
- Line 1561: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({idToPk_[idx], r.distance});
  Confidence: band=high; score=0.74
- Line 1760: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
  Confidence: band=high; score=0.74
- Line 1761: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
- Line 1769: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
- Line 1779: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1819: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> inResults;
  Confidence: band=medium; score=0.66
- Line 1879: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 1908: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
  Confidence: band=high; score=0.74
- Line 1909: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
- Line 1944: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(c);
  Confidence: band=high; score=0.74
- Line 1961: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 1962: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, dist});
- Line 1981: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { continue; }
- Line 1985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 1986: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, dist});
- Line 2024: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
- Line 2033: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2056: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> inResults;
  Confidence: band=medium; score=0.66
- Line 2095: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2200: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2328: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2374: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2415: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 2443: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 2480: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_serialized.push_back(std::move(serialized));
  Confidence: band=high; score=0.74
- Line 2520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_serialized.push_back(std::move(serialized));
  Confidence: band=high; score=0.74
- Line 2619: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.push_back(pk);
  Confidence: band=high; score=0.74
- Line 2626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(dist);
  Confidence: band=high; score=0.74
- Line 2626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(dist);
  Confidence: band=high; score=0.74
- Line 2737: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(pk);
  Confidence: band=high; score=0.74
- Line 2796: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train_data.push_back(vec);
  Confidence: band=high; score=0.74
- Line 3056: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/process_graph.cpp
Total findings: 84

- Line 982: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: fields["completed_at"] = static_cast<int64_t>(*token->completed_at_ms);
- Line 1699: severity=CRITICAL; category=missing_dtor
  Description: Class StackEntry allocates resources but has no destructor
  Remediation: Add explicit destructor: ~StackEntry() { /* cleanup */ }
  Context: class/struct StackEntry
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 133: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: * @brief Serialize a ProcessGraphVisitLog to a JSON string (node_id -> ns since epoch).
- Line 1736: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Push new entry for neighbor
- Line 1736: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Push new entry for neighbor
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(item.get<std::string>());
- Line 277: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto val = variables[expr];
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto leftVal = variables[left];
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 587: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::validateProcess(std::string_view process_id) const {
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Process has no start event");
  Confidence: band=high; score=0.74
- Line 668: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Process has no start event");
- Line 671: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("Process has no end event");
- Line 675: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> hasIncoming;
  Confidence: band=medium; score=0.66
- Line 676: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> hasOutgoing;
  Confidence: band=medium; score=0.66
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
- Line 686: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Node '" + id + "' has no outgoing edges");
- Line 692: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_node + "'");
  Confidence: band=high; score=0.74
- Line 693: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_n
- Line 696: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent target '" + edge.to_nod
- Line 723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow"
- Line 854: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1020: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_all.push_back(n);
  Confidence: band=high; score=0.74
- Line 1020: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_all.push_back(n);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token->traversed_edges.push_back(edge.edge_id);
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1072: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1081: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token->traversed_edges.push_back(edge.edge_id);
  Confidence: band=high; score=0.74
- Line 1082: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1091: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(outgoing[0].edge_id);
- Line 1141: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::suspendProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::resumeProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
- Line 1370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1445: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1482: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1607: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 1639: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adjacency;
  Confidence: band=medium; score=0.66
- Line 1640: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> nodeDurations;
  Confidence: band=medium; score=0.66
- Line 1661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adjacency[from].push_back(to);
  Confidence: band=high; score=0.74
- Line 1707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({startNode, 0.0, {}, {}});
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
  Confidence: band=high; score=0.74
- Line 1737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
- Line 1781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.source_nodes.push_back(src.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1782: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hyperedge.source_nodes.push_back(src.get<std::string>());
- Line 1786: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1795: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.target_nodes.push_back(tgt.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1796: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hyperedge.target_nodes.push_back(tgt.get<std::string>());
- Line 1800: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1829: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1921: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 1950: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { vars = nlohmann::json::parse(*varsStr); } catch (...) {}
- Line 1960: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(jr));
  Confidence: band=high; score=0.74
- Line 2057: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2079: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (varsStr) { try { vars = nlohmann::json::parse(*varsStr); } catch (...) {} }
- Line 2141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (varsStr) { try { vars = nlohmann::json::parse(*varsStr); } catch (...) {} }
- Line 2204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_number()) emb.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 2241: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(tok));
  Confidence: band=high; score=0.74
- Line 2423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sp));
  Confidence: band=high; score=0.74
- Line 2515: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, NodeBaseline> baselines;
  Confidence: band=medium; score=0.66
- Line 2551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2563: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> normalSet(normalNodes.begin(), normalNodes.end());
  Confidence: band=medium; score=0.66
- Line 2578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2725: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2765: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2822: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 2862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 2951: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3043: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 3097: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(mmr));
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(edge.to_node);
  Confidence: band=high; score=0.74
- Line 3153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: targets.push_back(edge.to_node);

### src/index/cuda_hnsw_graph_traversal.cpp
Total findings: 60

- Line 347: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.99
- Line 353: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t new_pool_sz  = impl_->max_batch_size * vis_per_q;
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
- Line 355: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
  Confidence: band=very_high; score=0.99
- Line 357: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->visited_pool_bytes = new_pool_sz;
- Line 364: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->visited_pool_bytes = 0;
- Line 366: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
  Confidence: band=very_high; score=0.99
- Line 448: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t pool_capacity = (vis_per_q > 0 && impl_->visited_pool_bytes > 0)
- Line 484: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: if (cudaMalloc(&impl_->d_result_ids,
  Confidence: band=very_high; score=0.99
- Line 486: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaMalloc(&impl_->d_result_scores,
  Confidence: band=very_high; score=0.99
- Line 561: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: const cudaError_t e1 = cudaMalloc(&d_pass_ids,
  Confidence: band=very_high; score=0.99
- Line 564: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: ? cudaMalloc(&d_pass_scores,
  Confidence: band=very_high; score=0.99
- Line 570: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
  Confidence: band=very_high; score=0.99
- Line 734: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: << ", visited_pool=" << (impl_ ? impl_->visited_pool_bytes : 0) << "B"
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5168 Complete GPU Vector Indexin... (2026-05-19) | #5145 research: fix and f
- Line 15: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * the implementation allocates device memory and issues kernel launches via
- Line 238: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // cudaMalloc/cudaFree overhead.  Each kernel thread zeroes its own slice
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // cudaMalloc/cudaFree overhead.  Each kernel thread zeroes its own slice
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(vectors) failed");
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_vectors, vectors, vec_bytes, cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(graph) failed");
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_offsets,    bottom.offsets.data(),    off_bytes, cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_neighbours, bottom.neighbours.data(), nb_bytes,  cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "allocated visited pool {} bytes "
- Line 366: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: //   pre-allocated pool — no per-launch cudaMalloc/cudaFree needed.
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: //   pre-allocated pool — no per-launch cudaMalloc/cudaFree needed.
  Confidence: band=very_high; score=0.9
- Line 467: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_queries_all, queries,
  Confidence: band=very_high; score=0.9
- Line 469: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: this_chunk * k * sizeof(int64_t)) != cudaSuccess ||
- Line 522: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ids.data(), impl_->d_result_ids,
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: h_ids.size() * sizeof(int64_t),
- Line 524: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_scores.data(), impl_->d_result_scores,
  Confidence: band=very_high; score=0.9
- Line 527: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 538: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_queries_all);
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: const cudaError_t e1 = cudaMalloc(&d_pass_ids,
  Confidence: band=very_high; score=0.9
- Line 562: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: mp_chunk * pass_k * sizeof(int64_t));
- Line 564: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: ? cudaMalloc(&d_pass_scores,
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_queries_all, queries,
  Confidence: band=very_high; score=0.9
- Line 576: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ids.data(), d_pass_ids,
  Confidence: band=very_high; score=0.9
- Line 624: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: h_ids.size() * sizeof(int64_t),
- Line 625: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_sc.data(),  d_pass_scores,
  Confidence: band=very_high; score=0.9
- Line 628: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_queries_all);
  Confidence: band=very_high; score=0.9
- Line 657: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return a.second == b.second;
  Confidence: band=very_high; score=0.9
- Line 692: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_pass_ids);
  Confidence: band=very_high; score=0.9
- Line 693: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_pass_scores);
  Confidence: band=very_high; score=0.9
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[gqi].push_back(
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[gqi].push_back(
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[gqi].push_back(
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_cands[gqi].emplace_back(score, id);
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_cands[gqi].emplace_back(score, id);
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[qi].push_back({c.second, c.first});
  Confidence: band=high; score=0.74

### src/index/graph_index.cpp
Total findings: 60

- Line 177: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = s.find(',', start);
- Line 180: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 181: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 1292: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = s.find(',', start);
- Line 1295: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 1296: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 1582: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseTemporalField = [&edge](std::string_view field) -> std::optional<int64_t> {
- Line 1641: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseTemporalField = [&edge](std::string_view field) -> std::optional<int64_t> {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
  Confidence: band=high; score=0.81
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
- Line 189: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 1304: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 2004: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(order.begin(), order.end(), req) == order.end()) {
- Line 2077: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(current.path.begin(), current.path.end(), req) == current.path.end()) {
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 172: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("_weight");
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("metadata");
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(node);
  Confidence: band=high; score=0.74
- Line 652: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> nodes;
  Confidence: band=medium; score=0.66
- Line 664: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 722: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> nodes;
  Confidence: band=medium; score=0.66
- Line 756: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 760: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 767: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 803: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 807: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 814: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 878: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 968: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbors.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 969: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbors.push_back(adj.targetPk);
- Line 1058: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 1287: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
- Line 1297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 1308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("_weight");
- Line 1309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("metadata");
- Line 1419: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1591: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1650: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1994: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(vertex);
  Confidence: band=high; score=0.74

### src/index/gpu_vector_index.cpp
Total findings: 54

- Line 1068: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vectors[i].size() != static_cast<size_t>(pImpl->dimension) ||
- Line 1079: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: allocatedBytes = pImpl->bytesPerVector() * static_cast<uint64_t>(ids.size());
- Line 1135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1138: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1142: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1334: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int32_t metric = static_cast<int32_t>(pImpl->config.metric);
- Line 1428: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1434: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1439: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1453: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1461: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1470: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1473: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4186 feat(index): GPU Memory Ove... (2026-03-13) | #4138 feat(index): Implem
- Line 194: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return static_cast<uint64_t>(dimension) * sizeof(float);
- Line 223: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (Backend candidateBackend : getBackendPriorityOrder()) {
- Line 258: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: std::to_string(indexCounter.fetch_add(1, std::memory_order_relaxed));
- Line 320: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: mgr.DeallocateGPU(vramAllocatedBytes, vramBudgetTag);
- Line 550: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(bytes, vramBudgetTag);
- Line 1077: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: uint64_t allocatedBytes = 0;
- Line 1079: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocatedBytes = pImpl->bytesPerVector() * static_cast<uint64_t>(ids.size());
- Line 1081: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!mgr.TryAllocateGPU(allocatedBytes, "vector_batch", pImpl->vramBudgetTag)) {
- Line 1100: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocatedBytes > 0) {
- Line 1101: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 1102: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocatedBytes, pImpl->vramBudgetTag);
- Line 1107: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocatedBytes > 0) {
- Line 1108: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: pImpl->vramAllocatedBytes += allocatedBytes;
- Line 1380: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Stored for future compatibility; callers set the metric via Config
  Confidence: band=high; score=0.8
- Line 1409: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(dist, globalOffset + vi);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], candidates[i].first});
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[index], distance});
  Confidence: band=high; score=0.74
- Line 673: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[distances[i].second], distances[i].first});
  Confidence: band=high; score=0.74
- Line 727: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchCPU(q, k));
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 906: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchCPU(q, k));
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 1094: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pImpl->vectorIds.push_back(ids[i]);
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(pImpl->searchOversubscribed(query, k));
  Confidence: band=high; score=0.74
- Line 1207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryResults.push_back({pImpl->vectorIds[index], distance});
  Confidence: band=high; score=0.74
- Line 1268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(pImpl->searchCPU(query, k));
  Confidence: band=high; score=0.74
- Line 1435: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "GPUVectorIndex: loadIndex read error at vector " << i << " (ID length)\n";
- Line 1440: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "GPUVectorIndex: loadIndex rejected oversized ID (" << idLen
- Line 1584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends.push_back(Backend::CPU);
  Confidence: band=high; score=0.74
- Line 1585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::CPU);
- Line 1590: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::HIP);
- Line 1599: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::CUDA);

### src/index/spatial_index.cpp
Total findings: 46

- Line 731: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
- Line 798: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
- Line 897: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: entry_mbr = cache_it->second;
- Line 1091: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache.end()) result.mbr = it->second;
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4145 feat(geo): Add SpatialIndex... (2026-03-13) | #3007 [geo] Implement R-t
- Line 218: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 224: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%08d", z_bucket);
- Line 237: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 310: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "entries={}, geo_index_bytes_allocated={}",
- Line 375: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (cfg.total_bounds.minx == 0.0 && cfg.total_bounds.maxx == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 465: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (entry.sidecar.z_min != 0.0 || entry.sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 543: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "geo_index_bytes_allocated={}",
- Line 588: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Save back to bucket (for backward compatibility)
  Confidence: band=high; score=0.8
- Line 606: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 664: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1009: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.z_min = entry.sidecar.z_min != 0.0
  Confidence: band=very_high; score=0.9
- Line 1011: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.z_max = entry.sidecar.z_max != 0.0
  Confidence: band=very_high; score=0.9
- Line 1089: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cache.find(pk);
- Line 1308: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: g.rings.push_back({
- Line 299: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 335: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 345: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(item);
  Confidence: band=high; score=0.74
- Line 741: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
  Confidence: band=high; score=0.74
- Line 926: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 928: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 963: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kvs.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
- Line 1000: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 1002: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 1051: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1091: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1263: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cand));
  Confidence: band=high; score=0.74
- Line 1331: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/graph_analytics.cpp
Total findings: 43

- Line 79: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dr.out_degree = static_cast<int>(out_it->second.size());
- Line 84: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dr.in_degree = static_cast<int>(in_it->second.size());
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                }', '', '                total_path.length = root_length + spur_path.length;', '                total_path.hop_count = static_cast<int>(total_path.edges.size());', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '                // Update if we found a better path', '                if (!best_dist.count(neighbor) || new_dist < best_dist[neighbor]) {', '                    best_dist[neighbor] = new_dist;', '']
  Confidence: band=high; score=0.81
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
- Line 108: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return {Status::Error("Damping factor must be in [0, 1]"), {}};
- Line 144: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 144: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 144: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 144: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 145: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto out_it = topo.outgoing.find(pk);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = new_ranks.find(neighbor);
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(v);
- Line 388: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (m == 0.0) m = 1.0;  // Avoid division by zero
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 60: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, GraphAnalytics::DegreeResult>>
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, DegreeResult> results;
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> ranks;
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> new_ranks;
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_degrees.push_back((out_it != topo.outgoing.end()) ? out_it->second.size() : 0);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> betweenness;
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> predecessors; // predecessors on shortest paths
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> distance;
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> sigma; // number of shortest paths
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> delta; // dependency
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back(v);
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predecessors[w].push_back(v);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> closeness;
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> distance;
  Confidence: band=high; score=0.74
- Line 566: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += v + "|";
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_path_vertices.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 730: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: total_path.vertices.push_back(spur_path.vertices[i]);
  Confidence: band=high; score=0.74
- Line 731: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: total_path.vertices.push_back(spur_path.vertices[i]);
- Line 733: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: total_path.edges.push_back(edge);
  Confidence: band=high; score=0.74

### src/index/inverted_index.cpp
Total findings: 34

- Line 129: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: meta = nullptr;
  Context: return Status::Error("InvertedIndex::drop: failed to delete meta key");
- Line 489: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (field.find(phraseNorm) != std::string::npos)
- Line 490: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field.find(phraseNorm) != std::string::npos)
  Confidence: band=very_high; score=0.9
- Line 560: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pkScores.find(pk);
  Confidence: band=very_high; score=0.9
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (w.is_string()) cfg.stopwords.push_back(w.get<std::string>());
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (w.is_string()) cfg.stopwords.push_back(w.get<std::string>());
- Line 157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 272: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revTokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revTokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> postings;
  Confidence: band=medium; score=0.66
- Line 335: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> pks;
  Confidence: band=medium; score=0.66
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: postings.push_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection = postings[0];
  Confidence: band=medium; score=0.66
- Line 349: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tmp;
  Confidence: band=medium; score=0.66
- Line 357: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> universe;
  Confidence: band=medium; score=0.66
- Line 363: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> docLen;
  Confidence: band=medium; score=0.66
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) {}
- Line 401: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { tf = static_cast<double>(std::stoul(s)); } catch (...) {}
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({std::string(pk), score});
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({std::string(pk), score});
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> postings;
  Confidence: band=medium; score=0.66
- Line 449: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> pks;
  Confidence: band=medium; score=0.66
- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: postings.push_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 460: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates = postings[0];
  Confidence: band=medium; score=0.66
- Line 462: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tmp;
  Confidence: band=medium; score=0.66
- Line 490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, 1.0});
  Confidence: band=high; score=0.74
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, 1.0});
- Line 492: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 540: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> pkScores;
  Confidence: band=medium; score=0.66
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, score});
  Confidence: band=high; score=0.74
- Line 396: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
  Confidence: band=medium; score=0.6

### src/index/gnn_embeddings.cpp
Total findings: 31

- Line 213: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity neighbor = BaseEntity::deserialize(neighbor_ids[i], *blob);
  Confidence: band=very_high; score=0.99
- Line 416: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity node = BaseEntity::deserialize(std::string(node_pk), *blob);
  Confidence: band=very_high; score=0.99
- Line 506: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity edge = BaseEntity::deserialize(std::string(edge_id), *blob);
  Confidence: band=very_high; score=0.99
- Line 570: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(keyStr, std::vector<uint8_t>(val.begin(), val.end()));
  Confidence: band=very_high; score=0.99
- Line 576: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_dim = static_cast<int>(embOpt->size());
- Line 576: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_dim = static_cast<int>(embOpt->size());
- Line 635: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 669: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 140: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 140: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 141: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (visited.find(neighbor) == visited.end()) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t j = 0; j < std::min(nf.size(), embedding.size()); ++j) {
- Line 46: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(static_cast<float>(*intVal));
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(*intVal));
- Line 53: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(*doubleVal));
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(hash % 10000) / 10000.0f);
- Line 108: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 4) result.entity_id += ":";
- Line 125: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_level.push_back(std::string(node_pk));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_level.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_level.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbor_features_list.push_back(neighbor_features);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raw_similarities.push_back(similarity);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raw_similarities.push_back(similarity);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attention_weights.push_back(weight);
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edge_ids.push_back(edge.edgeId);
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edge_ids.push_back(edge.edgeId);
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_embeddings.push_back(*embOpt);
- Line 729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.push_back(simRes);
  Confidence: band=high; score=0.74
- Line 770: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.push_back(simRes);
  Confidence: band=high; score=0.74
- Line 801: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/index/advanced_vector_index.cpp
Total findings: 27

- Line 42: severity=CRITICAL; category=missing_dtor
  Description: Class Index allocates resources but has no destructor
  Remediation: Add explicit destructor: ~Index() { /* cleanup */ }
  Context: class/struct Index
- Line 43: severity=CRITICAL; category=missing_dtor
  Description: Class IndexIVFPQ allocates resources but has no destructor
  Remediation: Add explicit destructor: ~IndexIVFPQ() { /* cleanup */ }
  Context: class/struct IndexIVFPQ
- Line 44: severity=CRITICAL; category=missing_dtor
  Description: Class IndexIVFFlat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~IndexIVFFlat() { /* cleanup */ }
  Context: class/struct IndexIVFFlat
- Line 78: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
- Line 79: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* ivf_pq = new faiss::IndexIVFPQ(
- Line 88: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_pq->nprobe = config_.nprobe;
- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_pq->polysemous_ht = config_.polysemous_ht;
- Line 110: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
- Line 111: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* ivf_flat = new faiss::IndexIVFFlat(
- Line 118: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_flat->nprobe = config_.nprobe;
- Line 126: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* hnsw = new faiss::IndexHNSWFlat(static_cast<int>(dimension_), 32);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 63: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 87: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: the = nullptr;
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 117: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: the = nullptr;
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 501: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 87: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 117: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 215: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 260: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 304: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 348: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 409: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 449: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 527: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/distributed_vector_index.cpp
Total findings: 27

- Line 124: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: const std::string token = "shard:" + std::to_string(s) + ":vn:" + std::to_string(v);
- Line 185: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
  Confidence: band=very_high; score=0.99
- Line 235: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
  Confidence: band=very_high; score=0.99
- Line 237: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return insert(pk, vec.data(), vec.size());
  Confidence: band=very_high; score=0.99
- Line 267: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<AnnSearchResult> merged;
  Confidence: band=very_high; score=0.99
- Line 279: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(r);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3136 [index] Wire distributed ve... (2026-03-12) | #3034 feat(index): Distri
- Line 267: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<AnnSearchResult> merged;
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = local_to_global_id_[s].find(r.id);
- Line 275: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = local_to_global_id_[s].find(r.id);
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(r);
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: sort by distance (ascending) and keep top-k.
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (static_cast<int>(merged.size()) > k) {
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(static_cast<size_t>(k));
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_.push_back(std::make_unique<ScaNN>());
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 228: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Rollback: remove the stale routing entry so the key is not
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto partial = shards_[s]->search(query, dim, k);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto it = local_to_global_id_[s].find(r.id);
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(r);
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(r);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.push_back({i, alive_ids_[i].size()});
  Confidence: band=high; score=0.74

### src/index/ann_index.cpp
Total findings: 25

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3136 [index] Wire distributed ve... (2026-03-12) | #2946 feat(index): DiskAN
- Line 96: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2sq(data + i * d, centroids[c].data(), d);
- Line 109: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_cents[c][j] += data[i * d + j];
- Line 129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2sq(data + i * d, centroids[c].data(), d);
- Line 174: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = ScaNN::l2sq(sv, centroids[s][c].data(), sub_dim);
- Line 187: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const float* sc = centroids[s][code[s]].data();
- Line 378: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: sizeof(int64_t) * n);
- Line 404: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: sizeof(int64_t) * n);
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids_.push_back(id);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids_.push_back(id);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[best_leaf].ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({dist, &leaf, i});
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({dist, &leaf, i});
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({c.leaf->ids[c.idx], exact});
  Confidence: band=high; score=0.74

### src/index/index_compression.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 200: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Need to rebuild existing suffixes with new (shorter) prefix
- Line 221: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start a new block
- Line 100: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Context: void BloomFilter::clear() {
  Confidence: band=medium; score=0.56
- Line 114: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> freq;
  Confidence: band=medium; score=0.66
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_string_.push_back(std::move(candidates[i].second));
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prefix + sfx);
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: single.suffixes.push_back(current.prefix + current.suffixes[0]);
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prev);
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.runs.push_back({values[0], 1});
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: block.runs.push_back({values[i], 1});
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.runs.push_back({values[i], 1});
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(run.value);
- Line 423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.suffixes.push_back(k);
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trivial.runs.push_back({v, 1});
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trivial.runs.push_back({v, 1});
- Line 61: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ln2   = std::log(2.0);
  Confidence: band=medium; score=0.6

### src/index/multi_vector_search.cpp
Total findings: 23

- Line 391: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator kw_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto kw_it = keyword_scores.find(doc_id);
- Line 99: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // 1. Validate inputs
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find_if(results.begin(), results.end(),
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(results.begin(), results.end(),
- Line 195: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(results.begin(), results.end(),
- Line 376: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto vec_it = std::find_if(vector_results.begin(), vector_results.end(),
- Line 558: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
- Line 558: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back((score - min_score) / range);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "MultiVectorSearch::search - vector search failed: " + status.message);
- Line 168: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(std::numeric_limits<int>::max());  // Worst rank
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: multi_query.vectors.push_back(query_vector);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, float>& keyword_scores,
  Confidence: band=medium; score=0.66
- Line 350: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "MultiVectorSearch::hybridSearch - vector search failed: " + status.message);
- Line 354: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(std::numeric_limits<int>::max());
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result.value()));
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_weights.push_back(w);
  Confidence: band=high; score=0.74

### src/index/vector_auto_buffer.cpp
Total findings: 19

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 124: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 247: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 310: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: status = vectorIndex_->addBatch(compressed_adds, config_.vector_field);
- Line 319: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: status = vectorIndex_->updateBatch(compressed_updates, config_.vector_field);
- Line 128: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[buffer_key];
- Line 384: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 594: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: quantised[dim] = abs_max; // scale metadata for downstream decoders
- Line 36: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adds.push_back(op.entity);
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adds.push_back(op.entity);
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: updates.push_back(op.entity);
- Line 298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: removes.push_back(op.pk);
- Line 306: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: VectorIndexManager::Status status;
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: training_vecs.push_back(*vec_opt);
  Confidence: band=high; score=0.74
- Line 533: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(compressed));
  Confidence: band=high; score=0.74

### src/index/multi_gpu_vector_index.cpp
Total findings: 18

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 96: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "Warning: Failed to initialize GPU " << deviceId
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failedDeviceIds.push_back(deviceId);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "Error: Failed to initialize GPU " << deviceId << "\n";
- Line 272: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto gpuResults = gpuIndices[gpuIdx]->search(query, k);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto res = gpuIndices[gpuIdx]->searchBatch(queries, k);
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: perGpuResults.push_back(f.get());
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto gpuStats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorsPerGPU.push_back(stats.numVectors);
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorsPerGPU.push_back(stats.numVectors);
- Line 596: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  Partition " << i << " (Device " << activeDeviceIds[i]

### src/index/gpu_vector_index_vulkan.cpp
Total findings: 17

- Line 477: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pipeline->wait();
- Line 642: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pipeline->wait();
- Line 146: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.first == b.first &&
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.middle == b.middle &&
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.last == b.last;
  Confidence: band=very_high; score=0.9
- Line 707: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (avg_query_time_ms_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: top.emplace_back(distances[i], i);
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "VulkanVectorIndexBackend: Vector dimension mismatch\n";
- Line 533: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: converted.push_back({"", distance});
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "VulkanVectorIndexBackend: Query dimension mismatch in batch\n";
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74
- Line 750: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "  - " << path << "\n";
- Line 943: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 960: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(vectors); } catch (...) { return false; }
- Line 978: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(query, k); } catch (...) { return {}; }
- Line 991: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(queries, k); } catch (...) { return {}; }

### src/index/product_quantizer.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #1005 [REFACTOR] Quantizer analys... (2026-03-11) | #1072 Add Vector Indexing
- Line 310: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: size_t compressed_size = config_.num_subquantizers * sizeof(uint8_t);
- Line 370: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: centroid[d] = centroid_data[i * subvector_dim_ + d];
- Line 407: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2Distance(subvector_data[j], centroid);
- Line 415: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
- Line 428: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2Distance(subvector_data[i], centroids[j]);
- Line 447: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_centroids[cluster][d] += subvector_data[i][d];
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subvector_data.push_back(std::move(subvec));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(code);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(std::move(centroid));
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(std::move(centroid));
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);

### src/index/property_graph.cpp
Total findings: 11

- Line 1246: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Compute new PageRank scores
- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back(std::move(label));
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 729: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PropertyGraphManager::federatedQuery(const std::vector<FederationPattern>& patterns) const {
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
- Line 1211: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> outgoing_count;
  Confidence: band=medium; score=0.66
- Line 1212: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> incoming_nodes;
  Confidence: band=medium; score=0.66
- Line 1227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: incoming_nodes[to_node].push_back(node);
  Confidence: band=high; score=0.74
- Line 1227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: incoming_nodes[to_node].push_back(node);
  Confidence: band=high; score=0.74

### src/index/gpu_memory_oversubscription.cpp
Total findings: 10

- Line 42: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.99
- Line 219: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map.find(partition_id);
- Line 42: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: .allocate(bytes, alloc_tag);
- Line 512: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << p.id << " (" << (bytes / 1024) << " KiB required)\n";
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74

### src/index/index_manager.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4587 feat(index): add IndexManag... (2026-04-13) | #3174 [index] Implement S
- Line 385: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: secondary_indices_[name_str] = raw_ptr;
- Line 447: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vector_indices_[name_str] = raw_ptr;
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(r.pk, r.distance);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 548: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status drop_status;
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indices.push_back(name);
  Confidence: band=high; score=0.74
- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_drop.push_back(key);
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(key.substr(prefix.size()));
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(key.substr(prefix.size()));

### src/index/graph_auto_buffer.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 104: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 160: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 214: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 109: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[gid];
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[gid];
- Line 328: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 26: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/learnable_rope.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 509: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 522: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 531: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/hnsw_parameter_tuner.cpp
Total findings: 7

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
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 432: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadClassifier::recordQuery(size_t k) {
  Confidence: band=high; score=0.74

### src/index/learned_quantizer.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: codes.push_back(static_cast<uint8_t>(bin));
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector.push_back(per_dim_centroids_[d][bin]);
  Confidence: band=high; score=0.74

### src/index/residual_quantizer.cpp
Total findings: 7

- Line 73: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: " training failed: " + status.message);
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: residuals.push_back(std::move(residual));
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: residuals.push_back(std::move(residual));
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_codes = stage_quantizers_[stage]->encode(residual);
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74

### src/index/rotary_embeddings_hip.cpp
Total findings: 6

- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: hipError_t err = hipMalloc(&gpu_resources_->d_theta_cache, cache_size);
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_resources_->d_theta_cache = nullptr;
- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_resources_->theta_cache_size = theta_cache.size();
- Line 95: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t allocated_batch_size = 0;
- Line 212: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_resources_->allocated_batch_size < batch_size) {
- Line 221: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_resources_->allocated_batch_size = batch_size;

### src/index/workload_replay.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2517 [index] Automated index adv... (2026-03-11)
- Line 83: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadCapture::recordQuery() {
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capture.events_.push_back(WorkloadEvent::fromJSON(ej));
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: capture.events_.push_back(WorkloadEvent::fromJSON(ej));

### src/index/approximate_radius_search.cpp
Total findings: 5

- Line 48: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 261: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 86: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Radius search failed: " + status.message);
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: search_result.results.push_back(std::move(rr));
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_results.push_back(std::move(result.value()));
  Confidence: band=high; score=0.74

### src/index/rotary_embeddings.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 259: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (norm_squared == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: theta_cache.push_back(theta);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rotated_batch.push_back(rotate(embeddings[i], positions[i]));
  Confidence: band=high; score=0.74

### src/index/tiered_index_manager.cpp
Total findings: 4

- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.tier == tier) names.push_back(k);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.tier == tier) names.push_back(k);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
  Confidence: band=high; score=0.74

### src/index/adaptive_index.cpp
Total findings: 2

- Line 537: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_aware_stats = analyzer_->analyzeCacheAware(stats);
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pattern);
  Confidence: band=high; score=0.74

### src/index/edge_types.cpp
Total findings: 2

- Line 340: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = category_index_.find(category);
- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(name);
  Confidence: band=high; score=0.74

### src/index/hnsw_layer_optimizer.cpp
Total findings: 2

- Line 73: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::pair<double, int>> entry_layer_performance;  // layer -> (total_time, count)
  Confidence: band=medium; score=0.66
- Line 108: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::pair<double, int>> ef_performance;  // ef -> (total_time, count)
  Confidence: band=medium; score=0.66

### src/index/lora_rope.cpp
Total findings: 2

- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(rotateWithAdapter(embeddings[i], positions[i], adapter_name));
  Confidence: band=high; score=0.74

### src/index/rotary_embeddings_gpu_cpu.cpp
Total findings: 2

- Line 61: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 74: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(

### src/index/hnsw_production_defaults.cpp
Total findings: 1

- Line 103: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: params.ml = 1.0 / std::log(static_cast<double>(params.M));
  Confidence: band=medium; score=0.6

### src/index/temporal_graph.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3114 [graph] Add time-range trav... (2026-03-12)

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
