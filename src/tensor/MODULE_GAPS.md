# tensor Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: tensor
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 87
- Actionable Findings (Critical + High): 62
- Affected Files: 15

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 9 |
| High | 53 |
| Medium | 19 |
| Low | 6 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 12 |
| pointer_arithmetic_unbounded | 8 |
| explicit_delete | 4 |
| hardcoded_output | 4 |
| legacy_or_compat_path | 4 |
| shift_overflow | 4 |
| uncaught_exception | 4 |
| uninitialized_access | 4 |
| delete_no_nullptr | 3 |
| delete_without_nullptr | 3 |
| iterator_invalidation | 3 |
| manual_cleanup | 3 |
| o_n_squared | 3 |
| unchecked_array_index | 3 |
| unnecessary_copy | 3 |
| unordered_container_iter | 3 |
| db_connection_leak | 2 |
| hardcoded_path | 2 |
| missing_dtor | 2 |
| module_doc_linkset_drift | 2 |
| smart_ptr_misuse | 2 |
| fp_exact_comparison | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_move_constructor_defaulted | 1 |
| missing_vector_reserve | 1 |
| no_timeout | 1 |
| string_concat_loop | 1 |
| unchecked_malloc | 1 |
| uninitialized_member_field | 1 |
| unwrapped_resource | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| tensor/hnsw_tt_bridge.cpp | 21 | 5 | 15 | 1 | 0 |
| tensor/hiss_structural_search.cpp | 10 | 0 | 4 | 6 | 0 |
| tensor/hyper_index_builder.cpp | 10 | 0 | 6 | 4 | 0 |
| tensor/tensor_index_manager.cpp | 8 | 1 | 4 | 3 | 0 |
| tensor/tensor_ingestion_bridge.cpp | 8 | 0 | 8 | 0 | 0 |
| tensor/adapter_repository.cpp | 7 | 1 | 2 | 0 | 4 |
| tensor/tensor_index.cpp | 5 | 0 | 4 | 1 | 0 |
| tensor/utr_converter.cpp | 5 | 1 | 2 | 2 | 0 |
| tensor/tensor_mmap_bridge.cpp | 4 | 0 | 2 | 2 | 0 |
| tensor/tnsr_task.cpp | 4 | 0 | 4 | 0 | 0 |
| tensor/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| tensor/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| tensor/tensor_butterfly_operator.cpp | 1 | 0 | 1 | 0 | 0 |
| tensor/tensor_core_bridge.cpp | 1 | 1 | 0 | 0 | 0 |
| tensor/tensor_fingerprint_graph.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### tensor/hnsw_tt_bridge.cpp
Total findings: 21

- Line 65: severity=CRITICAL; category=missing_dtor
  Description: Class HnswTTBridge allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct HnswTTBridge
- Line 90: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* sp = new hnswlib::L2Space(dim);
- Line 90: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* sp = new hnswlib::L2Space(dim);
- Line 92: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: appr_ = new hnswlib::HierarchicalNSW<float>(
- Line 189: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = label_to_id_.find(label);
- Line 95: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete sp;
- Line 95: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: appr_ = new hnswlib::HierarchicalNSW<float>(

                sp, kInitialCapacity, M_, ef_construction_);

        } catch (...) {

            delete sp;

            throw;

        }

        space_ = sp;
