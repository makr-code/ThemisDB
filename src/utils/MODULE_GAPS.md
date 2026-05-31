# utils Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: utils
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 785
- Actionable Findings (Critical + High): 369
- Affected Files: 45

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 51 |
| High | 318 |
| Medium | 416 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 192 |
| performance_patterns | 141 |
| raii | 80 |
| reliability | 75 |
| platform | 58 |
| memory | 49 |
| security | 35 |
| exception_safety | 32 |
| observability | 31 |
| performance | 30 |
| concurrency | 20 |
| legacy_duplication | 14 |
| determinism | 9 |
| input_validation | 7 |
| audit_logging | 5 |
| uninitialized | 5 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/utils/memory/pool_allocator.cpp | 92 | 8 | 82 | 2 | 0 |
| src/utils/pki_client.cpp | 74 | 2 | 22 | 50 | 0 |
| src/utils/geo/ewkb.cpp | 70 | 0 | 9 | 61 | 0 |
| src/utils/audit_logger.cpp | 45 | 6 | 7 | 31 | 1 |
| src/utils/error_registry.cpp | 43 | 2 | 7 | 33 | 1 |
| src/utils/http_client_pool.cpp | 35 | 5 | 24 | 6 | 0 |
| src/utils/build_info.cpp | 31 | 0 | 2 | 29 | 0 |
| src/utils/capability_auto_generator.cpp | 24 | 0 | 13 | 10 | 1 |
| src/utils/pii_detector.cpp | 24 | 0 | 7 | 17 | 0 |
| src/utils/hkdf_helper.cpp | 22 | 0 | 12 | 10 | 0 |
| src/utils/input_validator.cpp | 21 | 0 | 3 | 18 | 0 |
| src/utils/lek_manager.cpp | 19 | 1 | 14 | 4 | 0 |
| src/utils/pii_detection_engine.cpp | 18 | 0 | 0 | 18 | 0 |
| src/utils/grpc_channel_pool.cpp | 17 | 1 | 16 | 0 | 0 |
| src/utils/regex_detection_engine.cpp | 15 | 0 | 5 | 10 | 0 |
| src/utils/pii_pseudonymizer.cpp | 14 | 6 | 4 | 4 | 0 |
| src/utils/self_awareness.cpp | 13 | 0 | 1 | 12 | 0 |
| src/utils/tracing.cpp | 13 | 4 | 2 | 7 | 0 |
| src/utils/hkdf_cache.cpp | 12 | 1 | 8 | 3 | 0 |
| src/utils/timestamp_utils.cpp | 12 | 0 | 11 | 1 | 0 |
| src/utils/saga_logger.cpp | 11 | 1 | 2 | 8 | 0 |
| src/utils/utils_adapters.cpp | 11 | 1 | 6 | 4 | 0 |
| src/utils/ner_detection_engine.cpp | 10 | 0 | 3 | 7 | 0 |
| src/utils/rate_limiter.cpp | 10 | 3 | 6 | 1 | 0 |
| src/utils/thread_pool_manager.cpp | 10 | 4 | 5 | 1 | 0 |
| src/utils/cursor.cpp | 9 | 0 | 2 | 7 | 0 |
| src/utils/logger.cpp | 9 | 0 | 2 | 4 | 3 |
| src/utils/normalizer.cpp | 9 | 0 | 0 | 9 | 0 |
| src/utils/serialization.cpp | 8 | 1 | 5 | 2 | 0 |
| src/utils/bloom_filter.cpp | 7 | 0 | 3 | 1 | 3 |
| src/utils/checksum_utils.cpp | 7 | 0 | 3 | 4 | 0 |
| src/utils/compression_metrics.cpp | 7 | 1 | 1 | 5 | 0 |
| src/utils/cron_parser.cpp | 7 | 0 | 1 | 6 | 0 |
| src/utils/retention_manager.cpp | 7 | 1 | 3 | 3 | 0 |
| src/utils/zstd_codec.cpp | 7 | 1 | 6 | 0 | 0 |
| src/utils/consistent_hash.cpp | 6 | 0 | 1 | 5 | 0 |
| src/utils/pii_stream_scanner.cpp | 6 | 2 | 2 | 2 | 0 |
| src/utils/simd_distance.cpp | 6 | 0 | 6 | 0 | 0 |
| src/utils/runtime_license_gate.cpp | 5 | 0 | 3 | 2 | 0 |
| src/utils/sampled_logger.cpp | 5 | 0 | 2 | 0 | 3 |
| src/utils/lz4_codec.cpp | 4 | 0 | 3 | 1 | 0 |
| src/utils/stopwords.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/update_checker.cpp | 3 | 0 | 1 | 2 | 0 |
| src/utils/boost_throw_exception.cpp | 2 | 0 | 2 | 0 | 0 |
| src/utils/file_utils.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/utils/memory/pool_allocator.cpp
Total findings: 92

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 60: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~BuddyAllocator() { /* cleanup */ }
  Context: class/struct BuddyAllocator
- Line 67: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~BuddyAllocator() { /* cleanup */ }
  Context: class/struct BuddyAllocator
