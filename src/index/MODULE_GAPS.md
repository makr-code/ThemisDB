# index Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: index
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 476
- Actionable Findings (Critical + High): 95
- Affected Files: 39

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 20 |
| High | 75 |
| Medium | 377 |
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
| src/index/secondary_index.cpp | 99 | 0 | 6 | 92 | 1 |
| src/index/cuda_hnsw_graph_traversal.cpp | 45 | 8 | 32 | 5 | 0 |
| src/index/process_graph.cpp | 41 | 0 | 0 | 41 | 0 |
| src/index/vector_index.cpp | 29 | 0 | 1 | 28 | 0 |
| src/index/inverted_index.cpp | 25 | 0 | 2 | 22 | 1 |
| src/index/graph_analytics.cpp | 24 | 0 | 3 | 21 | 0 |
| src/index/distributed_vector_index.cpp | 20 | 5 | 8 | 7 | 0 |
| src/index/gnn_embeddings.cpp | 20 | 6 | 1 | 13 | 0 |
| src/index/spatial_index.cpp | 18 | 0 | 8 | 10 | 0 |
| src/index/gpu_vector_index.cpp | 16 | 0 | 1 | 15 | 0 |
| src/index/graph_index.cpp | 15 | 0 | 2 | 13 | 0 |
| src/index/ann_index.cpp | 14 | 0 | 0 | 14 | 0 |
| src/index/index_compression.cpp | 13 | 0 | 0 | 12 | 1 |
| src/index/multi_gpu_vector_index.cpp | 13 | 0 | 0 | 13 | 0 |
| src/index/multi_vector_search.cpp | 13 | 0 | 3 | 10 | 0 |
| src/index/gpu_vector_index_vulkan.cpp | 8 | 0 | 4 | 4 | 0 |
| src/index/property_graph.cpp | 8 | 0 | 0 | 8 | 0 |
| src/index/product_quantizer.cpp | 7 | 0 | 0 | 7 | 0 |
| src/index/residual_quantizer.cpp | 6 | 0 | 0 | 6 | 0 |
| src/index/vector_auto_buffer.cpp | 6 | 0 | 0 | 6 | 0 |
| src/index/approximate_radius_search.cpp | 4 | 0 | 2 | 2 | 0 |
| src/index/gpu_memory_oversubscription.cpp | 4 | 1 | 1 | 2 | 0 |
| src/index/index_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/index/learned_quantizer.cpp | 4 | 0 | 0 | 4 | 0 |
| src/index/tiered_index_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/index/rotary_embeddings.cpp | 3 | 0 | 1 | 2 | 0 |
| src/index/workload_replay.cpp | 3 | 0 | 0 | 3 | 0 |
| src/index/hnsw_layer_optimizer.cpp | 2 | 0 | 0 | 2 | 0 |
| src/index/learnable_rope.cpp | 2 | 0 | 0 | 2 | 0 |
| src/index/lora_rope.cpp | 2 | 0 | 0 | 2 | 0 |
| src/index/adaptive_index.cpp | 1 | 0 | 0 | 1 | 0 |
| src/index/edge_types.cpp | 1 | 0 | 0 | 1 | 0 |
| src/index/hnsw_parameter_tuner.cpp | 1 | 0 | 0 | 1 | 0 |
| src/index/hnsw_production_defaults.cpp | 1 | 0 | 0 | 0 | 1 |
| src/index/advanced_vector_index.cpp | 0 | 0 | 0 | 0 | 0 |
| src/index/graph_auto_buffer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/index/rotary_embeddings_gpu_cpu.cpp | 0 | 0 | 0 | 0 | 0 |
| src/index/rotary_embeddings_hip.cpp | 0 | 0 | 0 | 0 | 0 |
| src/index/temporal_graph.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/index/secondary_index.cpp
Total findings: 99

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
- Line 2593: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
  Confidence: band=very_high; score=0.9
- Line 2652: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto itLen = docLen.find(pk);
  Confidence: band=very_high; score=0.9
