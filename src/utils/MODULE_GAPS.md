# utils Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Remediation Log

### 2026-08-17 — Critical/High Gap Closure (manual remediation run)

Scope: Closed critical scanner findings in 5 source files. Scanner scan date was 2026-06-04;
remediation applied to `develop` branch on 2026-08-17.

| File | Findings Closed | Category | Status |
|---|---|---|---|
| utils/thread_pool_manager.cpp | 5 (CRITICAL×5) | thread_join_no_timeout, data_race | ✓ CLOSED |
| utils/http_client_pool.cpp | 3 (CRITICAL×3) | blocking_no_timeout, thread_join_no_timeout | ✓ CLOSED |
| utils/grpc_channel_pool.cpp | 2 (HIGH×2) | explicit_lock_unlock | ✓ CLOSED |
| utils/rate_limiter.cpp | 3 (CRITICAL×2, HIGH×1) | blocking_no_timeout, no_timeout, explicit_lock_unlock | ✓ CLOSED |
| utils/audit_logger.cpp | 4 (MEDIUM×4) | manual_cleanup (raw fd close) | ✓ CLOSED |

Remaining open critical/high items are tracked in ROADMAP.md Phase 2.4, 3.5-3.9, and the
outstanding scanner findings in pki_client.cpp, error_registry.cpp, and pool_allocator.cpp
(pool_allocator critical items were previously resolved per RESOLVED markers in scan output).

## Scan Snapshot

- Module: utils
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 414
- Actionable Findings (Critical + High): 235
- Affected Files: 44

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 44 |
| High | 191 |
| Medium | 160 |
| Low | 19 |

## Category Summary

| Category | Count |
|---|---:|
| hardcoded_path | 41 |
| db_connection_leak | 34 |
| resource_leaked_in_exception | 28 |
| copy_overhead | 26 |
| manual_cleanup | 24 |
| unnecessary_copy | 23 |
| string_concat_loop | 15 |
| lock_contention | 14 |
| missing_trace_point | 14 |
| pointer_arithmetic_unbounded | 14 |
| lock_in_loop | 12 |
| unstructured_log | 12 |
| legacy_or_compat_path | 10 |
| uninitialized_access | 10 |
| range_temporary | 9 |
| explicit_delete | 7 |
| no_timeout | 7 |
| unordered_container_iter | 7 |
| data_race | 6 |
| delete_without_nullptr | 5 |
| hardcoded_output | 5 |
| thread_join_no_timeout | 5 |
| array_bounds | 4 |
| array_bounds_violation | 4 |
| deadlock_risk | 4 |
| duplicate_qualified_signature | 4 |
| missing_dtor | 4 |
| repeated_lookup | 4 |
| unchecked_array_index | 4 |
| uninitialized_array | 4 |
| blocking_no_timeout | 3 |
| delete_no_nullptr | 3 |
| exception_in_destructor | 3 |
| explicit_lock_unlock | 3 |
| map_vs_unordered_map | 3 |
| missing_latency_metric | 3 |
| size_assumption | 3 |
| stale_doc_section_reference | 3 |
| missing_correlation_id | 2 |
| module_doc_linkset_drift | 2 |
| no_retry_logic | 2 |
| null_dereference | 2 |
| repeated_search | 2 |
| smart_ptr_misuse | 2 |
| unchecked_memcpy | 2 |
| allocation_loop | 1 |
| broken_raii_in_assignment | 1 |
| cast_to_smaller_type | 1 |
| expensive_copy | 1 |
| expensive_inner_op | 1 |
| fp_exact_comparison | 1 |
| manual_cleanup_in_destructor | 1 |
| nested_loop_find | 1 |
| new_without_delete | 1 |
| new_without_raii | 1 |
| pointer_without_null_check | 1 |
| shift_overflow | 1 |
| timestamp_sorting_unstable | 1 |
| uncaught_exception | 1 |
| unwrapped_resource | 1 |
| user_controlled_size | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| utils/memory/pool_allocator.cpp | 54 | 9 | 44 | 1 | 0 |
| utils/pki_client.cpp | 35 | 2 | 9 | 24 | 0 |
| utils/error_registry.cpp | 32 | 1 | 5 | 25 | 1 |
| utils/audit_logger.cpp | 25 | 5 | 4 | 15 | 1 |
| utils/http_client_pool.cpp | 20 | 3 | 16 | 1 | 0 |
| utils/grpc_channel_pool.cpp | 18 | 2 | 16 | 0 | 0 |
| utils/input_validator.cpp | 17 | 0 | 3 | 13 | 1 |
| utils/self_awareness.cpp | 13 | 0 | 1 | 12 | 0 |
| utils/tracing.cpp | 12 | 4 | 1 | 7 | 0 |
| utils/capability_auto_generator.cpp | 11 | 0 | 6 | 4 | 1 |
| utils/geo/ewkb.cpp | 11 | 0 | 5 | 6 | 0 |
| utils/lek_manager.cpp | 11 | 2 | 9 | 0 | 0 |
| utils/regex_detection_engine.cpp | 11 | 0 | 7 | 4 | 0 |
| utils/logger.cpp | 10 | 0 | 2 | 4 | 4 |
| utils/pii_detector.cpp | 10 | 0 | 4 | 6 | 0 |
| utils/pii_pseudonymizer.cpp | 9 | 4 | 4 | 1 | 0 |
| utils/thread_pool_manager.cpp | 8 | 5 | 3 | 0 | 0 |
| utils/build_info.cpp | 7 | 0 | 3 | 4 | 0 |
| utils/cursor.cpp | 7 | 0 | 7 | 0 | 0 |
| utils/hkdf_cache.cpp | 7 | 0 | 7 | 0 | 0 |
| utils/hkdf_helper.cpp | 7 | 0 | 2 | 5 | 0 |
| utils/zstd_codec.cpp | 7 | 1 | 6 | 0 | 0 |
| utils/checksum_utils.cpp | 6 | 0 | 3 | 3 | 0 |
| utils/rate_limiter.cpp | 6 | 2 | 3 | 1 | 0 |
| utils/cron_parser.cpp | 5 | 0 | 1 | 4 | 0 |
| utils/sampled_logger.cpp | 5 | 0 | 2 | 0 | 3 |
| utils/simd_distance.cpp | 5 | 0 | 5 | 0 | 0 |
| utils/bloom_filter.cpp | 4 | 0 | 0 | 1 | 3 |
| utils/lz4_codec.cpp | 4 | 0 | 3 | 1 | 0 |
| utils/ner_detection_engine.cpp | 4 | 0 | 1 | 3 | 0 |
| utils/retention_manager.cpp | 4 | 1 | 3 | 0 | 0 |
| utils/stopwords.cpp | 4 | 0 | 0 | 4 | 0 |
| utils/timestamp_utils.cpp | 4 | 0 | 0 | 1 | 3 |
| utils/compression_metrics.cpp | 3 | 0 | 0 | 3 | 0 |
| utils/pii_detection_engine.cpp | 3 | 0 | 1 | 2 | 0 |
| utils/utils_adapters.cpp | 3 | 0 | 2 | 1 | 0 |
| utils/consistent_hash.cpp | 2 | 0 | 0 | 2 | 0 |
| utils/pii_stream_scanner.cpp | 2 | 2 | 0 | 0 | 0 |
| utils/serialization.cpp | 2 | 1 | 1 | 0 | 0 |
| utils/update_checker.cpp | 2 | 0 | 1 | 1 | 0 |
| utils/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| utils/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| utils/runtime_license_gate.cpp | 1 | 0 | 1 | 0 | 0 |
| utils/saga_logger.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### utils/memory/pool_allocator.cpp
Total findings: 54