- Line 342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (int i = static_cast<int>(impl_->max_order); i >= 0; --i) {
- Line 356: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SlabAllocator() { /* cleanup */ }
  Context: class/struct SlabAllocator
- Line 421: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SlabAllocator() { /* cleanup */ }
  Context: class/struct SlabAllocator
- Line 467: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 467: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    Impl(size_t cap) : capacity(cap), offset(0) {', '        memory = new uint8_t[capacity];', '        std::memset(memory, 0, capacity);', '        // Reserve space for allocation tracking to reduce reallocations']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    Impl(size_t cap) : capacity(cap), offset(0) {', '        memory = new uint8_t[capacity];', '        std::memset(memory, 0, capacity);', '        // Reserve space for allocation tracking to reduce reallocations']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // Initialize slab allocators for common sizes', '        for (size_t size : config.slab_sizes) {', '            slabs[size] = std::make_unique<SlabAllocator>(', '                size, config.slab_objects_per_slab, config.slab_max_slabs);', '        }']
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
- Line 74: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::vector<uintptr_t> free_list_heads;  // heads[i] is address of first free block at order i
- Line 195: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uintptr_t* list_ptr = &free_list_heads[order];
- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& current_block = blocks[*list_ptr];
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
- Line 232: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = impl_->allocateBlock(order);
- Line 251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* ptr = impl_->allocateBlock(order);
- Line 251: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* ptr = impl_->allocateBlock(order);
- Line 260: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(size);
- Line 274: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> BuddyAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->deallocateBlock(ptr);
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->deallocateBlock(ptr);
- Line 310: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uintptr_t addr = reinterpret_cast<uintptr_t>(impl_->memory);
- Line 310: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uintptr_t addr = reinterpret_cast<uintptr_t>(impl_->memory);
- Line 326: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uintptr_t block_addr = impl_->free_list_heads[i];
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("Slab allocation size overflow: object_size * object_count exceeds SIZE_MA
- Line 380: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: slab = nullptr;
  Context: delete slab;
- Line 451: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* allocate() {
- Line 451: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 455: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = slab->allocate();
- Line 455: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* ptr = slab->allocate();
- Line 455: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* ptr = slab->allocate();
- Line 472: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return new_slab->allocate();
- Line 475: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool deallocate(void* ptr) {
- Line 475: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 478: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (slab->contains(ptr)) {
- Line 479: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return slab->deallocate(ptr);
- Line 479: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return slab->deallocate(ptr);
- Line 494: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 494: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (size > impl_->object_size) {
- Line 507: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = impl_->allocate();
- Line 507: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* ptr = impl_->allocate();
- Line 507: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* ptr = impl_->allocate();
- Line 515: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(impl_->object_size);
- Line 515: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_.bytes_allocated.fetch_add(impl_->object_size);
- Line 529: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> SlabAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!impl_->deallocate(ptr)) {
- Line 542: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_.bytes_freed.fetch_add(impl_->object_size);
- Line 554: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: slab = nullptr;
  Context: delete slab;
- Line 558: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->head_slab = nullptr;
- Line 558: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->head_slab = nullptr;
- Line 577: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t total_objects = impl_->slab_count * impl_->objects_per_slab;
- Line 582: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: used_objects += (slab->object_count - slab->free_count);
- Line 625: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 625: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* ptr = impl_->memory + impl_->offset;
- Line 642: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* ptr = impl_->memory + impl_->offset;
- Line 646: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->allocation_stack.push_back({reinterpret_cast<uintptr_t>(ptr), aligned_size});
- Line 646: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->allocation_stack.push_back({reinterpret_cast<uintptr_t>(ptr), aligned_size});
- Line 649: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(aligned_size);
- Line 663: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> StackAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 676: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uintptr_t last_addr = impl_->allocation_stack.back().first;
- Line 676: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uintptr_t last_addr = impl_->allocation_stack.back().first;
- Line 708: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 713: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 735: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uintptr_t addr = impl_->allocation_stack.back().first;
- Line 735: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uintptr_t addr = impl_->allocation_stack.back().first;
- Line 736: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t alloc_offset = addr - reinterpret_cast<uintptr_t>(impl_->memory);
- Line 736: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: size_t alloc_offset = addr - reinterpret_cast<uintptr_t>(impl_->memory);
- Line 812: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
- Line 812: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 814: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto result = allocator->allocate(size, hint);
- Line 819: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->ownership[reinterpret_cast<uintptr_t>(ptr)] = allocator;
- Line 825: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
- Line 825: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 833: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = impl_->ownership.find(reinterpret_cast<uintptr_t>(ptr));
- Line 833: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto it = impl_->ownership.find(reinterpret_cast<uintptr_t>(ptr));
- Line 842: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return allocator->deallocate(ptr);
- Line 842: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return allocator->deallocate(ptr);
- Line 899: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [size, slab] : impl_->slabs) {
  Confidence: band=very_high; score=0.9
- Line 446: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete slab;
- Line 554: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete slab;

### src/utils/pki_client.cpp
Total findings: 74

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    int len = static_cast<int>(pass->size());', '    if (len > size) len = size;', '    std::memcpy(buf, pass->data(), len);', '    return len;', '}']
  Confidence: band=very_high; score=0.93
- Line 202: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int len = static_cast<int>(pass->size());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 65: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] SSL_CTX* ssl_ctx = ssl ? SSL_get_SSL_CTX(ssl) : nullptr;
- Line 332: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!bptr || !bptr->data || bptr->length == 0) return {};
- Line 333: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return std::string(bptr->data, bptr->length);
- Line 363: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 363: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 365: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, real_size);
- Line 365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, real_size);
- Line 534: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 534: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 537: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out->append(ptr, real_size);
- Line 537: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->append(ptr, real_size);
- Line 601: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY signing (preferred) instead of deprecated RSA_sign API.
  Confidence: band=high; score=0.8
- Line 609: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use PKCS#1 v1.5 padding for compatibility with RSA_sign
  Confidence: band=high; score=0.8
- Line 753: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 756: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out->append(ptr, real_size);
- Line 756: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->append(ptr, real_size);
- Line 821: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY verification instead of deprecated RSA_verify
  Confidence: band=high; score=0.8
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[n & 63]);
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
- Line 192: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (alg.find("SHA256") != std::string::npos) { expected_len = 32; return NID_sha256; }
- Line 242: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (hex) OPENSSL_free(hex);
- Line 488: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(*pub_result);
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_
- Line 553: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
- Line 553: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 563: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" <<
- Line 563: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" <<
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
- Line 584: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "
- Line 584: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "
- Line 618: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(*pub_result);
- Line 626: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 629: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 677: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 680: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 770: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << res
- Line 770: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << res
- Line 771: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
- Line 771: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
- Line 778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 780: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code="
- Line 780: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code="
- Line 789: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
- Line 789: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
- Line 796: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body <<
- Line 796: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body <<
- Line 830: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 831: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pub);
- Line 834: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 836: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pub);

