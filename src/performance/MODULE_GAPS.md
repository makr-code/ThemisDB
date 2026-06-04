# performance Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: performance
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 319
- Actionable Findings (Critical + High): 232
- Affected Files: 30

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 88 |
| High | 144 |
| Medium | 82 |
| Low | 5 |

## Category Summary

| Category | Count |
|---|---:|
| data_race | 33 |
| manual_cleanup | 28 |
| no_timeout | 27 |
| resource_leaked_in_exception | 19 |
| db_connection_leak | 17 |
| fp_exact_comparison | 12 |
| primitive_no_volatile | 12 |
| unordered_container_iter | 9 |
| unchecked_array_index | 8 |
| blocking_no_timeout | 7 |
| copy_overhead | 7 |
| delete_without_nullptr | 7 |
| explicit_delete | 7 |
| memory_order | 7 |
| missing_trace_point | 7 |
| null_dereference | 7 |
| thread_join_no_timeout | 7 |
| manual_cleanup_in_destructor | 6 |
| pointer_arithmetic_unbounded | 6 |
| uncaught_exception | 6 |
| generic_catch | 5 |
| uninitialized_access | 5 |
| allocation_loop | 4 |
| delete_no_nullptr | 4 |
| explicit_lock_unlock | 4 |
| hardcoded_path | 3 |
| lock_contention | 3 |
| new_without_delete | 3 |
| new_without_raii | 3 |
| o_n_squared | 3 |
| path_traversal | 3 |
| smart_ptr_misuse | 3 |
| stale_doc_section_reference | 3 |
| arithmetic_overflow | 2 |
| exception_in_destructor | 2 |
| expensive_inner_op | 2 |
| hardcoded_output | 2 |
| lock_in_loop | 2 |
| missing_latency_metric | 2 |
| missing_move_constructor_defaulted | 2 |
| module_doc_linkset_drift | 2 |
| posix_only_api | 2 |
| range_temporary | 2 |
| unnecessary_copy | 2 |
| iterator_invalidation | 1 |
| missing_consensus | 1 |
| nested_loop_find | 1 |
| pointer_without_null_check | 1 |
| random_unseeded | 1 |
| shared_state_no_sync | 1 |
| size_assumption | 1 |
| unchecked_malloc | 1 |
| uninitialized_member_field | 1 |
| uninitialized_pointer | 1 |
| unspecified_consistency | 1 |
| unstructured_log | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| performance/phase3/bwtree.cpp | 45 | 14 | 27 | 4 | 0 |
| performance/phase4/pmu_counters.cpp | 31 | 6 | 8 | 17 | 0 |
| performance/adaptive_query_compiler.cpp | 25 | 5 | 17 | 3 | 0 |
| performance/phase4/io_uring_zero_copy.cpp | 22 | 11 | 7 | 4 | 0 |
| performance/numa_memory_manager.cpp | 21 | 1 | 13 | 5 | 2 |
| performance/phase3/diskann.cpp | 21 | 6 | 13 | 2 | 0 |
| performance/wisckey.cpp | 20 | 7 | 7 | 5 | 1 |
| performance/ligra.cpp | 16 | 9 | 7 | 0 | 0 |
| performance/numa_topology.cpp | 15 | 1 | 2 | 12 | 0 |
| performance/phase4/pmem_storage.cpp | 15 | 6 | 5 | 4 | 0 |
| performance/async_metrics_exporter.cpp | 13 | 2 | 9 | 2 | 0 |
| performance/prometheus_exporter.cpp | 8 | 8 | 0 | 0 | 0 |
| performance/rabitq.cpp | 8 | 0 | 8 | 0 | 0 |
| performance/chimera_exporter.cpp | 7 | 4 | 0 | 3 | 0 |
| performance/hardware_accelerator.cpp | 7 | 0 | 4 | 3 | 0 |
| performance/phase3/splinterdb.cpp | 7 | 5 | 1 | 1 | 0 |
| performance/cicada.cpp | 6 | 0 | 5 | 1 | 0 |
| performance/advanced_cache_manager.cpp | 5 | 0 | 2 | 3 | 0 |
| performance/intelligent_prefetcher.cpp | 4 | 0 | 3 | 1 | 0 |
| performance/workload_adaptive_optimizer.cpp | 4 | 1 | 1 | 2 | 0 |
| performance/phase3/adaptive_batch_tuner.cpp | 3 | 0 | 3 | 0 | 0 |
| performance/phase3/gunrock.cpp | 3 | 0 | 2 | 1 | 0 |
| performance/phase3/memory_pressure.cpp | 3 | 1 | 0 | 2 | 0 |
| performance/phase3/per_query_cost_model.cpp | 3 | 0 | 0 | 3 | 0 |
| performance/phase3/bao.cpp | 2 | 1 | 0 | 1 | 0 |
| performance/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| performance/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| performance/cycle_metrics.cpp | 1 | 0 | 0 | 1 | 0 |
| performance/phase2_feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |
| performance/phase3/feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### performance/phase3/bwtree.cpp
Total findings: 45