- Line 95: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete sp;
- Line 114: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ~HnswLayer() {
- Line 116: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete appr_;
- Line 116: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ~HnswLayer() {

#ifdef THEMIS_HNSW_ENABLED

        delete appr_;

        delete space_;

#endif

    }
- Line 116: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete appr_;
- Line 117: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete space_;
- Line 117: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ~HnswLayer() {

#ifdef THEMIS_HNSW_ENABLED

        delete appr_;

        delete space_;

#endif

    }
- Line 117: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete space_;
- Line 533: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 541: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 542: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 616: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (d == 0 || d != B.cores.size()) return 0.0f;
- Line 627: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 121: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### tensor/hiss_structural_search.cpp
Total findings: 10

- Line 74: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return h / std::log2(static_cast<double>(kBins)); // normalized [0,1]
- Line 299: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 417: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (avg_entropy >= (cfg.entropy_threshold * 1.5)) {

            const auto rerouted = graph.rerouteEdge(e.from, e.to, "clustered");

            if (!rerouted) {

                throw std::logic_error(

                    "failed to reroute edge from " + std::to_string(e.from) +

                    " to " + std::to_string(e.to) + " to clustered topology");

            }
- Line 487: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    for (std::size_t physical_idx = 0; physical_idx < dense_tensor.size(); ++physical_idx) {', '        const auto qtt_idx = dense_to_qtt.physicalToQTT(physical_idx);', '        padded_dense_tensor[qtt_idx] = dense_tensor[physical_idx];', '    }', '']
- Line 216: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto bd = bit_depths[d];
- Line 220: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            --bit_pos;', '            const auto bit = (multi_idx[d] >> (bd - 1u - b)) & 1ULL;', '            qtt_idx |= bit << bit_pos;', '        }', '    }']
- Line 252: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto bd = bit_depths[d];
- Line 256: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        for (std::size_t b = 0; b < bd; ++b) {', '            --bit_pos;', '            const auto bit = (qtt_idx >> bit_pos) & 1ULL;', '            idx_d = (idx_d << 1u) | bit;', '        }']
- Line 257: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            --bit_pos;', '            const auto bit = (qtt_idx >> bit_pos) & 1ULL;', '            idx_d = (idx_d << 1u) | bit;', '        }', '        if (idx_d >= grid_sizes[d]) {']
- Line 383: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::uint64_t, TensorGraphEdge> best_by_edge;

### tensor/hyper_index_builder.cpp
Total findings: 10

- Line 249: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            }', '', '            auto& out = category_orders[category_index];', '            if (category_schema) {', '                out = category_schema->categories;']
- Line 273: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                      return lhs.first < rhs.first;', '                  });', '        auto& out = category_orders[category_index];', '        out.reserve(ordered.size());', '        for (const auto& [value, _] : ordered) {']
- Line 288: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @return Bucket index in [0, bucket_count-1].
- Line 304: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: double weight;      ///< Validated join strength in [0,1]
- Line 348: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: case HyperIndexConfig::MissingFkStatsFallback::IGNORE_EDGE:

                continue;

            case HyperIndexConfig::MissingFkStatsFallback::THROW:

                throw std::runtime_error(

                    "fk_graph edge is missing join_strength for from=" +

                    std::to_string(edge.from_column) + ", to=" +

                    std::to_string(edge.to_column));
- Line 375: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * deeper hops apply `propagation_decay` (clamped to [0,1]).
- Line 258: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::size_t> frequencies;
- Line 301: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 437: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::size_t> visited;
- Line 473: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto propagated_bucket = signal_bucket_sum[k] / signal_weight[k];

### tensor/tensor_index_manager.cpp
Total findings: 8

- Line 39: severity=CRITICAL; category=missing_dtor
  Description: Class FlatTensorIndex allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct FlatTensorIndex
- Line 15: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * - TIM-01  `ggmlCorePtrs()` legacy raw-pointer path — resolved 2026-05-20
- Line 16: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: * - TIM-02  `dropTenantIndexes()` RocksDB prefix-delete — resolved 2026-05-06
- Line 344: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ggmlCorePtrs() — raw-pointer legacy bridge (kept for backward compat)
- Line 346: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy compatibility path:
- Line 16: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: * - TIM-02  `dropTenantIndexes()` RocksDB prefix-delete — resolved 2026-05-06
- Line 296: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: escaped += (c == ':' || c == '/' || c == '\\') ? '_' : c;

### tensor/tensor_ingestion_bridge.cpp
Total findings: 8

- Line 183: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.serialized_train   = train.serialize();



        // Provenance metadata (FITKO / regulated-industry requirement)

        rec.metadata["tt_epsilon"]         = std::to_string(eff_eps);

        rec.metadata["tt_max_rank_cap"]    = std::to_string(eff_max_rank);

        rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);
- Line 184: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Provenance metadata (FITKO / regulated-industry requirement)

        rec.metadata["tt_epsilon"]         = std::to_string(eff_eps);

        rec.metadata["tt_max_rank_cap"]    = std::to_string(eff_max_rank);

        rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);
- Line 185: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Provenance metadata (FITKO / regulated-industry requirement)

        rec.metadata["tt_epsilon"]         = std::to_string(eff_eps);

        rec.metadata["tt_max_rank_cap"]    = std::to_string(eff_max_rank);

        rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);