### src/utils/geo/ewkb.cpp
Total findings: 70

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2520 [geo] Full GeoJSON RFC 7946 parsing + in-memory R-tree spatial index (2026-03-11T21:41:0
- Line 83: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WKT: Invalid coordinate token: " + token);
- Line 95: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WKT: Missing coordinate body");
- Line 242: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t temp[sizeof(double)];
- Line 244: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: temp[i] = ptr[sizeof(double) - 1 - i];
- Line 257: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t temp[sizeof(uint32_t)];
- Line 259: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: temp[i] = ptr[sizeof(uint32_t) - 1 - i];
- Line 501: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "GeoJSON: longitude " + std::to_string(lon) + " is out of WGS84 range [-180, 180]");
- Line 505: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "GeoJSON: latitude " + std::to_string(lat) + " is out of WGS84 range [-90, 90]");
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.push_back(c);
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current.push_back(c);
- Line 61: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current.push_back(c);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(trimCopy(current));
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current.push_back(c);
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(trimCopy(current));
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(bytes[i]);
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(bytes[i]);
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.rings[r].emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
- Line 425: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(std::move(pt));
- Line 593: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(std::move(ls));
- Line 617: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.rings.push_back(std::move(ring_coords));
- Line 645: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pg.rings.push_back(std::move(ring_coords));
- Line 647: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(std::move(pg));
- Line 653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeoJSONGeomImpl(member, depth - 1));
  Confidence: band=high; score=0.74
- Line 654: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.geometries.push_back(parseGeoJSONGeomImpl(member, depth - 1));
- Line 693: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.coords.push_back(parseCoordinateToken(body));
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.coords.push_back(parseCoordinateToken(token));
- Line 723: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: merged += ",";
- Line 732: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_groups.push_back(trimCopy(merged));
- Line 745: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 746: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords.push_back(parseCoordinateToken(token));
- Line 748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.rings.push_back(std::move(coords));
- Line 842: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 843: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
- Line 845: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y});
- Line 853: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
- Line 856: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y});
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
- Line 869: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: line_coords.push_back({c.x, c.y});
- Line 872: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines_arr.push_back(line_coords);
- Line 881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
- Line 884: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y});
- Line 887: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rings_arr.push_back(ring_coords);
- Line 898: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 898: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 898: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 899: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
- Line 901: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y});
- Line 904: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rings_arr.push_back(ring_coords);
- Line 906: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: polys_arr.push_back(rings_arr);
- Line 912: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(json::parse(toGeoJSON(sub)));
  Confidence: band=high; score=0.74