- Line 2697: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Public API: returns PKs only (deprecated, use scanFulltextWithScores for scores)
  Confidence: band=high; score=0.8
- Line 147: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idxmeta:";
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
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
- Line 190: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
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
- Line 224: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
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
- Line 273: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
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
- Line 547: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 762: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.emplace_back(s.get<std::string>());
  Confidence: band=high; score=0.74
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
- Line 1900: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 1987: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
- Line 2026: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 2038: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.emplace_back(rest);
  Confidence: band=high; score=0.74
- Line 2061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
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
- Line 3292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybeVal);
  Confidence: band=high; score=0.74
- Line 3546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*mv);
  Confidence: band=high; score=0.74
- Line 3637: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
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
- Line 4103: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4128: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sparseCols;
  Confidence: band=medium; score=0.66
- Line 4174: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> geoCols;
  Confidence: band=medium; score=0.66
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
- Line 4589: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 2659: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
  Confidence: band=medium; score=0.6

### src/index/cuda_hnsw_graph_traversal.cpp
Total findings: 45

- Line 347: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.99
- Line 355: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
  Confidence: band=very_high; score=0.99
- Line 366: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
  Confidence: band=very_high; score=0.99
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
- Line 522: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ids.data(), impl_->d_result_ids,
  Confidence: band=very_high; score=0.9
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

### src/index/process_graph.cpp
Total findings: 41

- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto val = variables[expr];
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto leftVal = variables[left];
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::validateProcess(std::string_view process_id) const {
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Process has no start event");
  Confidence: band=high; score=0.74
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
- Line 692: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_node + "'");
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
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
- Line 1081: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token->traversed_edges.push_back(edge.edge_id);
  Confidence: band=high; score=0.74
- Line 1141: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::suspendProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::resumeProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
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
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
  Confidence: band=high; score=0.74
- Line 1781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.source_nodes.push_back(src.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1795: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.target_nodes.push_back(tgt.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1921: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 1985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(jr));
  Confidence: band=high; score=0.74
- Line 2141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_number()) emb.push_back(v.get<float>());
  Confidence: band=high; score=0.74
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
- Line 2862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 2862: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 3097: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(mmr));
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(edge.to_node);
  Confidence: band=high; score=0.74

### src/index/vector_index.cpp
Total findings: 29

- Line 2294: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Original plaintext load (backward compatibility)
  Confidence: band=high; score=0.8
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
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_delete.push_back(pk);
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 1358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_ptrs.push_back(&entry);
  Confidence: band=high; score=0.74
- Line 1543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(r);
  Confidence: band=high; score=0.74
- Line 1576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({idToPk_[idx], r.distance});
  Confidence: band=high; score=0.74
- Line 1760: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
  Confidence: band=high; score=0.74
- Line 1769: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
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
- Line 1944: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(c);
  Confidence: band=high; score=0.74
- Line 1961: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 1985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 2024: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
- Line 2056: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> inResults;
  Confidence: band=medium; score=0.66
- Line 2095: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
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

### src/index/inverted_index.cpp
Total findings: 25

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
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
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

### src/index/graph_analytics.cpp
Total findings: 24

- Line 145: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto out_it = topo.outgoing.find(pk);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = new_ranks.find(neighbor);
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (m == 0.0) m = 1.0;  // Avoid division by zero
  Confidence: band=very_high; score=0.9
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
- Line 733: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: total_path.edges.push_back(edge);
  Confidence: band=high; score=0.74

### src/index/distributed_vector_index.cpp
Total findings: 20

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
- Line 267: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<AnnSearchResult> merged;
  Confidence: band=very_high; score=0.9
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

### src/index/gnn_embeddings.cpp
Total findings: 20

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
- Line 635: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 669: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 141: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (visited.find(neighbor) == visited.end()) {
  Confidence: band=very_high; score=0.9
- Line 46: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(static_cast<float>(*intVal));
  Confidence: band=high; score=0.74
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

### src/index/spatial_index.cpp
Total findings: 18

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
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
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
- Line 963: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kvs.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
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
- Line 1298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cand));
  Confidence: band=high; score=0.74