- Line 186: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["tt_epsilon"]         = std::to_string(eff_eps);

        rec.metadata["tt_max_rank_cap"]    = std::to_string(eff_max_rank);

        rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);

        rec.metadata["tt_dim_original"]    = std::to_string(embedding.size());
- Line 187: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["tt_max_rank_cap"]    = std::to_string(eff_max_rank);

        rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);

        rec.metadata["tt_dim_original"]    = std::to_string(embedding.size());

        rec.metadata["tt_mode_shape"]      =
- Line 188: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["tt_order"]           = std::to_string(rec.order);

        rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);

        rec.metadata["tt_dim_original"]    = std::to_string(embedding.size());

        rec.metadata["tt_mode_shape"]      =

            std::to_string(mode_shape[0]) + "x" + std::to_string(mode_shape[1]);
- Line 189: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["tt_achieved_rank"]   = std::to_string(rec.max_rank);

        rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);

        rec.metadata["tt_dim_original"]    = std::to_string(embedding.size());

        rec.metadata["tt_mode_shape"]      =

            std::to_string(mode_shape[0]) + "x" + std::to_string(mode_shape[1]);
- Line 190: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["tt_compression"]     = std::to_string(rec.compression_ratio);

        rec.metadata["tt_achieved_eps"]    = std::to_string(rec.achieved_eps);

        rec.metadata["tt_dim_original"]    = std::to_string(embedding.size());

        rec.metadata["tt_mode_shape"]      =

            std::to_string(mode_shape[0]) + "x" + std::to_string(mode_shape[1]);



        spdlog::debug("[TensorIngestionBridge] decompose chunk='{}' "

### tensor/adapter_repository.cpp
Total findings: 7

- Line 288: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator sep may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto sep = rest.find(':');
- Line 287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto sep = rest.find(':');
- Line 313: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto sep = rest.find(':');
- Line 231: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,
- Line 237: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,
- Line 357: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,
- Line 364: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,

### tensor/tensor_index.cpp
Total findings: 5

- Line 293: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 340: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 342: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 418: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 230: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static constexpr char kMagic[11] = "THEMIS_TTI";  // 10 chars + '\0'

### tensor/utr_converter.cpp
Total findings: 5

- Line 186: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto pos = text.find("\n\n", start);
- Line 610: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Priority 1: registered ITextEncoder

            emb = text_encoder->encode(seg, embed_dim);

            if (emb.size() != embed_dim) {

                throw std::runtime_error(

                    "UTRConverter::fromDocument: registered ITextEncoder ('" +

                    std::string(text_encoder->description()) +

                    "') returned " + std::to_string(emb.size()) +
- Line 620: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Priority 2: raw EmbedFn bridge

            emb = embed_fn(seg, embed_dim);

            if (emb.size() != embed_dim) {

                throw std::runtime_error(

                    "UTRConverter::fromDocument: injected EmbedFn returned " +

                    std::to_string(emb.size()) +

                    " elements but embed_dim=" + std::to_string(embed_dim));
- Line 204: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!seg.empty()) segments.push_back(seg);
- Line 269: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    for (std::size_t lane = 0; lane < 8 && lane < embed_dim; ++lane) {', '        const auto shift = (lane % 4) * 16U;', '        const auto bits  = static_cast<uint16_t>((h >> shift) & 0xFFFFULL);', '        const auto dim   = static_cast<std::size_t>((bits + lane * 131U) % embed_dim);', '        const auto sign  = ((h >> (shift + 7U)) & 1ULL) ? 1.0f : -1.0f;']

### tensor/tensor_mmap_bridge.cpp
Total findings: 4

- Line 24: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: * heap-allocated copy guarded by `VirtualLock` (Windows) or a no-lock
- Line 104: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: MEM_COMMIT | MEM_RESERVE,

                          PAGE_READWRITE);

#else

    return std::malloc(bytes);

#endif

}
- Line 140: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr);
- Line 276: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: release();

### tensor/tnsr_task.cpp
Total findings: 4

- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (cancel_requested_.load(std::memory_order_acquire)) {
- Line 162: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 186: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 187: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### tensor/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### tensor/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### tensor/tensor_butterfly_operator.cpp
Total findings: 1

- Line 318: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Validate shape compatibility

### tensor/tensor_core_bridge.cpp
Total findings: 1

- Line 120: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<void> TensorCoreStorageBridge::write(

### tensor/tensor_fingerprint_graph.cpp
Total findings: 1

- Line 252: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto it_train = trains_.find(key);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
