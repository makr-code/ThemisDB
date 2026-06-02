# tensor Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: tensor
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 42
- Actionable Findings (Critical + High): 2
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 2 |
| Medium | 40 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 37 |
| container | 19 |
| reliability | 17 |
| exception_safety | 13 |
| memory | 10 |
| raii | 9 |
| audit_logging | 4 |
| determinism | 4 |
| legacy_duplication | 4 |
| type_conversion | 4 |
| input_validation | 3 |
| platform | 3 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/tensor/hyper_index_builder.cpp | 9 | 0 | 0 | 9 | 0 |
| src/tensor/hiss_structural_search.cpp | 8 | 0 | 0 | 8 | 0 |
| src/tensor/tensor_index_manager.cpp | 8 | 0 | 0 | 8 | 0 |
| src/tensor/ht_index.cpp | 4 | 0 | 0 | 4 | 0 |
| src/tensor/utr_converter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/tensor/hnsw_tt_bridge.cpp | 3 | 0 | 1 | 2 | 0 |
| src/tensor/adapter_repository.cpp | 2 | 0 | 0 | 2 | 0 |
| src/tensor/tensor_butterfly_operator.cpp | 2 | 0 | 1 | 1 | 0 |
| src/tensor/tensor_index.cpp | 1 | 0 | 0 | 1 | 0 |
| src/tensor/tensor_mmap_bridge.cpp | 1 | 0 | 0 | 1 | 0 |
| src/tensor/tensor_core_bridge.cpp | 0 | 0 | 0 | 0 | 0 |
| src/tensor/tensor_fingerprint_graph.cpp | 0 | 0 | 0 | 0 | 0 |
| src/tensor/tnsr_task.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/tensor/hyper_index_builder.cpp
Total findings: 9

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buckets.push_back(numericBucket(row.numeric_values[num_col],
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(row.numeric_values[numeric_index]);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(values[idx]);
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> frequencies;
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(value);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(node);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(node);
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::size_t> visited;
  Confidence: band=medium; score=0.66
- Line 473: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto propagated_bucket = signal_bucket_sum[k] / signal_weight[k];
  Confidence: band=high; score=0.74

### src/tensor/hiss_structural_search.cpp
Total findings: 8

- Line 216: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto bd = bit_depths[d];
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto bd = bit_depths[d];
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (e.from == node_index) out.push_back(e.to);
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (e.from == node_index) out.push_back(e.to);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::uint64_t, TensorGraphEdge> best_by_edge;
  Confidence: band=medium; score=0.66
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bit_depths.push_back(bit_depth);
  Confidence: band=high; score=0.74

### src/tensor/tensor_index_manager.cpp
Total findings: 8

- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_del.emplace_back(k);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_del.emplace_back(k);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (k.substr(0, prefix.size()) == prefix) out.push_back(h);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (k.substr(0, prefix.size()) == prefix) out.push_back(h);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(k, idx.get());
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ptrs.emplace_back(slice.data, slice.bytes);
  Confidence: band=high; score=0.74

### src/tensor/ht_index.cpp
Total findings: 4

- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_.push_back({id, std::move(train)});
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({e.id, sim});
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: void appendU8(std::vector<uint8_t>& buf, uint8_t v) { buf.push_back(v); }
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_.push_back({std::move(id), std::move(*ht)});
  Confidence: band=high; score=0.74

### src/tensor/utr_converter.cpp
Total findings: 4

- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!seg.empty()) segments.push_back(seg);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patch_features.push_back(static_cast<float>(mean));
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patch_features.push_back(static_cast<float>(mean));
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patch_features.push_back(static_cast<float>(mean));
  Confidence: band=high; score=0.74

### src/tensor/hnsw_tt_bridge.cpp
Total findings: 3

- Line 616: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d == 0 || d != B.cores.size()) return 0.0f;
  Confidence: band=very_high; score=0.9
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(dist_ids[i].second);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({ cid, 1.0f - sim, tn });
  Confidence: band=high; score=0.74

### src/tensor/adapter_repository.cpp
Total findings: 2

- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domains.push_back(rest.substr(0, sep));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(rest.substr(0, sep), rest.substr(sep + 1));
  Confidence: band=high; score=0.74

### src/tensor/tensor_butterfly_operator.cpp
Total findings: 2

- Line 318: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate shape compatibility
  Confidence: band=high; score=0.8
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: op_matrices_.push_back(buildHadamardMatrix(nk));
  Confidence: band=high; score=0.74

### src/tensor/tensor_index.cpp
Total findings: 1

- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({ id, dist, t_n });
  Confidence: band=high; score=0.74

### src/tensor/tensor_mmap_bridge.cpp
Total findings: 1

- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bridge->slices_.push_back({nullptr, 0, ci, 0});
  Confidence: band=high; score=0.74

### src/tensor/tensor_core_bridge.cpp
Total findings: 0


### src/tensor/tensor_fingerprint_graph.cpp
Total findings: 0


### src/tensor/tnsr_task.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
