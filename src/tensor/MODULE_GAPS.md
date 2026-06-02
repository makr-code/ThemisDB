# tensor Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: tensor
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 121
- Actionable Findings (Critical + High): 48
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 8 |
| High | 40 |
| Medium | 73 |
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
| src/tensor/hnsw_tt_bridge.cpp | 20 | 4 | 9 | 7 | 0 |
| src/tensor/hyper_index_builder.cpp | 20 | 0 | 6 | 14 | 0 |
| src/tensor/hiss_structural_search.cpp | 17 | 0 | 6 | 11 | 0 |
| src/tensor/tensor_index_manager.cpp | 13 | 1 | 1 | 11 | 0 |
| src/tensor/adapter_repository.cpp | 8 | 1 | 3 | 4 | 0 |
| src/tensor/tensor_index.cpp | 8 | 0 | 4 | 4 | 0 |
| src/tensor/tensor_mmap_bridge.cpp | 8 | 0 | 2 | 6 | 0 |
| src/tensor/utr_converter.cpp | 8 | 1 | 0 | 7 | 0 |
| src/tensor/tnsr_task.cpp | 7 | 0 | 4 | 3 | 0 |
| src/tensor/tensor_butterfly_operator.cpp | 5 | 0 | 4 | 1 | 0 |
| src/tensor/ht_index.cpp | 4 | 0 | 0 | 4 | 0 |
| src/tensor/tensor_core_bridge.cpp | 2 | 1 | 0 | 1 | 0 |
| src/tensor/tensor_fingerprint_graph.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/tensor/hnsw_tt_bridge.cpp
Total findings: 20

- Line 65: severity=CRITICAL; category=missing_dtor
  Description: Class HnswTTBridge allocates resources but has no destructor
  Remediation: Add explicit destructor: ~HnswTTBridge() { /* cleanup */ }
  Context: class/struct HnswTTBridge