- Line 47: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto root = new LeafPage();
- Line 47: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Create initial root page

    root_pid_ = next_pid_.fetch_add(1, std::memory_order_relaxed);

    auto root = new LeafPage();

    

    // Install root in mapping table

    BwTreePage* expected = nullptr;
- Line 47: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto root = new LeafPage();
- Line 57: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 90: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto delta = new DeltaInsert(key, value);
- Line 90: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        // Create delta insert record

        auto delta = new DeltaInsert(key, value);

        delta->next_delta.store(page, std::memory_order_relaxed);

        

        // Try to install delta
- Line 90: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto delta = new DeltaInsert(key, value);
- Line 121: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: auto delta = new DeltaDelete(key);
- Line 121: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // the key is absent (it simply skips the erase).  A pre-CAS search

        // would introduce a TOCTOU race — another thread could remove the same

        // key between the check and the CAS, making the check unreliable.

        auto delta = new DeltaDelete(key);

        delta->next_delta.store(page, std::memory_order_relaxed);



        // Try to install delta via CAS.
- Line 121: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto delta = new DeltaDelete(key);
- Line 136: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: BwTreePage* page = mapping_table_->get(pid);
- Line 36: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::memory_order_acquire
- Line 52: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete root;
- Line 52: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Install root in mapping table

    BwTreePage* expected = nullptr;

    if (!mapping_table_->compare_and_swap(root_pid_, expected, root)) {

        delete root;

        throw std::runtime_error("Failed to install root page");

    }

}
- Line 52: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete root;
- Line 60: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: BwTree::~BwTree() {

    // Reclaim all deferred-deletion chains accumulated during operation.

    // At destruction time there are no concurrent readers, so it is safe

    // to delete every chain unconditionally.

    std::lock_guard<std::mutex> lk(retired_mutex_);

    for (auto& rc : retired_chains_) {

        delete_chain(rc.head);
- Line 60: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // to delete every chain unconditionally.
- Line 90: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto delta = new DeltaInsert(key, value);
- Line 90: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 99: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete delta;
- Line 99: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        // CAS failed, retry

        delete delta;

    }

}
- Line 99: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete delta;
- Line 116: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: continue;

        }



        // Create delta delete record and link it to the current chain head.

        // We install it unconditionally: apply_deltas() handles the case where

        // the key is absent (it simply skips the erase).  A pre-CAS search

        // would introduce a TOCTOU race — another thread could remove the same
- Line 116: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Create delta delete record and link it to the current chain head.
- Line 121: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 130: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        // CAS failed: `delta` was never published to the mapping table so no

        // other thread holds a reference to it — safe to delete immediately.

        delete delta;

    }

}
- Line 130: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // other thread holds a reference to it — safe to delete immediately.
- Line 131: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete delta;
- Line 131: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // CAS failed: `delta` was never published to the mapping table so no

        // other thread holds a reference to it — safe to delete immediately.

        delete delta;

    }

}
- Line 131: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete delta;
- Line 255: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: current = current->next_delta.load(std::memory_order_acquire);
- Line 332: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: current = current->next_delta.load(std::memory_order_acquire);
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: consolidation_epoch_.load(std::memory_order_acquire)});
- Line 354: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: consolidation_epoch_.load(std::memory_order_acquire);
- Line 372: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BwTreePage* next = current->next_delta.load(std::memory_order_acquire);
- Line 373: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete current;
- Line 373: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: BwTreePage* current = head;

    while (current) {

        BwTreePage* next = current->next_delta.load(std::memory_order_acquire);

        delete current;

        current = next;

    }

}
- Line 373: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete current;
- Line 99: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete delta;
- Line 131: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete delta;
- Line 222: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: consolidated.release();
- Line 373: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete current;