- Line 58: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  RESOLVED: BuddyAllocator now declares an explicit noexcept destructor and releases its backing pool via RAII-owned storage.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct BuddyAllocator
- Line 65: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  RESOLVED: BuddyAllocator now declares an explicit noexcept destructor and releases its backing pool via RAII-owned storage.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct BuddyAllocator
- Line 354: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  RESOLVED: SlabAllocator now declares an explicit noexcept destructor and owns slabs through smart pointers for automatic cleanup.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct SlabAllocator
- Line 374: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  RESOLVED: Slab and allocator teardown paths now use noexcept destructors backed by std::unique_ptr-managed storage.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 419: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  RESOLVED: SlabAllocator now declares an explicit noexcept destructor and owns slabs through smart pointers for automatic cleanup.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct SlabAllocator
- Line 465: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  RESOLVED: New slab creation now uses std::make_unique with ownership transferred into the slab list only after successful allocation setup.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 465: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  RESOLVED: New slab creation now uses std::make_unique with ownership transferred into the slab list only after successful allocation setup.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return nullptr;  // Hit slab limit

        }

        

        Slab* new_slab = new Slab(object_size, objects_per_slab);

        new_slab->next = head_slab;

        head_slab = new_slab;

        slab_count++;
- Line 465: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  RESOLVED: New slab creation is immediately wrapped in std::unique_ptr and linked with move ownership semantics.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 465: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  RESOLVED: Slab list ownership is now RAII-managed with std::unique_ptr for both slab nodes and slab buffers.
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 99: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 230: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
- Line 230: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
- Line 249: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* ptr = impl_->allocateBlock(order);
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats_.bytes_allocated.fetch_add(size);
- Line 272: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> BuddyAllocator::deallocate(void* ptr) {
- Line 369: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 378: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* allocate() {
- Line 394: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool deallocate(void* ptr) {
- Line 440: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ~Impl() {
- Line 444: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete slab;
- Line 444: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: Slab* slab = head_slab;

        while (slab != nullptr) {

            Slab* next = slab->next;

            delete slab;

            slab = next;

        }

    }
- Line 444: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete slab;
- Line 449: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* allocate() {
- Line 449: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* allocate() {
- Line 453: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* ptr = slab->allocate();
- Line 465: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 466: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 467: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 470: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new_slab->allocate();
- Line 473: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool deallocate(void* ptr) {
- Line 473: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool deallocate(void* ptr) {
- Line 477: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return slab->deallocate(ptr);
- Line 492: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 492: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 505: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* ptr = impl_->allocate();
- Line 513: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats_.bytes_allocated.fetch_add(impl_->object_size);
- Line 527: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> SlabAllocator::deallocate(void* ptr) {
- Line 552: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete slab;
- Line 552: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: Slab* slab = impl_->head_slab;

    while (slab != nullptr) {

        Slab* next = slab->next;

        delete slab;

        slab = next;

    }
- Line 552: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete slab;
- Line 602: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 602: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    Impl(size_t cap) : capacity(cap), offset(0) {', '        memory = new uint8_t[capacity];', '        std::memset(memory, 0, capacity);', '        // Reserve space for allocation tracking to reduce reallocations']
- Line 602: severity=HIGH; category=user_controlled_size
  Description: Allocation size not validated (potential DoS or overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    Impl(size_t cap) : capacity(cap), offset(0) {', '        memory = new uint8_t[capacity];', '        std::memset(memory, 0, capacity);', '        // Reserve space for allocation tracking to reduce reallocations']
- Line 623: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 623: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 647: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats_.bytes_allocated.fetch_add(aligned_size);
- Line 661: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> StackAllocator::deallocate(void* ptr) {
- Line 768: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        // Initialize slab allocators for common sizes', '        for (size_t size : config.slab_sizes) {', '            slabs[size] = std::make_unique<SlabAllocator>(', '                size, config.slab_objects_per_slab, config.slab_max_slabs);', '        }']
- Line 810: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
- Line 810: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
- Line 812: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto result = allocator->allocate(size, hint);
- Line 823: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
- Line 823: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
- Line 552: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete slab;

### utils/pki_client.cpp
Total findings: 35

- Line 200: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int len = static_cast<int>(pass->size());
- Line 202: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    int len = static_cast<int>(pass->size());', '    if (len > size) len = size;', '    std::memcpy(buf, pass->data(), len);', '    return len;', '}']
- Line 289: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 330: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!bptr || !bptr->data || bptr->length == 0) return {};
- Line 331: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return std::string(bptr->data, bptr->length);
- Line 402: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 430: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 441: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 599: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use EVP_PKEY signing (preferred) instead of deprecated RSA_sign API.
- Line 607: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use PKCS#1 v1.5 padding for compatibility with RSA_sign
- Line 819: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use EVP_PKEY verification instead of deprecated RSA_verify
- Line 190: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (alg.find("SHA256") != std::string::npos) { expected_len = 32; return NID_sha256; }
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" <<
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
- Line 582: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "
- Line 616: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(*pub_result);
- Line 624: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 627: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 675: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 697: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'PKI Client Production Signing' that was not found in 'src/utils/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/utils/FUTURE_ENHANCEMENTS.md § "PKI Client Production Signing"
- Line 768: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << res
- Line 769: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
- Line 776: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code="
- Line 787: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
- Line 794: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body <<
- Line 828: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 829: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pub);
- Line 832: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(ctx);
- Line 834: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pub);
- Line 849: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'PKI Client Production Signing' that was not found in 'src/utils/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/utils/FUTURE_ENHANCEMENTS.md § "PKI Client Production Signing"

### utils/error_registry.cpp
Total findings: 32

- Line 140: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
- Line 458: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "cudaMalloc or hipMalloc failed to allocate GPU memory.",
- Line 890: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
- Line 1322: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Failed to allocate memory for compression/decompression operation.",
- Line 1351: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Failed to allocate memory from the system.",
- Line 1579: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 86: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check file/directory permissions\n"
- Line 250: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "2. Verify tar/gzip availability\n"
- Line 345: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "2. Free up system memory (RAM/VRAM)\n"
- Line 446: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Ensure CUDA/ROCm runtime is available\n"
- Line 476: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Review NVIDIA/AMD documentation for peer access requirements\n"
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Ensure adapter format is supported (safetensors/GGUF)\n"
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "6. Use mixed precision training (fp16/bf16)\n"
- Line 662: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. List available tools with: GET /api/v1/mcp/tools\n"
- Line 690: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check if stdin/stdout are available\n"
- Line 692: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Ensure not running in detached/daemon mode\n"
- Line 849: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Verify table/field names exist\n"
- Line 861: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check query syntax matches AQL/GraphQL specification\n"
- Line 862: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "2. Verify all brackets/quotes are balanced\n"
- Line 875: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check if referenced tables/indexes exist\n"
- Line 890: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
- Line 929: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check window frame bounds (ROWS/RANGE)\n"
- Line 942: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Reduce result set size with additional FILTER/LIMIT clauses\n"
- Line 998: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Verify request format (JSON/GraphQL/etc.)\n"
- Line 1182: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Verify entropy source is available (/dev/urandom)\n"
- Line 1239: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "1. Check user/process permissions\n"
- Line 1240: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "2. Verify file/directory ownership\n"
- Line 1241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "3. Review SELinux/AppArmor policies\n"
- Line 1476: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "2. Ensure a node/edge is not both required and forbidden\n"
- Line 1722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(errors_.at(code));
- Line 1737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(pair.second);
- Line 140: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",

### utils/audit_logger.cpp
Total findings: 25

- Line 47: severity=CRITICAL; category=broken_raii_in_assignment
  Description: Broken RAII: undefined behavior and memory corruption
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 163: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int fd = ::open(cfg_.log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 189: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int fd2 = ::open(cfg_.secondary_log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 1552: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int fd = ::open(cfg_.chain_head_path.c_str(), O_RDONLY);
- Line 1597: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void HashChainAuditWriter::write(nlohmann::json record) {
- Line 763: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // This method is kept for API compatibility
- Line 1133: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (baseline.avg_frequency_seconds == 0.0) {
- Line 1459: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            // Track per-user counts', '            std::string user = payload.value("user_id", payload.value("user", std::string{"system"}));', '            user_counts[user] = user_counts.value(user, 0) + 1;', '', '        } catch (const nlohmann::json::exception &) {']
- Line 1604: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 166: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd2);
- Line 505: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = state["last_timestamp_ms"].get<uint64_t>();
- Line 664: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(sock);
- Line 794: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ts_ms = record["ts"].get<uint64_t>();
- Line 843: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ts_ms = record["ts"].get<uint64_t>();
- Line 941: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ts_ms = record["ts"].get<uint64_t>();
- Line 1201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: extensions.push_back("taskId=" + event["task_id"].get<std::string>());
- Line 1204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: extensions.push_back("suser=" + event["user_id"].get<std::string>());
- Line 1207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: extensions.push_back("rt=" + std::to_string(event["timestamp"].get<uint64_t>()));
- Line 1210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: extensions.push_back("executionTime=" + std::to_string(event["execution_time_ms"].get<double>()));
- Line 1213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: extensions.push_back("anomalyScore=" + std::to_string(event["anomaly_score"].get<double>()));
- Line 1317: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = record["ts"].get<int64_t>();
- Line 1407: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ms = record["ts"].get<int64_t>();
- Line 1555: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 642: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: formatted_message = formatAsSyslog(event, event_type);

### utils/http_client_pool.cpp
Total findings: 20

- Line 90: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Join I/O threads

    for (auto& thread : io_threads_) {

        if (thread.joinable()) {

            thread.join();

        }

    }
- Line 90: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 90: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 172: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::shared_ptr<HTTPClient> HTTPClientPool::acquireConnection() {
- Line 175: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // Use striped locking to reduce contention', '    size_t stripe_idx = getStripeIndex();', '    auto& stripe = stripes_[stripe_idx];', '', '    std::unique_lock<std::mutex> lock(stripe->mutex);']
- Line 220: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto deadline = now + config_.acquire_timeout;
- Line 247: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& stripe : stripes_) {
- Line 248: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 268: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 274: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& stripe : stripes_) {
- Line 275: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 290: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& stripe : stripes_) {
- Line 291: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 298: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& stripe : stripes_) {
- Line 299: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 330: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 0; i < stripes_.size(); ++i) {
- Line 332: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 432: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: beast::get_lowest_layer(stream).expires_after(config_.connect_timeout);

            

            // Connect

            beast::get_lowest_layer(stream).connect(results);

            

            // Perform SSL handshake

            stream.handshake(ssl::stream_base::client);
- Line 535: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return response;

        

    } catch (const std::exception& e) {

        throw std::runtime_error("HTTP request failed: " + std::string(e.what()));

    }

}
- Line 27: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex url_regex(R"((https?)://([^:/]+)(?::(\d+))?(/.*)?)", std::regex::icase);

### utils/grpc_channel_pool.cpp
Total findings: 18

- Line 84: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: pooled_channel->in_use = true;

            pooled_channel->last_used = std::chrono::steady_clock::now();

            

            lock.lock();

            pool->all_channels[channel] = pooled_channel;

            total_channels_.fetch_add(1);

            channels_created_.fetch_add(1);
- Line 84: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::shared_ptr<grpc::Channel> GrpcChannelPool::acquireChannel(
- Line 42: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 73: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // No available channels, check if we can create new one
- Line 77: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 84: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 140: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 142: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 143: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& [target, pool] : target_pools_) {
- Line 144: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 158: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 160: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [target, pool] : target_pools_) {
- Line 161: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 171: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 173: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& [target, pool] : target_pools_) {
- Line 174: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 360: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();

### utils/input_validator.cpp
Total findings: 17

- Line 309: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: static const char* forbidden[] = { "drop ", "truncate ", "alter ", "grant ", "revoke ", "create table", "insert ", "update ", "delete " };
- Line 343: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (unsigned char c : std::string(";|&`$(){}[]<>!?*\"\\'")) {
- Line 368: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: " truncate ", " insert ", " update ", " delete ",
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 131: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto min_len = prop["minLength"].get<size_t>();
- Line 138: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto max_len = prop["maxLength"].get<size_t>();
- Line 218: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = schema["properties"].begin();
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static const char* forbidden[] = { "drop ", "truncate ", "alter ", "grant ", "revoke ", "create tabl
- Line 351: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool InputValidator::validateAQLQuery(const std::string& query) const {
- Line 446: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '&':  result += "&amp;";  break;
- Line 447: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&':  result += "&amp;";  break;
- Line 448: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<':  result += "&lt;";   break;
- Line 449: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>':  result += "&gt;";   break;
- Line 450: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  result += "&quot;"; break;
- Line 451: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': result += "&#x27;"; break;
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '/':  result += "&#x2F;"; break;
- Line 617: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // if the string is ever passed as a printf format argument.

### utils/self_awareness.cpp
Total findings: 13

- Line 595: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.snapshot_directory)) {
- Line 41: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sa = root["self_awareness"];
- Line 54: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto thresh = sa["thresholds"];
- Line 65: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto snaps = sa["snapshots"];
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("CRITICAL: CPU usage at " +
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("WARNING: CPU usage at " +
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("CRITICAL: Memory usage at " +
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("WARNING: Memory usage at " +
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("CRITICAL: Disk usage at " +
- Line 380: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("WARNING: Disk usage at " +
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("WARNING: High average query time: " +
- Line 593: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Collect snapshot files sorted by name (which encodes timestamp)
- Line 598: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files.push_back(entry.path());

### utils/tracing.cpp
Total findings: 12

- Line 471: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (value[2] != '-' || value[35] != '-' || value[52] != '-') return false;
- Line 471: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: bool parseTraceparent(const std::string& value,

                      otel::trace::TraceId& trace_id_out,

                      otel::trace::SpanId& parent_id_out,

                      otel::trace::TraceFlags& flags_out) {

    if (value.size() != 55) return false;

    if (value[2] != '-' || value[35] != '-' || value[52] != '-') return false;



    auto hexByte = [](char hi, char lo, uint8_t& out) -> bool {

        auto fromHex = [](char c) -> int {

            if (c >= '0' && c <= '9') return c - '0';

            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
- Line 487: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: if (!hexByte(value[0], value[1], ver) || ver != 0) return false;
- Line 487: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: out = static_cast<uint8_t>((h << 4) | l);

        return true;

    };



    uint8_t ver{};

    if (!hexByte(value[0], value[1], ver) || ver != 0) return false;



    std::array<uint8_t, 16> tid{};

    for (int i = 0; i < 16; ++i) {

        if (!hexByte(value[3 + i * 2], value[3 + i * 2 + 1], tid[i])) return false;

    }
- Line 253: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return false;

            }

            tcp::socket socket(io);

            socket.connect(*results.begin(), ec);

            if (ec) {

                THEMIS_WARN("Tracing collector unreachable ({}:{}): {}. Tracing disabled.", host, port, ec.message());

                initialized_ = true;
- Line 164: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!out.empty()) out += ',';
- Line 170: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void Baggage::inject(std::map<std::string, std::string>& headers) {
- Line 177: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void Baggage::extract(const std::map<std::string, std::string>& headers) {
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex re(R"((?:http|https)://([^/:]+)(?::(\d+))?)", std::regex::icase);
- Line 272: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: {"service.name", serviceName},
- Line 273: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: {"service.version", "0.1.0"}
- Line 449: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::string headerValue(const std::map<std::string, std::string>& headers,

### utils/capability_auto_generator.cpp
Total findings: 11

- Line 152: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 173: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(60));
- Line 219: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 262: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 332: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(sorted_keywords.size(), (size_t)config_.max_keywords); ++i) {
- Line 371: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(capability.domains.begin(), capability.domains.end(), domain) == capability.domains.en
- Line 55: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto analysis = root["rocksdb_analysis"];
- Line 62: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto audit = root["audit"];
- Line 69: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto security = root["security"];
- Line 76: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto output = root["output"];
- Line 535: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::ofstream log(config_.audit_log_path, std::ios::app);

### utils/geo/ewkb.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2520 [geo] Full GeoJSON RFC 7946... (2026-03-11)
- Line 240: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint8_t temp[sizeof(double)];
- Line 255: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint8_t temp[sizeof(uint32_t)];
- Line 499: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "GeoJSON: longitude " + std::to_string(lon) + " is out of WGS84 range [-180, 180]");
- Line 503: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "GeoJSON: latitude " + std::to_string(lat) + " is out of WGS84 range [-90, 90]");
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(trimCopy(current));
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(trimCopy(current));
- Line 454: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: buf.push_back(is_little_endian ? 0x01 : 0x00);
- Line 721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: merged += ",";
- Line 722: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: merged += ",";
- Line 730: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring_groups.push_back(trimCopy(merged));

### utils/lek_manager.cpp
Total findings: 11

- Line 43: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 325: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: rotation_thread_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4263 PKIClient v1.8.0 + PII Stre... (2026-03-15) | #4216 feat(timeseries): C
- Line 115: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 193: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 193: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Delete from DB

    if (!db_->del(dbKey(date_str))) {

        throw std::runtime_error("Failed to delete rotated LEK from RocksDB");

    }

    

    // Regenerate
- Line 193: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 263: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 268: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 339: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(rotation_cv_mu_);
- Line 366: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& expired_date : to_revoke) {

### utils/regex_detection_engine.cpp
Total findings: 11

- Line 224: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::lock_guard<std::mutex> lock(mutex_);

    

    nlohmann::json metadata;

    metadata["engine_type"] = "regex";

    metadata["version"] = signature_.version;

    metadata["enabled"] = enabled_;

    metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),
- Line 225: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: nlohmann::json metadata;

    metadata["engine_type"] = "regex";

    metadata["version"] = signature_.version;

    metadata["enabled"] = enabled_;

    metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),

                                              [](const RegexPattern& p) { return p.enabled; });
- Line 226: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: nlohmann::json metadata;

    metadata["engine_type"] = "regex";

    metadata["version"] = signature_.version;

    metadata["enabled"] = enabled_;

    metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),

                                              [](const RegexPattern& p) { return p.enabled; });

    metadata["total_patterns"] = patterns_.size();
- Line 227: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["engine_type"] = "regex";

    metadata["version"] = signature_.version;

    metadata["enabled"] = enabled_;

    metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),

                                              [](const RegexPattern& p) { return p.enabled; });

    metadata["total_patterns"] = patterns_.size();

    metadata["signature_id"] = signature_.signature_id;
- Line 229: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["enabled"] = enabled_;

    metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),

                                              [](const RegexPattern& p) { return p.enabled; });

    metadata["total_patterns"] = patterns_.size();

    metadata["signature_id"] = signature_.signature_id;

    metadata["signer"] = signature_.signer;

    metadata["signed_at"] = signature_.signed_at;
- Line 230: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["pattern_count"] = std::count_if(patterns_.begin(), patterns_.end(),

                                              [](const RegexPattern& p) { return p.enabled; });

    metadata["total_patterns"] = patterns_.size();

    metadata["signature_id"] = signature_.signature_id;

    metadata["signer"] = signature_.signer;

    metadata["signed_at"] = signature_.signed_at;
- Line 231: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: [](const RegexPattern& p) { return p.enabled; });

    metadata["total_patterns"] = patterns_.size();

    metadata["signature_id"] = signature_.signature_id;

    metadata["signer"] = signature_.signer;

    metadata["signed_at"] = signature_.signed_at;

    

    return metadata;
- Line 67: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sig = config["signature"];
- Line 78: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto settings = config["settings"];
- Line 357: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: url.regex_str = R"(https?://[^\s]+)";
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: flag_strings.push_back(flag.get<std::string>());

### utils/logger.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4330 feat(cache): network-backed... (2026-03-19) | #4268 ProvenanceTracker:
- Line 51: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "\\\"";
- Line 46: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\"";
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\\";
- Line 51: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 51: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 95: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: spdlog::set_default_logger(logger_);
- Line 123: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: spdlog::set_default_logger(logger_);
- Line 153: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: spdlog::set_default_logger(logger_);

### utils/pii_detector.cpp
Total findings: 10

- Line 239: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use ConfigPathResolver to handle both new and legacy paths
- Line 239: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 508: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 529: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 150: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> PIIDetector::detectInJson(
- Line 153: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> result;
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: enabled.push_back(engine->getName());
- Line 275: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto settings = config["global_settings"];
- Line 490: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<PIIFinding>>& findings) const {
- Line 559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: deduplicated.push_back(findings.front());

### utils/pii_pseudonymizer.cpp
Total findings: 9

- Line 52: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 6
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
- Line 52: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 6
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (RAND_bytes(bytes, sizeof(bytes)) != 1) {

        throw std::runtime_error("Failed to generate random UUID");

    }

    

    // Set version (4) and variant bits

    bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4

    bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 10

    

    std::ostringstream oss;

    oss << std::hex << std::setfill('0');
- Line 53: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 8
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 10
- Line 53: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 8
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: throw std::runtime_error("Failed to generate random UUID");

    }

    

    // Set version (4) and variant bits

    bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4

    bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 10

    

    std::ostringstream oss;

    oss << std::hex << std::setfill('0');

    

    for (int i = 0; i < 16; ++i) {
- Line 148: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

        auto mapping = nlohmann::json::parse(*mapping_str);

        // Respect soft-delete flag

        bool active = mapping.value("active", true);

        if (!active) {

            return std::nullopt; // hidden
- Line 148: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Respect soft-delete flag
- Line 184: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: bool PIIPseudonymizer::erasePII(const std::string& pii_uuid) {

    std::scoped_lock lk(mu_);

    // Use short-lived transaction for read-check + delete to avoid conflicts

    for (int attempt = 0; attempt < 3; ++attempt) {

        auto txn = db_->beginTransaction();

        if (!txn) return false;
- Line 184: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Use short-lived transaction for read-check + delete to avoid conflicts
- Line 155: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto encrypted_json = mapping["original_value_encrypted"];

### utils/thread_pool_manager.cpp
Total findings: 8

- Line 160: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker.join();
- Line 260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.io_stats = io_pool_->getStatistics();
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.cpu_stats = cpu_pool_->getStatistics();
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.blocking_stats = blocking_pool_->getStatistics();
- Line 287: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: metrics_thread_.join();
- Line 41: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::shared_mutex> lock(mutex_);
- Line 44: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: bool available = cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
- Line 126: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));

### utils/build_info.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3830 feat(themis): Modular Build... (2026-03-12) | #3646 fix(themis): comple
- Line 534: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // This legacy file is no longer compiled; the note is kept for reference.
- Line 912: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 549: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'HSM Key Provider Production' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §"HSM Key Provider Production"
- Line 847: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(mod.name);
- Line 858: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(mod.name);
- Line 925: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### utils/cursor.cpp
Total findings: 7

- Line 100: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };

    

    if (order_value.has_value()) {

        cursor_data["order_value"] = *order_value;

    }

    

    std::string json_str = cursor_data.dump();
- Line 122: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        CursorInfo info;

        info.pk = cursor_data["pk"].get<std::string>();

        info.collection = cursor_data["collection"].get<std::string>();

        

        // Optional fields
- Line 123: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: CursorInfo info;

        info.pk = cursor_data["pk"].get<std::string>();

        info.collection = cursor_data["collection"].get<std::string>();

        

        // Optional fields

        if (cursor_data.contains("order_value") && !cursor_data["order_value"].is_null()) {
- Line 126: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: info.collection = cursor_data["collection"].get<std::string>();

        

        // Optional fields

        if (cursor_data.contains("order_value") && !cursor_data["order_value"].is_null()) {

            info.order_value = cursor_data["order_value"].get<std::string>();

        }
- Line 127: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Optional fields

        if (cursor_data.contains("order_value") && !cursor_data["order_value"].is_null()) {

            info.order_value = cursor_data["order_value"].get<std::string>();

        }

        

        if (cursor_data.contains("created_at")) {
- Line 131: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        if (cursor_data.contains("created_at")) {

            info.created_at = cursor_data["created_at"].get<int64_t>();

        }

        

        if (cursor_data.contains("version")) {
- Line 135: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        if (cursor_data.contains("version")) {

            info.version = cursor_data["version"].get<int>();

        }

        

        return info;

### utils/hkdf_cache.cpp
Total findings: 7

- Line 158: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& s : impl_->shards) {
- Line 159: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 165: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& s : impl_->shards) {
- Line 166: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 211: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& shard : impl_->shards) {
- Line 212: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(shard.mu);
- Line 217: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: size_t ikm_end = raw_key.find('\x00');

### utils/hkdf_helper.cpp
Total findings: 7

- Line 36: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 44: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_KDF_CTX_free(kctx);
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_KDF_CTX_free(kctx);
- Line 85: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(pctx);
- Line 112: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(pctx);
- Line 116: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_CTX_free(pctx);

### utils/zstd_codec.cpp
Total findings: 7

- Line 252: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 62: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_ERROR("Failed to allocate memory for compression: {}", e.what());
- Line 65: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fmt::format("Cannot allocate {} bytes for compression", max_size)
- Line 163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_ERROR("Failed to allocate memory for decompression: {}", e.what());
- Line 166: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fmt::format("Cannot allocate {} bytes for decompression", decompressed_size)
- Line 254: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 255: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### utils/checksum_utils.cpp
Total findings: 6

- Line 28: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 66: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // The deprecated signature is kept for backward-compatible callers that still
- Line 67: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // reference the old symbol (e.g. llm_deployment_plugin.cpp for legacy manifests).
- Line 41: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 49: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 52: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### utils/rate_limiter.cpp
Total findings: 6

- Line 53: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return false;

}



void RateLimiter::acquire(double tokens) {

    if (tokens <= 0.0) return;

    std::unique_lock<std::mutex> lk(mutex_);

    while (true) {
- Line 53: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void RateLimiter::acquire(double tokens) {
- Line 42: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool RateLimiter::try_acquire(double tokens) {
- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void RateLimiter::acquire(double tokens) {
- Line 69: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lk.lock();
- Line 73: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RateLimiter::reset() {

### utils/cron_parser.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2558 [scheduler] Full cron expre... (2026-03-12) | #1178 Verify and document
- Line 282: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (minutes_.find(tm.tm_min) == minutes_.end())      return false;
- Line 283: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (hours_.find(tm.tm_hour) == hours_.end())         return false;
- Line 341: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (minutes_.find(tm.tm_min) == minutes_.end()) {
- Line 345: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (hours_.find(tm.tm_hour) == hours_.end()) {

### utils/sampled_logger.cpp
Total findings: 5

- Line 92: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 131: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 58: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool SampledLogger::should_log(Logger::Level level, const char* file, int line) {
- Line 99: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void SampledLogger::log(Logger::Level level, const std::string& msg,
- Line 102: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: if (!should_log(level, file, line)) {

### utils/simd_distance.cpp
Total findings: 5

- Line 154: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 175: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 209: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 372: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
- Line 376: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {

### utils/bloom_filter.cpp
Total findings: 4

- Line 101: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void BloomFilter::clear() {
- Line 28: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: const double ln2 = std::log(2.0);
- Line 29: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (ln2 * ln2)));
- Line 35: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: size_t k = static_cast<size_t>(std::ceil(static_cast<double>(bits) / static_cast<double>(n) * std::log(2.0)));

### utils/lz4_codec.cpp
Total findings: 4

- Line 57: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fmt::format("Cannot allocate {} bytes for LZ4 output", bound));
- Line 106: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fmt::format("Cannot allocate {} bytes for LZ4 decompressed output", original_size));
- Line 134: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy API
- Line 43: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    }', '', '    const int src_size = static_cast<int>(size);', '    const int bound    = LZ4_compressBound(src_size);', '    if (bound <= 0) {']

### utils/ner_detection_engine.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2543 [utils] Upgrade PII detecti... (2026-03-11)
- Line 66: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sig = config["signature"];
- Line 511: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > first) value += ' ';
- Line 512: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > first) value += ' ';

### utils/retention_manager.cpp
Total findings: 4

- Line 422: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bg_thread_.join();
- Line 310: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // - legacy: retention_period_days/archive_after_days/auto_purge_enabled
- Line 391: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);
- Line 391: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);

### utils/stopwords.cpp
Total findings: 4

- Line 16: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static std::unordered_set<std::string> make_set(std::initializer_list<const char*> list) {
- Line 17: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> s;
- Line 45: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> Stopwords::merge(const std::unordered_set<std::string>& base,
- Line 47: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> out = base;

### utils/timestamp_utils.cpp
Total findings: 4

- Line 229: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // Seconds with millisecond fraction', '    oss << s.count();', '    if (ms_part.count() > 0) {', '        char ms_buf[8];']
- Line 89: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf),
- Line 101: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(ms_buf, sizeof(ms_buf), ".%03d", ms_part);
- Line 232: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(ms_buf, sizeof(ms_buf), ".%03lld", static_cast<long long>(ms_part.count()));

### utils/compression_metrics.cpp
Total findings: 3

- Line 86: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (stats.bytes_in / 1024.0 / 1024.0) << " MB)\n";
- Line 89: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (stats.bytes_out / 1024.0 / 1024.0) << " MB)\n";
- Line 97: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << stats.compression_throughput_mbps() << " MB/s\n";

### utils/pii_detection_engine.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4263 PKIClient v1.8.0 + PII Stre... (2026-03-15) | #998 C++ Audit: Eliminate
- Line 263: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sig_node = config["signature"];
- Line 359: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sig_node = config["signature"];

### utils/utils_adapters.cpp
Total findings: 3

- Line 84: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(result.categories.begin(), result.categories.end(), cat)
- Line 171: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
- Line 171: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {

### utils/consistent_hash.cpp
Total findings: 2

- Line 75: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
- Line 121: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: size_t ConsistentHashRing::nodeCount() const {

### utils/pii_stream_scanner.cpp
Total findings: 2

- Line 41: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cfg_.lookahead_bytes = engine_->maxPatternLength();
- Line 146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string hmac_key = cfg_.tenant_id + ":" + lek_mgr_->getCurrentLEK();

### utils/serialization.cpp
Total findings: 2

- Line 228: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    std::vector<float> vec(count);', '    const uint8_t* data = &data_[pos_];', '    std::memcpy(vec.data(), data, count * sizeof(float));', '    pos_ += count * sizeof(float);', '']
- Line 20: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: buffer_.reserve(1024); // Pre-allocate

### utils/update_checker.cpp
Total findings: 2

- Line 164: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: releases.push_back(*release);

### utils/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### utils/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### utils/runtime_license_gate.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4518 [WIP] Update developer docu... (2026-04-12) | #3408 Migrate Themis core

### utils/saga_logger.cpp
Total findings: 1

- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: batch_ids.push_back(j["batch_id"].get<std::string>());

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