- Line 913: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: members.push_back(json::parse(toGeoJSON(sub)));
- Line 940: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/utils/audit_logger.cpp
Total findings: 45

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 165: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(cfg_.log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 191: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd2 = ::open(cfg_.secondary_log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 267: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto blob = enc_->encrypt(plain, cfg_.key_id);
- Line 1554: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(cfg_.chain_head_path.c_str(), O_RDONLY);
- Line 1599: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void HashChainAuditWriter::write(nlohmann::json record) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Track per-user counts', '            std::string user = payload.value("user_id", payload.value("user", std::string{"system"}));', '            user_counts[user] = user_counts.value(user, 0) + 1;', '', '        } catch (const nlohmann::json::exception &) {']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 595: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto b : hash_bytes) {
  Confidence: band=very_high; score=0.9
- Line 652: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sock = socket(AF_INET, SOCK_DGRAM, 0);
- Line 765: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This method is kept for API compatibility
  Confidence: band=high; score=0.8
- Line 904: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : kept_entries) {
  Confidence: band=very_high; score=0.9
- Line 1135: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (baseline.avg_frequency_seconds == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[n & 63]);
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 18) & 63]);
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 12) & 63]);
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>((val >> valb) & 0xFF));
- Line 168: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 194: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd2);
- Line 507: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = state["last_timestamp_ms"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(sock);
- Line 796: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 845: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 850: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kept_entries.push_back(line);
- Line 875: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ifs.close();
- Line 895: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: archive_ofs.close();
- Line 907: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: main_ofs.close();
- Line 943: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 948: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kept_entries.push_back(line);
- Line 974: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ifs.close();
- Line 991: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: main_ofs.close();
- Line 1319: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 1409: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 1557: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 644: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: formatted_message = formatAsSyslog(event, event_type);
  Confidence: band=medium; score=0.6

### src/utils/error_registry.cpp
Total findings: 43

- Line 142: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
- Line 1721: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = category_index_.find(category);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 460: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "cudaMalloc or hipMalloc failed to allocate GPU memory.",
- Line 892: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=very_high; score=0.9
- Line 1324: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Failed to allocate memory for compression/decompression operation.",
- Line 1353: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Failed to allocate memory from the system.",
- Line 1693: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: errors_[code_value] = metadata;
- Line 1694: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: category_index_[metadata.category].push_back(code_value);
- Line 88: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check file/directory permissions\n"
- Line 252: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify tar/gzip availability\n"
- Line 347: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Free up system memory (RAM/VRAM)\n"
- Line 448: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure CUDA/ROCm runtime is available\n"
- Line 478: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Review NVIDIA/AMD documentation for peer access requirements\n"
- Line 563: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure adapter format is supported (safetensors/GGUF)\n"
- Line 623: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "6. Use mixed precision training (fp16/bf16)\n"
- Line 664: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. List available tools with: GET /api/v1/mcp/tools\n"
- Line 692: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check if stdin/stdout are available\n"
- Line 694: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure not running in detached/daemon mode\n"
- Line 851: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Verify table/field names exist\n"
- Line 863: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check query syntax matches AQL/GraphQL specification\n"
- Line 864: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify all brackets/quotes are balanced\n"
- Line 877: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check if referenced tables/indexes exist\n"
- Line 892: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=high; score=0.74
- Line 931: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check window frame bounds (ROWS/RANGE)\n"
- Line 944: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Reduce result set size with additional FILTER/LIMIT clauses\n"
- Line 1000: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Verify request format (JSON/GraphQL/etc.)\n"
- Line 1184: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Verify entropy source is available (/dev/urandom)\n"
- Line 1241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check user/process permissions\n"
- Line 1242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify file/directory ownership\n"
- Line 1243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Review SELinux/AppArmor policies\n"
- Line 1478: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Ensure a node/edge is not both required and forbidden\n"
- Line 1694: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: category_index_[metadata.category].push_back(code_value);
- Line 1723: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(errors_.at(code));
  Confidence: band=high; score=0.74
- Line 1724: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(errors_.at(code));
- Line 1738: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1738: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1739: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second);
- Line 1750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(category);
  Confidence: band=high; score=0.74
- Line 1751: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: categories.push_back(category);
- Line 1763: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["errors"].push_back(pair.second.toJSON());
  Confidence: band=high; score=0.74
- Line 1764: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["errors"].push_back(pair.second.toJSON());
- Line 142: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
  Confidence: band=medium; score=0.6

### src/utils/http_client_pool.cpp
Total findings: 35

- Line 92: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 460: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(stream, req);
- Line 465: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(stream, buffer, res);
- Line 514: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::write(stream, req);
- Line 519: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: http::read(stream, buffer, res);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Use striped locking to reduce contention', '    size_t stripe_idx = getStripeIndex();', '    auto& stripe = stripes_[stripe_idx];', '', '    std::unique_lock<std::mutex> lock(stripe->mutex);']
  Confidence: band=high; score=0.81
- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid URL format: " + url);
- Line 174: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<HTTPClient> HTTPClientPool::acquireConnection() {
- Line 222: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = now + config_.acquire_timeout;
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HTTPClientPool is shutting down");
- Line 242: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Timeout waiting for available connection");
- Line 249: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 270: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 276: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 278: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& conn : stripe->connections) {
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 300: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 332: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < stripes_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 396: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(http::verb::post, url, body.dump(), headers);
- Line 403: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(http::verb::get, url, "", headers);
- Line 406: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: HTTPResponse BeastHTTPClient::execute(
- Line 434: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: beast::get_lowest_layer(stream).connect(results);
- Line 531: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stream.socket().shutdown(tcp::socket::shutdown_both, ec);
- Line 537: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HTTP request failed: " + std::string(e.what()));
- Line 29: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex url_regex(R"((https?)://([^:/]+)(?::(\d+))?(/.*)?)", std::regex::icase);
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stripes_.push_back(std::make_unique<LockStripe>());
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe->connections.push_back(pooled);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stripe->connections.push_back(pooled);
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stripe->connections.push_back(pooled);

### src/utils/build_info.cpp
Total findings: 31

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 536: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This legacy file is no longer compiled; the note is kept for reference.
  Confidence: band=high; score=0.8
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 462: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 469: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 485: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 494: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 510: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 517: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 849: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 859: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 860: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 927: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/utils/capability_auto_generator.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 85: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to load capability auto-gen config: " + std::string(e.what()));
- Line 154: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::seconds(60));
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(60));
- Line 258: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
- Line 264: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
- Line 334: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(sorted_keywords.size(), (size_t)config_.max_keywords); ++i) {
- Line 372: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& domain : previous_capability->domains) {
  Confidence: band=very_high; score=0.9
- Line 373: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(capability.domains.begin(), capability.domains.end(), domain) == capability.domains.en
- Line 642: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [shard_id, err] : corrupt_entries) {
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto analysis = root["rocksdb_analysis"];
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto audit = root["audit"];
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto security = root["security"];
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto output = root["output"];
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data_types.push_back(type);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.data_types.push_back(type);
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.keywords.push_back(sorted_keywords[i].first);
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.keywords.push_back(sorted_keywords[i].first);
- Line 373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capability.domains.push_back(domain);
  Confidence: band=high; score=0.74
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: capability.domains.push_back(domain);
- Line 537: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::ofstream log(config_.audit_log_path, std::ios::app);
  Confidence: band=medium; score=0.6

### src/utils/pii_detector.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 164: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& engine : engines_) {
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& engine : engines_) {
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& engine : engines_) {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use ConfigPathResolver to handle both new and legacy paths
  Confidence: band=high; score=0.8
- Line 31: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back(static_cast<char>(std::tolower(ch)));
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: normalized.push_back(static_cast<char>(std::tolower(ch)));
- Line 152: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> PIIDetector::detectInJson(
  Confidence: band=medium; score=0.66
- Line 155: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> result;
  Confidence: band=medium; score=0.66
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: enabled.push_back(engine->getName());
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: enabled.push_back(engine->getName());
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.at("engines").push_back(engine->getMetadata());
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata.at("engines").push_back(engine->getMetadata());
- Line 277: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["global_settings"];
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_node.push_back(item_json);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: json_node.push_back(item_json);
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: engines_.push_back(std::move(ner_engine));
- Line 492: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>>& findings) const {
  Confidence: band=medium; score=0.66
- Line 510: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path_it->second.push_back(std::move(finding));
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path_it->second.push_back(std::move(finding));
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduplicated.push_back(curr);
  Confidence: band=high; score=0.74
- Line 582: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deduplicated.push_back(curr);

### src/utils/hkdf_helper.cpp
Total findings: 22

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_KDF_fetch failed");
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_KDF_CTX_new failed");
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_KDF_derive failed");
- Line 78: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_CTX_new_id failed");
- Line 83: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_derive_init failed");
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_CTX_set_hkdf_md failed");
- Line 94: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_CTX_set1_hkdf_salt failed");
- Line 100: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_CTX_set1_hkdf_key failed");
- Line 108: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_CTX_add1_hkdf_info failed");
- Line 115: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EVP_PKEY_derive failed");
- Line 39: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_free(kdf);
- Line 68: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_CTX_free(kctx);
- Line 72: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_CTX_free(kctx);
- Line 82: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 87: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 93: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 99: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 107: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 118: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);

### src/utils/input_validator.cpp
Total findings: 21

- Line 343: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static const std::array<bool, 256> tbl = [] {
- Line 345: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (unsigned char c : std::string(";|&`$(){}[]<>!?*\"\\'")) {
- Line 619: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // if the string is ever passed as a printf format argument.
  Confidence: band=very_high; score=0.9
- Line 36: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 36: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 36: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!isAsciiControl(c)) out.push_back(c);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!isAsciiControl(c)) out.push_back(c);
- Line 133: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto min_len = prop["minLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto max_len = prop["maxLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = schema["properties"].begin();
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static const char* forbidden[] = { "drop ", "truncate ", "alter ", "grant ", "revoke ", "create tabl
- Line 353: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool InputValidator::validateAQLQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: " truncate ", " insert ", " update ", " delete ",
- Line 448: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  result += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  result += "&amp;";  break;
- Line 450: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  result += "&lt;";   break;
- Line 451: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  result += "&gt;";   break;
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  result += "&quot;"; break;
- Line 453: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': result += "&#x27;"; break;
- Line 454: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '/':  result += "&#x2F;"; break;

### src/utils/lek_manager.cpp
Total findings: 19

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create KEK in key provider");
- Line 120: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to generate random LEK");
- Line 133: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to persist encrypted LEK in RocksDB");
- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to register generated LEK in key provider");
- Line 195: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: rotated = nullptr;
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 304: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("max_age_days must be >= 1");
- Line 341: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(rotation_cv_mu_);
- Line 342: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: bool stopped = rotation_cv_.wait_for(
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [cached_date, key_id] : lek_cache_) {
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& expired_date : to_revoke) {
  Confidence: band=very_high; score=0.9
- Line 195: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_revoke.push_back(cached_date);

### src/utils/pii_detection_engine.cpp
Total findings: 18

- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hash_bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else { out.push_back(c); }
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(c);
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else { out.push_back(c); }
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(' '); // normalize separator to space
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(c);
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('*');
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(c);
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(c);
- Line 265: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74

### src/utils/grpc_channel_pool.cpp
Total findings: 17

- Line 86: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 32: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<grpc::Channel> GrpcChannelPool::acquireChannel(
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Channel pool is shutting down");
- Line 44: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 75: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // No available channels, check if we can create new one
- Line 97: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Timeout acquiring gRPC channel for target: " + target);
- Line 142: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 145: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 149: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [ch, pooled_ch] : pool->all_channels) {
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 164: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pool->available = std::queue<std::shared_ptr<PooledChannel>>();
- Line 164: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: pool->available = std::queue<std::shared_ptr<PooledChannel>>();
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);

### src/utils/regex_detection_engine.cpp
Total findings: 15

- Line 45: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 197: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [hint, type] : field_name_hints_) {
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["engine_type"] = "regex";
- Line 227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["version"] = signature_.version;
- Line 228: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["enabled"] = enabled_;
- Line 69: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["settings"];
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back(finding);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: findings.push_back(finding);
- Line 359: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: url.regex_str = R"(https?://[^\s]+)";
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: flag_strings.push_back(flag.get<std::string>());
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern.field_hints.push_back(hint.get<std::string>());
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern.field_hints.push_back(hint.get<std::string>());

### src/utils/pii_pseudonymizer.cpp
Total findings: 14

- Line 54: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 6
  Remediation: Fix loop condition or increase array size
  Context: bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
- Line 55: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 8
  Remediation: Fix loop condition or increase array size
  Context: bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 10
- Line 81: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto findings_map = detector_->detectInJson(data);
- Line 143: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto mapping_str = db_->get(dbKey(pii_uuid));
- Line 190: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto mapping_opt = txn->get(dbKey(pii_uuid));
- Line 237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto mapping_opt = txn->get(dbKey(pii_uuid));
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to generate random key for PII mapping");
- Line 50: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to generate random UUID");
- Line 60: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < 16; ++i) {
  Confidence: band=very_high; score=0.9
- Line 81: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto findings_map = detector_->detectInJson(data);
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: created_uuids.push_back(pii_uuid);
- Line 157: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto encrypted_json = mapping["original_value_encrypted"];
  Confidence: band=high; score=0.74

### src/utils/self_awareness.cpp
Total findings: 13

- Line 597: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.snapshot_directory)) {
- Line 43: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sa = root["self_awareness"];
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto thresh = sa["thresholds"];
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto snaps = sa["snapshots"];
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshots_.push_back(snapshot);
- Line 195: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MEMORYSTATUSEX mem_status;
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("CRITICAL: CPU usage at " +
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("WARNING: CPU usage at " +
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("WARNING: High average query time: " +
- Line 595: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Collect snapshot files sorted by name (which encodes timestamp)
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());
- Line 639: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshots_.push_back(std::move(s));

### src/utils/tracing.cpp
Total findings: 13

- Line 147: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = thread_baggage_.find(key);
- Line 192: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eq may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eq = token.find('=');
- Line 473: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 2
  Remediation: Fix loop condition or increase array size
  Context: if (value[2] != '-' || value[35] != '-' || value[52] != '-') return false;
- Line 489: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (!hexByte(value[0], value[1], ver) || ver != 0) return false;
- Line 254: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket socket(io);
- Line 255: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket.connect(*results.begin(), ec);
- Line 166: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!out.empty()) out += ',';
- Line 172: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::inject(std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::extract(const std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex re(R"((?:http|https)://([^/:]+)(?::(\d+))?)", std::regex::icase);
- Line 274: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.name", serviceName},
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.version", "0.1.0"}
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::string headerValue(const std::map<std::string, std::string>& headers,
  Confidence: band=high; score=0.74

### src/utils/hkdf_cache.cpp
Total findings: 12

- Line 92: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = map.find(tail);
- Line 160: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 167: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 170: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [key, pair] : s.map) {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& shard : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(shard.mu);
- Line 219: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: size_t ikm_end = raw_key.find('\x00');
  Confidence: band=very_high; score=0.9
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: k.push_back('\x00');
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: k.push_back('\x00');
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: k.push_back('\x00');

### src/utils/timestamp_utils.cpp
Total findings: 12

- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TimestampUtils::parse: unexpected end of string");
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TimestampUtils::parse: field width exceeds safe limit");
- Line 46: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(std::string("TimestampUtils::parse: expected digit, got '") + c + "'");
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 91: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf),
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(ms_buf, sizeof(ms_buf), ".%03d", ms_part);
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(std::string("TimestampUtils::parse: unexpected character '") + tz + "' i
- Line 165: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TimestampUtils::parse: trailing characters in timestamp");
- Line 172: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TimestampUtils::parse: field out of range");
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TimestampUtils::parse: invalid date/time values");
- Line 234: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(ms_buf, sizeof(ms_buf), ".%03lld", static_cast<long long>(ms_part.count()));
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['', '    // Seconds with millisecond fraction', '    oss << s.count();', '    if (ms_part.count() > 0) {', '        char ms_buf[8];']
  Confidence: band=medium; score=0.65

### src/utils/saga_logger.cpp
Total findings: 11

- Line 206: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto blob = enc_->encrypt(plaintext, cfg_.key_id);
- Line 61: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto get_str = [](const nlohmann::json& obj, const char* key) -> std::string {
- Line 65: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto get_i64 = [](const nlohmann::json& obj, const char* key) -> int64_t {
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_array.push_back(entry);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_array.push_back(entry);
- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step);
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(step);
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_ids.push_back(j["batch_id"].get<std::string>());
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: retained_lines.push_back(line);
- Line 457: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: archive_lines.push_back(line);
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: retained_lines.push_back(line);

### src/utils/utils_adapters.cpp
Total findings: 11

- Line 155: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return writer_.sequenceNumber() - 1; // write() post-increments seq
- Line 86: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.categories.begin(), result.categories.end(), cat)
- Line 173: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
- Line 173: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : stages_) snapshot.push_back(s.get());
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (IUtilsStage* stage : snapshot) {
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!stage->execute()) {
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.categories.push_back(cat);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.categories.push_back(cat);
- Line 173: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : stages_) snapshot.push_back(s.get());

### src/utils/ner_detection_engine.cpp
Total findings: 10

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2543 [utils] Upgrade PII detection to NER engine + audit fixes (2026-03-11T21:44:14Z)
- Line 47: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 165: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [hint, type] : field_name_hints_) {
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(t));
- Line 375: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, i, name_end - 1, PIIType::PERSON_NAME,
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, i, i, PIIType::ORGANIZATION,
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, loc_start, loc_end - 1,
- Line 513: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > first) value += ' ';
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > first) value += ' ';

### src/utils/rate_limiter.cpp
Total findings: 10

- Line 44: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool RateLimiter::try_acquire(double tokens) {
- Line 55: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void RateLimiter::acquire(double tokens) {
- Line 71: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lk.lock();
- Line 27: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (rate_per_second <= 0.0) throw std::invalid_argument("RateLimiter: rate must be positive");
- Line 28: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (burst_size <= 0.0)      throw std::invalid_argument("RateLimiter: burst_size must be positive");
- Line 44: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool RateLimiter::try_acquire(double tokens) {
- Line 55: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void RateLimiter::acquire(double tokens) {
- Line 70: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(sleep_dur);
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (rate_per_second <= 0.0) throw std::invalid_argument("RateLimiter: rate must be positive");
- Line 75: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/utils/thread_pool_manager.cpp
Total findings: 10

- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.io_stats = io_pool_->getStatistics();
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.cpu_stats = cpu_pool_->getStatistics();
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.blocking_stats = blocking_pool_->getStatistics();
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.blocking_stats = blocking_pool_->getStatistics();
- Line 29: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < config.min_threads; i++) {
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::shared_mutex> lock(mutex_);
- Line 46: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: bool available = cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
- Line 63: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: task->execute();
- Line 128: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this]() { workerLoop(); });
  Confidence: band=high; score=0.74

### src/utils/cursor.cpp
Total findings: 9

- Line 124: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: info.pk = cursor_data["pk"].get<std::string>();
- Line 125: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: info.collection = cursor_data["collection"].get<std::string>();
- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(base64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(base64_chars[(val >> valb) & 0x3F]);
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back('=');
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(char((val >> valb) & 0xFF));

### src/utils/logger.cpp
Total findings: 9

- Line 53: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 47: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 53: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 97: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 125: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 155: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6

### src/utils/normalizer.cpp
Total findings: 9

- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is2(c, d, 0xC3, 0xA4)) { out.push_back('a'); i += 2; continue; } // ä
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0xA4)) { out.push_back('a'); i += 2; continue; } // ä
- Line 36: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0xB6)) { out.push_back('o'); i += 2; continue; } // ö
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0xBC)) { out.push_back('u'); i += 2; continue; } // ü
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0x84)) { out.push_back('A'); i += 2; continue; } // Ä
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0x96)) { out.push_back('O'); i += 2; continue; } // Ö
- Line 40: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0x9C)) { out.push_back('U'); i += 2; continue; } // Ü
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (is2(c, d, 0xC3, 0x9F)) { out.push_back('s'); out.push_back('s'); i += 2; continue; } // ß -> ss
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(c));