### performance/phase4/pmu_counters.cpp
Total findings: 31

- Line 112: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 115: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ssize_t bytes_read = ::read(fd_, &value, sizeof(value));
- Line 409: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 587: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 716: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 848: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: uint64_t PmuCounter::read()  const noexcept {
- Line 45: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PmuCounter::~PmuCounter() noexcept {
- Line 275: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: lib = ::dlopen(path, RTLD_LAZY | RTLD_LOCAL);
- Line 375: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['// Per-thread kpc counter snapshot buffers', 'constexpr uint32_t kKpcBufSize = 32;', 'static thread_local uint64_t tl_kpc_baseline[kKpcBufSize] = {};', '', '} // anonymous namespace']
- Line 384: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PmuCounter::~PmuCounter() noexcept { close(); }
- Line 460: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        auto& api = KpcApi::instance();', '        if (api.loaded) {', '            uint64_t current[kKpcBufSize] = {};', '            int ret = api.get_thread_counters(0, kKpcBufSize, current);', '            if (ret != 0) {']
- Line 500: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        auto& api = KpcApi::instance();', '        if (api.init()) {', '            uint64_t probe[kKpcBufSize] = {};', '            int ret = api.get_thread_counters(0, kKpcBufSize, probe);', '            return ret == 0;']
- Line 564: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PmuCounter::~PmuCounter() noexcept { close(); }
- Line 695: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PmuCounter::~PmuCounter() noexcept { close(); }
- Line 64: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 122: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PmuCounter::close() noexcept {
- Line 124: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);
- Line 153: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: static_cast<uint64_t>(PERF_COUNT_HW_BRANCH_MISSES));



        available_ = ok;

    } catch (...) {

        available_ = false;

        // Ensure counters are in closed state on exception

        l1d_misses_.close();
- Line 153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 258: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool                         loaded               = false;
- Line 308: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 332: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kMaxFallbackSlots = 128;
- Line 392: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 416: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 553: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kMaxWinSlots = 128;
- Line 572: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 594: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 684: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kMaxRdtscSlots = 128;
- Line 703: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 723: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 786: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 4 (PMU Counters)' that was not found in 'src/performance/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/performance/ROADMAP.md § Phase 4 (PMU Counters).

### performance/adaptive_query_compiler.cpp
Total findings: 25

- Line 374: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (count == cfg_.hot_threshold && !entry.compiled && is_compilable(query)) {

            lock.unlock();

            auto cq = compileImpl(query, schema, cfg_, /*record_stats=*/true);

            lock.lock();

            if (cq) {

                entry.compiled             = std::move(cq);

                entry.baseline_row_count   = 0.0;  // Set after first hot call
- Line 374: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 399: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.recompilations++;

                    lock.unlock();

                    auto cq = compileImpl(query, schema, cfg_, /*record_stats=*/true);

                    lock.lock();

                    if (cq) {

                        entry.compiled = std::move(cq);

                        entry.baseline_row_count = 0.0;
- Line 399: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 738: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto rit = hash_table.find(lkstr);
- Line 92: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::EQ:   return lhs == rhs;
- Line 93: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
- Line 104: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::EQ:   return lhs == rhs;
- Line 105: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
- Line 118: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::EQ:   return lhs == rhs;
- Line 119: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
- Line 139: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: return lhs == pattern;
- Line 279: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: asm_str << "  prefetcht0 [rdi]\n";
- Line 374: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 399: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 416: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 418: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (entry.baseline_row_count == 0.0)
- Line 436: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 656: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (groups.find(gkey) == groups.end())
- Line 697: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fn == "MIN")   return acc.min_v != std::numeric_limits<double>::max()
- Line 699: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fn == "MAX")   return acc.max_v != std::numeric_limits<double>::lowest()
- Line 987: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (groups.find(gk) == groups.end()) order.push_back(gk);
- Line 31: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 639: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, AggAccum> groups;
- Line 979: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, AggAccum> groups;

### performance/phase4/io_uring_zero_copy.cpp
Total findings: 22

- Line 82: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->ring_fd  = -1;
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->sq_ptr   = nullptr;
- Line 265: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->sqe_ptr  = nullptr;
- Line 268: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_ptr       = cq_ptr;
- Line 269: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_mmap_size = cq_ring_sz;
- Line 272: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_head         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.head);
- Line 273: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_tail         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.tail);
- Line 274: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_ring_mask    = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_mask);
- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cq_ring_entries = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_entries);
- Line 276: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_->cqes = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);
- Line 373: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            sqe->opcode      = IORING_OP_SEND;', '            sqe->fd          = fd;', '            sqe->addr        = reinterpret_cast<uint64_t>(buffers_[buf_index].data());', '            sqe->len         = static_cast<uint32_t>(len);', '            sqe->buf_index   = static_cast<uint16_t>(buf_index);']
- Line 382: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: #endif

            sqe->user_data   = static_cast<uint64_t>(buf_index);



            ring_->sq_array[index] = index;

            // Store-release so the kernel sees our SQE before we advance the tail

            __atomic_store_n(ring_->sq_tail, tail + 1, __ATOMIC_RELEASE);