### src/index/gpu_vector_index.cpp
Total findings: 16

- Line 1380: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Stored for future compatibility; callers set the metric via Config
  Confidence: band=high; score=0.8
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(dist, globalOffset + vi);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], candidates[i].first});
  Confidence: band=high; score=0.74
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
- Line 1584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends.push_back(Backend::CPU);
  Confidence: band=high; score=0.74

### src/index/graph_index.cpp
Total findings: 15

- Line 189: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 1304: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
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
- Line 968: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbors.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 1058: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
- Line 1419: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1994: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(vertex);
  Confidence: band=high; score=0.74

### src/index/ann_index.cpp
Total findings: 14

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
Total findings: 13

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
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prev);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: block.runs.push_back({values[i], 1});
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.suffixes.push_back(k);
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trivial.runs.push_back({v, 1});
  Confidence: band=high; score=0.74
- Line 61: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ln2   = std::log(2.0);
  Confidence: band=medium; score=0.6

### src/index/multi_gpu_vector_index.cpp
Total findings: 13

- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failedDeviceIds.push_back(deviceId);
  Confidence: band=high; score=0.74
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

### src/index/multi_vector_search.cpp
Total findings: 13

- Line 99: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // 1. Validate inputs
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find_if(results.begin(), results.end(),
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
  Confidence: band=very_high; score=0.9
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back((score - min_score) / range);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(score);
  Confidence: band=high; score=0.74
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
- Line 354: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
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

### src/index/gpu_vector_index_vulkan.cpp
Total findings: 8

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
- Line 533: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: converted.push_back({"", distance});
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74

### src/index/property_graph.cpp
Total findings: 8

- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back(std::move(label));
  Confidence: band=high; score=0.74
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

### src/index/product_quantizer.cpp
Total findings: 7

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

### src/index/residual_quantizer.cpp
Total findings: 6

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

### src/index/vector_auto_buffer.cpp
Total findings: 6

- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adds.push_back(op.entity);
  Confidence: band=high; score=0.74
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

### src/index/approximate_radius_search.cpp
Total findings: 4

- Line 48: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 261: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: search_result.results.push_back(std::move(rr));
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_results.push_back(std::move(result.value()));
  Confidence: band=high; score=0.74

### src/index/gpu_memory_oversubscription.cpp
Total findings: 4

- Line 42: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.99
- Line 42: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.9
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74

### src/index/index_manager.cpp
Total findings: 4

- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(r.pk, r.distance);
  Confidence: band=high; score=0.74
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

### src/index/learned_quantizer.cpp
Total findings: 4

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
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector.push_back(per_dim_centroids_[d][bin]);
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

### src/index/rotary_embeddings.cpp
Total findings: 3

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

### src/index/workload_replay.cpp
Total findings: 3

- Line 83: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadCapture::recordQuery() {
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capture.events_.push_back(WorkloadEvent::fromJSON(ej));
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

### src/index/learnable_rope.cpp
Total findings: 2

- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74

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

### src/index/adaptive_index.cpp
Total findings: 1

- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pattern);
  Confidence: band=high; score=0.74

### src/index/edge_types.cpp
Total findings: 1

- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(name);
  Confidence: band=high; score=0.74

### src/index/hnsw_parameter_tuner.cpp
Total findings: 1

- Line 432: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadClassifier::recordQuery(size_t k) {
  Confidence: band=high; score=0.74

### src/index/hnsw_production_defaults.cpp
Total findings: 1

- Line 103: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: params.ml = 1.0 / std::log(static_cast<double>(params.M));
  Confidence: band=medium; score=0.6

### src/index/advanced_vector_index.cpp
Total findings: 0


### src/index/graph_auto_buffer.cpp
Total findings: 0


### src/index/rotary_embeddings_gpu_cpu.cpp
Total findings: 0


### src/index/rotary_embeddings_hip.cpp
Total findings: 0


### src/index/temporal_graph.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