### src/utils/serialization.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    std::vector<float> vec(count);', '    const uint8_t* data = &data_[pos_];', '    std::memcpy(vec.data(), data, count * sizeof(float));', '    pos_ += count * sizeof(float);', '']
  Confidence: band=very_high; score=0.93
- Line 22: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: buffer_.reserve(1024); // Pre-allocate
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value |= static_cast<uint32_t>(data_[pos_++]) << (i * 8);
- Line 158: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value |= static_cast<uint64_t>(data_[pos_++]) << (i * 8);
- Line 211: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string str(reinterpret_cast<const char*>(&data_[pos_]), size);
- Line 229: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t* data = &data_[pos_];
- Line 37: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_.push_back((value >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buffer_.push_back((value >> (i * 8)) & 0xFF);

### src/utils/bloom_filter.cpp
Total findings: 7

- Line 45: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("BloomFilter: false_positive_rate must be in (0, 1)");
- Line 87: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < num_hashes_; ++i) {
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < num_hashes_; ++i) {
  Confidence: band=very_high; score=0.9
- Line 103: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Context: void BloomFilter::clear() {
  Confidence: band=medium; score=0.56
- Line 30: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double ln2 = std::log(2.0);
  Confidence: band=medium; score=0.6
- Line 31: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (ln2 * ln2)));
  Confidence: band=medium; score=0.6
- Line 37: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: size_t k = static_cast<size_t>(std::ceil(static_cast<double>(bits) / static_cast<double>(n) * std::log(2.0)));
  Confidence: band=medium; score=0.6

### src/utils/checksum_utils.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 68: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The deprecated signature is kept for backward-compatible callers that still
  Confidence: band=high; score=0.8
- Line 69: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // reference the old symbol (e.g. llm_deployment_plugin.cpp for legacy manifests).
  Confidence: band=high; score=0.8
- Line 34: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 43: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 51: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 54: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/utils/compression_metrics.cpp
Total findings: 7

- Line 49: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = stats_.find(method);
- Line 57: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : stats_) {
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: methods.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: methods.push_back(pair.first);
- Line 88: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (stats.bytes_in / 1024.0 / 1024.0) << " MB)\n";
- Line 91: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (stats.bytes_out / 1024.0 / 1024.0) << " MB)\n";
- Line 99: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << stats.compression_throughput_mbps() << " MB/s\n";

### src/utils/cron_parser.cpp
Total findings: 7

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2558 [scheduler] Full cron expression parsing (v1.5.0) + scientific refe... (2026-03-12T05:51
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(field);
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(field);
- Line 284: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (minutes_.find(tm.tm_min) == minutes_.end())      return false;
- Line 285: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (hours_.find(tm.tm_hour) == hours_.end())         return false;
- Line 343: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (minutes_.find(tm.tm_min) == minutes_.end()) {
- Line 347: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (hours_.find(tm.tm_hour) == hours_.end()) {

### src/utils/retention_manager.cpp
Total findings: 7

- Line 58: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = policies_.find(policy_name);
- Line 312: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // - legacy: retention_period_days/archive_after_days/auto_purge_enabled
  Confidence: band=high; score=0.8
- Line 393: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);
- Line 394: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: bool stopped = bg_cv_.wait_for(lk, interval, [this]{ return bg_stop_; });
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(policy);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(policy);
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: action_history_.push_back(action);

### src/utils/zstd_codec.cpp
Total findings: 7

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 64: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Failed to allocate memory for compression: {}", e.what());
- Line 67: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for compression", max_size)
- Line 165: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Failed to allocate memory for decompression: {}", e.what());
- Line 168: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for decompression", decompressed_size)

### src/utils/consistent_hash.cpp
Total findings: 6

- Line 59: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < virtual_nodes_; ++i) {
  Confidence: band=very_high; score=0.9
- Line 77: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(node);
- Line 123: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Context: size_t ConsistentHashRing::nodeCount() const {
  Confidence: band=medium; score=0.56

### src/utils/pii_stream_scanner.cpp
Total findings: 6

- Line 43: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cfg_.lookahead_bytes = engine_->maxPatternLength();
- Line 148: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string hmac_key = cfg_.tenant_id + ":" + lek_mgr_->getCurrentLEK();
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!engine_) throw std::invalid_argument("PIIStreamScanner: engine must not be null");
- Line 136: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!lek_mgr_) throw std::invalid_argument("PIIStreamPseudonymizer: lek_mgr must not be null");
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(f));
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(f));