- Line 399: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    // Fallback: standard blocking send()', '    ssize_t ret = ::send(fd, buffers_[buf_index].data(), len,', '#ifdef MSG_NOSIGNAL', '                         MSG_NOSIGNAL']
- Line 429: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            sqe->opcode    = IORING_OP_RECV;', '            sqe->fd        = fd;', '            sqe->addr      = reinterpret_cast<uint64_t>(buffers_[buf_index].data());', '            sqe->len       = static_cast<uint32_t>(max_len);', '            sqe->buf_index = static_cast<uint16_t>(buf_index);']
- Line 437: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: #endif

            sqe->user_data = static_cast<uint64_t>(buf_index);



            ring_->sq_array[index] = index;

            __atomic_store_n(ring_->sq_tail, tail + 1, __ATOMIC_RELEASE);



            int submitted = submit_sqes();
- Line 451: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    // Fallback: standard blocking recv()', '    ssize_t ret = ::recv(fd, buffers_[buf_index].data(), max_len, 0);', '    if (ret < 0) return -errno;', '    fallback_recvs_.fetch_add(1, std::memory_order_relaxed);']
- Line 487: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cq_completed_.fetch_add(count, std::memory_order_relaxed);
- Line 219: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 243: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 543: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);

### performance/numa_memory_manager.cpp
Total findings: 21