- Line 90: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* sp = new hnswlib::L2Space(dim);
- Line 92: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: appr_ = new hnswlib::HierarchicalNSW<float>(
- Line 189: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = label_to_id_.find(label);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 95: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: sp = nullptr;
  Context: delete sp;
- Line 116: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: appr_ = nullptr;
  Context: delete appr_;
- Line 117: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space_ = nullptr;
  Context: delete space_;
- Line 298: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [train, stats] = decomposer.decompose(data, shape, cfg);
- Line 616: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (d == 0 || d != B.cores.size()) return 0.0f;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 94: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 159: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(dist_ids[i].second);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({ cid, 1.0f - sim, tn });
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/tensor/hyper_index_builder.cpp
Total findings: 20

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            }', '', '            auto& out = category_orders[category_index];', '            if (category_schema) {', '                out = category_schema->categories;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                      return lhs.first < rhs.first;', '                  });', '        auto& out = category_orders[category_index];', '        out.reserve(ordered.size());', '        for (const auto& [value, _] : ordered) {']
  Confidence: band=high; score=0.78
- Line 288: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @return Bucket index in [0, bucket_count-1].
- Line 304: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: double weight;      ///< Validated join strength in [0,1]
- Line 375: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * deeper hops apply `propagation_decay` (clamped to [0,1]).
- Line 579: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error(
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buckets.push_back(numericBucket(row.numeric_values[num_col],
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(numeric_schema->range_min + (span * (bucket_d / bucket_count_d)));
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(row.numeric_values[numeric_index]);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(values[idx]);
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(values[idx]);
- Line 258: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> frequencies;
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(value);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolved.push_back({edge.from_column, edge.to_column, weight});
- Line 402: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adjacency[edge.from].push_back({edge.to, edge.weight});
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
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    for (std::size_t physical_idx = 0; physical_idx < dense_tensor.size(); ++physical_idx) {', '        const auto qtt_idx = dense_to_qtt.physicalToQTT(physical_idx);', '        padded_dense_tensor[qtt_idx] = dense_tensor[physical_idx];', '    }', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 74: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return h / std::log2(static_cast<double>(kBins)); // normalized [0,1]
- Line 92: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("shape product overflow");
- Line 105: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("buildExactBinaryTT bit_count too large: " +
- Line 162: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("grid_size " + std::to_string(grid_size) +
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            --bit_pos;', '            const auto bit = (multi_idx[d] >> (bd - 1u - b)) & 1ULL;', '            qtt_idx |= bit << bit_pos;', '        }', '    }']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        for (std::size_t b = 0; b < bd; ++b) {', '            --bit_pos;', '            const auto bit = (qtt_idx >> bit_pos) & 1ULL;', '            idx_d = (idx_d << 1u) | bit;', '        }']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            --bit_pos;', '            const auto bit = (qtt_idx >> bit_pos) & 1ULL;', '            idx_d = (idx_d << 1u) | bit;', '        }', '        if (idx_d >= grid_sizes[d]) {']
  Confidence: band=medium; score=0.65
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
Total findings: 13

- Line 39: severity=CRITICAL; category=missing_dtor
  Description: Class FlatTensorIndex allocates resources but has no destructor
  Remediation: Add explicit destructor: ~FlatTensorIndex() { /* cleanup */ }
  Context: class/struct FlatTensorIndex
- Line 151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != indexes_.end()) ? it->second.get() : nullptr;
- Line 16: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: * - TIM-02  `dropTenantIndexes()` RocksDB prefix-delete — resolved 2026-05-06
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
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(k, idx.get());
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ptrs.emplace_back(slice.data, slice.bytes);
  Confidence: band=high; score=0.74

### src/tensor/adapter_repository.cpp
Total findings: 8

- Line 288: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator sep may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto sep = rest.find(':');
- Line 28: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: * Production Delta: GgmlCoreDescriptor::train.cores[k].data is a heap copy,
- Line 287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto sep = rest.find(':');
- Line 313: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto sep = rest.find(':');
- Line 236: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: domains.push_back(rest.substr(0, sep));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(rest.substr(0, sep), rest.substr(sep + 1));
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/tensor/tensor_index.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 110: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({ id, dist, t_n });
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 230: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static constexpr char kMagic[11] = "THEMIS_TTI";  // 10 chars + '\0'

### src/tensor/tensor_mmap_bridge.cpp
Total findings: 8

- Line 24: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * heap-allocated copy guarded by `VirtualLock` (Windows) or a no-lock
- Line 104: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Check: if (ptr != nullptr) before dereferencing
  Context: return std::malloc(bytes);
- Line 140: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(ptr);
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bridge->slices_.push_back({nullptr, 0, ci, 0});
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bridge->slices_.push_back({nullptr, 0, ci, 0});
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bridge->regions_.push_back({ptr, bytes, locked, sst_mapped});
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bridge->slices_.push_back({
- Line 276: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();

### src/tensor/utr_converter.cpp
Total findings: 8

- Line 186: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto pos = text.find("\n\n", start);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    for (std::size_t lane = 0; lane < 8 && lane < embed_dim; ++lane) {', '        const auto shift = (lane % 4) * 16U;', '        const auto bits  = static_cast<uint16_t>((h >> shift) & 0xFFFFULL);', '        const auto dim   = static_cast<std::size_t>((bits + lane * 131U) % embed_dim);', '        const auto sign  = ((h >> (shift + 7U)) & 1ULL) ? 1.0f : -1.0f;']
  Confidence: band=medium; score=0.62
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
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patch_features.push_back(static_cast<float>(mean));
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patch_features.push_back(static_cast<float>(std::sqrt(variance)));

### src/tensor/tnsr_task.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (cancel_requested_.load(std::memory_order_acquire)) {
- Line 89: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 108: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 149: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/tensor/tensor_butterfly_operator.cpp
Total findings: 5

- Line 107: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (std::size_t i = 0; i < n; ++i) data[i] *= inv_sqrt_n;
- Line 277: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data[al * n_k * r_right + i * r_right + ar] = fiber[i];
- Line 318: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate shape compatibility
  Confidence: band=high; score=0.8
- Line 360: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fiber[i] = core.data[al * n_k * r_right + i * r_right + ar];
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: op_matrices_.push_back(buildHadamardMatrix(nk));
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

### src/tensor/tensor_core_bridge.cpp
Total findings: 2

- Line 120: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> TensorCoreStorageBridge::write(
- Line 180: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/tensor/tensor_fingerprint_graph.cpp
Total findings: 1

- Line 252: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it_train = trains_.find(key);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