### src/utils/simd_distance.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 374: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
- Line 378: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
- Line 432: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: results[i] = 1.0f - cosine_distance(query, database + i * dim, dim);

### src/utils/runtime_license_gate.cpp
Total findings: 5

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4518 [WIP] Update developer documentation to match current source code (2026-04-12T20:32:47Z)
- Line 134: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 139: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 173: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: msg << "license has been " << status << ".";
- Line 179: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: msg << "license status is '" << status << "'.";

### src/utils/sampled_logger.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 60: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: bool SampledLogger::should_log(Logger::Level level, const char* file, int line) {
  Confidence: band=medium; score=0.6
- Line 101: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void SampledLogger::log(Logger::Level level, const std::string& msg,
  Confidence: band=medium; score=0.6
- Line 104: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!should_log(level, file, line)) {
  Confidence: band=medium; score=0.6

### src/utils/lz4_codec.cpp
Total findings: 4

- Line 59: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for LZ4 output", bound));
- Line 108: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for LZ4 decompressed output", original_size));
- Line 136: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy API
  Confidence: band=high; score=0.8
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    }', '', '    const int src_size = static_cast<int>(size);', '    const int bound    = LZ4_compressBound(src_size);', '    if (bound <= 0) {']
  Confidence: band=medium; score=0.65

### src/utils/stopwords.cpp
Total findings: 4

- Line 18: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static std::unordered_set<std::string> make_set(std::initializer_list<const char*> list) {
  Confidence: band=medium; score=0.66
- Line 19: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> s;
  Confidence: band=medium; score=0.66
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> Stopwords::merge(const std::unordered_set<std::string>& base,
  Confidence: band=medium; score=0.66
- Line 49: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> out = base;
  Confidence: band=medium; score=0.66

### src/utils/update_checker.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: releases.push_back(*release);
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: releases.push_back(*release);

### src/utils/boost_throw_exception.cpp
Total findings: 2

- Line 21: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw e;
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw e;

### src/utils/file_utils.cpp
Total findings: 1

- Line 26: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open file: " + path);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