- Line 55: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* f = fopen(path, "r");
- Line 43: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (access(path, F_OK) == 0) n = static_cast<size_t>(i + 1);
- Line 55: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: FILE* f = fopen(path, "r");
- Line 109: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(size);
- Line 148: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* NUMAMemoryManager::allocate_on_node(size_t size, int node) {
- Line 150: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* ptr = do_allocate(size, resolved, false);
- Line 156: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* NUMAMemoryManager::allocate_local(size_t size) {
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return allocate_on_node(size, get_current_node());
- Line 160: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
- Line 160: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
- Line 162: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* ptr = do_allocate(size, resolved, hint.use_huge_pages);
- Line 168: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
- Line 168: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
- Line 243: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: s.local_accesses  = stat_local_.load(std::memory_order_relaxed);
- Line 42: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
- Line 54: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%zu/meminfo", i);
- Line 65: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fclose(f);
- Line 170: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int node = 0;
- Line 191: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr);
- Line 42: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
- Line 54: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%zu/meminfo", i);

### performance/phase3/diskann.cpp
Total findings: 21

- Line 308: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->read(reinterpret_cast<char*>(node.vector.data()),
- Line 313: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->read(reinterpret_cast<char*>(&neighbor_count), sizeof(uint32_t));
- Line 317: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->read(reinterpret_cast<char*>(node.neighbors.data()),
- Line 333: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->write(reinterpret_cast<const char*>(node.vector.data()),
- Line 338: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->write(reinterpret_cast<const char*>(&neighbor_count), sizeof(uint32_t));
- Line 341: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: graph_file_->write(reinterpret_cast<const char*>(node.neighbors.data()),
- Line 28: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t node_size_estimate = sizeof(VectorID) + dimension * sizeof(float) + 64 * sizeof(VectorID);
- Line 50: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: DiskANNIndex::~DiskANNIndex() {
- Line 108: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& node : nodes) {
- Line 111: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(file_mutex_);
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 298: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto it = vector_offsets_.find(id);

    if (it == vector_offsets_.end()) {

        throw std::runtime_error("Vector ID not found in index");

    }

    

    graph_file_->seekg(it->second);
- Line 465: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->id = vectors[start].first;
- Line 466: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->vector = vectors[start].second;
- Line 475: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));
- Line 481: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->threshold = sorted_dists[sorted_dists.size() / 2];
- Line 486: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (compute_distance(node->vector, vectors[i].second) < node->threshold) {
- Line 493: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left = build_tree(vectors, start + 1, mid);
- Line 494: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = build_tree(vectors, mid, end);
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({id, dist});
- Line 475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));

### performance/wisckey.cpp
Total findings: 20

- Line 33: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: current_offset_ = log_file_->tellg();
- Line 71: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: std::optional<std::string> ValueLog::read(const ValueAddress& addr) {
- Line 78: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: log_file_->read(&value[0], addr.size);
- Line 114: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: log_file_->read(&value[0], addr.size);
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ValueAddress addr = value_log_->append(value);
- Line 183: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: return value_log_->read(addr);
- Line 194: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.value_log_size = value_log_->size();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #967 [WIP] Implement garbage col... (2026-03-11) | #1122 Eliminate lock overl
- Line 20: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 49: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ValueLog::~ValueLog() {
- Line 99: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 122: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Write value to new log
- Line 122: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 131: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 23: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: test.close();
- Line 39: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: log_file_->close();
- Line 117: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: temp_log.close();
- Line 137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: temp_log.close();
- Line 140: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: log_file_->close();
- Line 101: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::fstream temp_log(temp_log_path, std::ios::out | std::ios::binary);

### performance/ligra.cpp
Total findings: 16

- Line 63: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    for (auto& thread : threads) {

        thread.join();

    }

}
- Line 63: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 63: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 86: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    for (auto& thread : threads) {

        thread.join();

    }

}
- Line 86: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 86: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 159: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        

        for (auto& thread : threads) {

            thread.join();

        }

        

        // Merge thread-local frontiers into next_frontier
- Line 159: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 159: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 44: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Simple parallel processing (would use thread pool in production)', '    std::vector<std::thread> threads;', '    size_t chunk_size = (active.size() + num_threads_ - 1) / num_threads_;', '', '    auto it = active.begin();']
- Line 104: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: atomic_dense[i].store(false, std::memory_order_relaxed);
- Line 112: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: atomic_dense[dst].store(true, std::memory_order_relaxed);
- Line 121: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (atomic_dense[i].load(std::memory_order_relaxed)) {
- Line 132: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        const auto& active = frontier.get_sparse();', '        std::vector<std::thread> threads;', '        size_t chunk_size = (active.size() + num_threads_ - 1) / num_threads_;', '', '        auto it = active.begin();']
- Line 212: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Reset new ranks
- Line 212: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### performance/numa_topology.cpp
Total findings: 15

- Line 124: severity=CRITICAL; category=uninitialized_pointer
  Description: Undefined behavior: potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer declared but not initialized
- Line 303: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cs) == 0;
- Line 347: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cs) != 0) return {};
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: try { cpus.push_back(std::stoi(token)); } catch (...) {}
- Line 82: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // token is either "N" or "N-M"

        size_t dash = token.find('-');

        if (dash == std::string::npos) {

            try { cpus.push_back(std::stoi(token)); } catch (...) {}

        } else {

            try {

                int lo = std::stoi(token.substr(0, dash));
- Line 82: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { cpus.push_back(std::stoi(token)); } catch (...) {}
- Line 132: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                int id = std::stoi(name.substr(4));

                node_ids.push_back(id);

            } catch (...) {}

        }

    }

    closedir(dir);
- Line 132: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 158: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint64_t kb = 0;

                    // Format: "Node N MemTotal: XXXX kB"

                    while (iss >> tok) {

                        try { kb = std::stoull(tok); } catch (...) {}

                    }

                    node.memory_bytes = kb * 1024;

                    break;
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { kb = std::stoull(tok); } catch (...) {}
- Line 287: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (unsigned int i = 0; i < nproc; ++i) node0.cpu_ids.push_back(static_cast<int>(i));
- Line 349: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < CPU_SETSIZE; ++i) {
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cpus.push_back(base + bit);
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (unsigned int i = 0; i < nproc; ++i) cpus.push_back(static_cast<int>(i));
- Line 440: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (unsigned int i = 0; i < nproc; ++i) cpus.push_back(static_cast<int>(i));

### performance/phase4/pmem_storage.cpp
Total findings: 15

- Line 50: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* mounts = fopen("/proc/mounts", "r");
- Line 57: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: while (fscanf(mounts, "%254s %254s %62s %510s %d %d",
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: header_->pool_size     = pool_size;
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: header_->bitmap_offset = header_end;
- Line 336: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: s.total_bytes  = header_->pool_size;
- Line 360: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void* PMemStorageLayout::write(const std::string& /*key*/,
- Line 286: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* PMemPool::allocate(size_t size) noexcept {
- Line 286: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* PMemPool::allocate(size_t size) noexcept {
- Line 299: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 308: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 369: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* dst = pool_.allocate(padded);
- Line 73: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fclose(mounts);
- Line 133: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (fd_ >= 0) ::close(fd_);
- Line 202: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);
- Line 315: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PMemPool::free(void* /*ptr*/, size_t /*size*/) noexcept {

### performance/async_metrics_exporter.cpp
Total findings: 13

- Line 129: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: export_thread_.join();
- Line 228: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: aggregated_metrics_.insert(
- Line 70: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 112: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: export_thread_ = std::thread([this]() {
- Line 178: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 180: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 181: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::seconds(export_interval_), [this]() {
- Line 182: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return !running_.load(std::memory_order_acquire);
- Line 215: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 220: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 231: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 62: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool registered = false;
- Line 104: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: void start(int export_interval_seconds = 1) {

### performance/prometheus_exporter.cpp
Total findings: 8

- Line 92: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_hnsw += m->hnsw_search_cycles;
- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_pointer += m->pointer_passing_cycles;
- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_llm += m->llm_inference_cycles;
- Line 95: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_cache += m->cache_miss_cycles;
- Line 96: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_pcie_h2d += m->pcie_host_to_device_cycles;
- Line 97: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_pcie_d2h += m->pcie_device_to_host_cycles;
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_total += m->total_cycles;
- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_cpu_eff += m->cpu_efficiency_ratio;

### performance/rabitq.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #1005 [REFACTOR] Quantizer analys... (2026-03-11) | #1072 Add Vector Indexing
- Line 262: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<std::vector<float>> centroids;

        centroids.reserve(k);

        centroids.push_back(subvec_data[uniform(rng)]);



        for (size_t ci = 1; ci < k; ++ci) {

            // For each sample compute D^2 distance to the nearest existing centroid.
- Line 295: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t ci = 0; ci < k; ++ci) {

                    float dist = 0.0f;

                    for (size_t dim = 0; dim < subvector_dimension_; ++dim) {

                        float diff = subvec_data[i][dim] - centroids[ci][dim];

                        dist += diff * diff;

                    }

                    if (dist < min_dist) {
- Line 307: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 315: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: size_t cluster = assignments[i];

                ++counts[cluster];

                for (size_t dim = 0; dim < subvector_dimension_; ++dim) {

                    new_centroids[cluster][dim] += subvec_data[i][dim];

                }

            }
- Line 324: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 325: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 331: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: max_shift = std::max(max_shift, shift);

                } else {

                    // Empty cluster: reinitialize to a random sample

                    new_centroids[ci] = subvec_data[uniform(rng)];

                }

            }

### performance/chimera_exporter.cpp
Total findings: 7

- Line 97: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_hnsw += m->hnsw_search_cycles;
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_pointer += m->pointer_passing_cycles;
- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_llm += m->llm_inference_cycles;
- Line 100: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_total += m->total_cycles;
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << ((double)avg_hnsw / avg_total * 100.0) << ",\n";
- Line 153: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << ((double)avg_pointer / avg_total * 100.0) << ",\n";
- Line 156: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << ((double)avg_llm / avg_total * 100.0) << "\n";

### performance/hardware_accelerator.cpp
Total findings: 7

- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = ht.find(key);
- Line 156: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = ht.find(key);
- Line 184: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [col](const Row& a, const Row& b) {
- Line 700: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
- Line 97: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<uint64_t, std::vector<size_t>>
- Line 99: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<uint64_t, std::vector<size_t>> ht;
- Line 700: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {

### performance/phase3/splinterdb.cpp
Total findings: 7

- Line 39: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_lock<std::mutex> lock(mutex_);

        

        if (wait) {

            cv_.wait(lock, [this]() { return !tasks_.empty() || shutdown_; });

        }

        

        if (tasks_.empty() || shutdown_) {
- Line 39: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: cv_.wait(lock, [this]() { return !tasks_.empty() || shutdown_; });
- Line 111: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Wait for all workers to finish

    for (auto& thread : worker_threads_) {

        if (thread.joinable()) {

            thread.join();

        }

    }
- Line 111: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 111: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 70: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static TaskQueue g_task_queue;
- Line 22: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields

### performance/cicada.cpp
Total findings: 6

- Line 24: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {
- Line 54: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Write the new data value into the record while the write lock is held
- Line 54: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 68: severity=HIGH; category=missing_trace_point
  Description: Critical function commit without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CicadaTransaction::commit() {
- Line 93: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void CicadaTransaction::abort() {
- Line 24: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {

### performance/advanced_cache_manager.cpp
Total findings: 5

- Line 462: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& p : partitions_) {
- Line 463: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(p->mtx);
- Line 85: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Consolidation Phase — Compression Codec' that was not found in 'src/performance/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md § "Consolidation Phase — Compression Codec"
- Line 213: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!bridged.empty()) {

                    return bridged;

                }

            } catch (...) {

            }

        }

    }
- Line 213: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### performance/intelligent_prefetcher.cpp
Total findings: 4

- Line 23: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   each candidate stride, and normalising to a [0,1] confidence score.
- Line 115: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stat_useful_prefetches_.fetch_add(1, std::memory_order_relaxed);
- Line 184: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stat_total_prefetches_.fetch_add(1, std::memory_order_relaxed);
- Line 292: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int64_t, size_t> stride_counts;

### performance/workload_adaptive_optimizer.cpp
Total findings: 4

- Line 195: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (adapt_thread_.joinable()) adapt_thread_.join();
- Line 187: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 56: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> table_counts;
- Line 186: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < static_cast<int>(adapt_interval_.count()) * 10 && adapt_running_; ++i)

### performance/phase3/adaptive_batch_tuner.cpp
Total findings: 3

- Line 77: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (ema_throughput_ == 0.0) {
- Line 135: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: } else if (throughput_improving || prev_ema_ == 0.0) {
- Line 181: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    size_t p99_idx = static_cast<size_t>(', '        0.99 * static_cast<double>(latencies.size() - 1));', '    s.p99_latency_ms = latencies[p99_idx];', '', '    return s;']

### performance/phase3/gunrock.cpp
Total findings: 3

- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: impl_->row_offsets.push_back(impl_->column_indices.size());

### performance/phase3/memory_pressure.cpp
Total findings: 3

- Line 61: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: poll_thread_.join();
- Line 51: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_call.push_back(entry.callback);

### performance/phase3/per_query_cost_model.cpp
Total findings: 3

- Line 124: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> factors;
- Line 261: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> type_time_sum;
- Line 262: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> type_count;

### performance/phase3/bao.cpp
Total findings: 2

- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.avg_speedup = impl_->queries_optimized > 0
- Line 25: severity=MEDIUM; category=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::mt19937 rng;

### performance/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### performance/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### performance/cycle_metrics.cpp
Total findings: 1

- Line 171: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'GPU Cycle Metrics.' that was not found in 'src/performance/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/performance/FUTURE_ENHANCEMENTS.md § GPU Cycle Metrics.

### performance/phase2_feature_flags.cpp
Total findings: 1

- Line 29: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto phase2 = config["performance"]["phase2"];

### performance/phase3/feature_flags.cpp
Total findings: 1

- Line 30: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto phase3 = config["performance"]["phase3"];

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
