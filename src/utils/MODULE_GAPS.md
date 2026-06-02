# utils Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: utils
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 516
- Actionable Findings (Critical + High): 212
- Affected Files: 44

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 31 |
| High | 181 |
| Medium | 292 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 115 |
| container | 94 |
| raii | 64 |
| platform | 58 |
| exception_safety | 32 |
| observability | 31 |
| performance | 30 |
| reliability | 25 |
| memory | 22 |
| legacy_duplication | 14 |
| concurrency | 11 |
| determinism | 9 |
| input_validation | 7 |
| audit_logging | 5 |
| uninitialized | 5 |
| security | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/utils/memory/pool_allocator.cpp | 51 | 7 | 43 | 1 | 0 |
| src/utils/pki_client.cpp | 50 | 2 | 11 | 37 | 0 |
| src/utils/geo/ewkb.cpp | 46 | 0 | 5 | 41 | 0 |
| src/utils/error_registry.cpp | 40 | 1 | 7 | 31 | 1 |
| src/utils/build_info.cpp | 31 | 0 | 2 | 29 | 0 |
| src/utils/audit_logger.cpp | 24 | 5 | 5 | 13 | 1 |
| src/utils/http_client_pool.cpp | 23 | 1 | 19 | 3 | 0 |
| src/utils/input_validator.cpp | 17 | 0 | 1 | 16 | 0 |
| src/utils/capability_auto_generator.cpp | 15 | 0 | 6 | 8 | 1 |
| src/utils/grpc_channel_pool.cpp | 15 | 1 | 14 | 0 | 0 |
| src/utils/pii_detector.cpp | 15 | 0 | 3 | 12 | 0 |
| src/utils/regex_detection_engine.cpp | 12 | 0 | 3 | 9 | 0 |
| src/utils/pii_detection_engine.cpp | 11 | 0 | 1 | 10 | 0 |
| src/utils/self_awareness.cpp | 11 | 0 | 1 | 10 | 0 |
| src/utils/tracing.cpp | 11 | 2 | 2 | 7 | 0 |
| src/utils/lek_manager.cpp | 10 | 1 | 7 | 2 | 0 |
| src/utils/logger.cpp | 9 | 0 | 2 | 4 | 3 |
| src/utils/thread_pool_manager.cpp | 9 | 4 | 4 | 1 | 0 |
| src/utils/cursor.cpp | 7 | 0 | 0 | 7 | 0 |
| src/utils/hkdf_cache.cpp | 7 | 0 | 7 | 0 | 0 |
| src/utils/hkdf_helper.cpp | 7 | 0 | 2 | 5 | 0 |
| src/utils/ner_detection_engine.cpp | 7 | 0 | 1 | 6 | 0 |
| src/utils/zstd_codec.cpp | 7 | 1 | 6 | 0 | 0 |
| src/utils/pii_pseudonymizer.cpp | 6 | 2 | 1 | 3 | 0 |
| src/utils/serialization.cpp | 6 | 1 | 4 | 1 | 0 |
| src/utils/utils_adapters.cpp | 6 | 0 | 4 | 2 | 0 |
| src/utils/checksum_utils.cpp | 5 | 0 | 2 | 3 | 0 |
| src/utils/cron_parser.cpp | 5 | 0 | 1 | 4 | 0 |
| src/utils/sampled_logger.cpp | 5 | 0 | 2 | 0 | 3 |
| src/utils/simd_distance.cpp | 5 | 0 | 5 | 0 | 0 |
| src/utils/bloom_filter.cpp | 4 | 0 | 0 | 1 | 3 |
| src/utils/compression_metrics.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/consistent_hash.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/rate_limiter.cpp | 4 | 1 | 2 | 1 | 0 |
| src/utils/stopwords.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/lz4_codec.cpp | 3 | 0 | 2 | 1 | 0 |
| src/utils/pii_stream_scanner.cpp | 3 | 2 | 0 | 1 | 0 |
| src/utils/retention_manager.cpp | 3 | 0 | 2 | 1 | 0 |
| src/utils/runtime_license_gate.cpp | 3 | 0 | 1 | 2 | 0 |
| src/utils/saga_logger.cpp | 3 | 0 | 0 | 3 | 0 |
| src/utils/update_checker.cpp | 3 | 0 | 1 | 2 | 0 |
| src/utils/boost_throw_exception.cpp | 2 | 0 | 2 | 0 | 0 |
| src/utils/normalizer.cpp | 2 | 0 | 0 | 2 | 0 |
| src/utils/timestamp_utils.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/utils/memory/pool_allocator.cpp
Total findings: 51

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 58: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~BuddyAllocator() { /* cleanup */ }
  Context: class/struct BuddyAllocator
- Line 65: severity=CRITICAL; category=missing_dtor
  Description: Class BuddyAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~BuddyAllocator() { /* cleanup */ }
  Context: class/struct BuddyAllocator
- Line 354: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SlabAllocator() { /* cleanup */ }
  Context: class/struct SlabAllocator
- Line 419: severity=CRITICAL; category=missing_dtor
  Description: Class SlabAllocator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SlabAllocator() { /* cleanup */ }
  Context: class/struct SlabAllocator
- Line 465: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: Slab* new_slab = new Slab(object_size, objects_per_slab);
- Line 465: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 230: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
- Line 230: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = impl_->allocateBlock(order);
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(size);
- Line 272: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> BuddyAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uintptr_t addr = reinterpret_cast<uintptr_t>(impl_->memory);
- Line 367: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error("Slab allocation size overflow: object_size * object_count exceeds SIZE_MA
- Line 378: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 444: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: slab = nullptr;
  Context: delete slab;
- Line 449: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* allocate() {
- Line 449: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 453: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = slab->allocate();
- Line 453: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void* ptr = slab->allocate();
- Line 470: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return new_slab->allocate();
- Line 473: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool deallocate(void* ptr) {
- Line 473: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return slab->deallocate(ptr);
- Line 492: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 492: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 505: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = impl_->allocate();
- Line 513: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(impl_->object_size);
- Line 527: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> SlabAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 552: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: slab = nullptr;
  Context: delete slab;
- Line 556: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->head_slab = nullptr;
- Line 623: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
- Line 623: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated.fetch_add(aligned_size);
- Line 661: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> StackAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
- Line 810: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 812: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto result = allocator->allocate(size, hint);
- Line 823: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
- Line 823: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 552: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete slab;

### src/utils/pki_client.cpp
Total findings: 50

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    int len = static_cast<int>(pass->size());', '    if (len > size) len = size;', '    std::memcpy(buf, pass->data(), len);', '    return len;', '}']
  Confidence: band=very_high; score=0.93
- Line 200: severity=CRITICAL; category=data_race
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
- Line 330: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!bptr || !bptr->data || bptr->length == 0) return {};
- Line 331: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return std::string(bptr->data, bptr->length);
- Line 361: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
- Line 363: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, real_size);
- Line 599: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY signing (preferred) instead of deprecated RSA_sign API.
  Confidence: band=high; score=0.8
- Line 607: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use PKCS#1 v1.5 padding for compatibility with RSA_sign
  Confidence: band=high; score=0.8
- Line 819: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY verification instead of deprecated RSA_verify
  Confidence: band=high; score=0.8
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (alg.find("SHA256") != std::string::npos) { expected_len = 32; return NID_sha256; }
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_
- Line 550: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
- Line 551: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 559: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" <<
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" <<
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
- Line 582: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "
- Line 582: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "
- Line 616: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(*pub_result);
- Line 624: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 627: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 675: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 678: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 768: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << res
- Line 768: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << res
- Line 769: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
- Line 769: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
- Line 776: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 776: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
- Line 778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code="
- Line 778: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code="
- Line 787: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
- Line 787: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
- Line 794: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body <<
- Line 794: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body <<
- Line 828: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 829: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pub);
- Line 832: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(ctx);
- Line 834: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pub);

### src/utils/geo/ewkb.cpp
Total findings: 46

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2520 [geo] Full GeoJSON RFC 7946... (2026-03-11)
- Line 240: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t temp[sizeof(double)];
- Line 255: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t temp[sizeof(uint32_t)];
- Line 499: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "GeoJSON: longitude " + std::to_string(lon) + " is out of WGS84 range [-180, 180]");
- Line 503: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "GeoJSON: latitude " + std::to_string(lat) + " is out of WGS84 range [-90, 90]");
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.push_back(c);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(trimCopy(current));
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(trimCopy(current));
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(bytes[i]);
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(bytes[i]);
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.rings[r].emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
  Confidence: band=high; score=0.74
- Line 651: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeoJSONGeomImpl(member, depth - 1));
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom.coords.push_back(parseCoordinateToken(body));
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: merged += ",";
- Line 730: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_groups.push_back(trimCopy(merged));
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 840: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 841: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
- Line 843: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y});
- Line 851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 852: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: coords_arr.push_back({c.x, c.y});
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 865: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
- Line 867: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: line_coords.push_back({c.x, c.y});
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
- Line 882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y});
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 897: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
- Line 899: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring_coords.push_back({c.x, c.y});
- Line 910: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(json::parse(toGeoJSON(sub)));
  Confidence: band=high; score=0.74
- Line 911: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: members.push_back(json::parse(toGeoJSON(sub)));
- Line 938: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/utils/error_registry.cpp
Total findings: 40

- Line 140: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 458: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "cudaMalloc or hipMalloc failed to allocate GPU memory.",
- Line 890: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=very_high; score=0.9
- Line 1322: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Failed to allocate memory for compression/decompression operation.",
- Line 1351: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Failed to allocate memory from the system.",
- Line 1691: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: errors_[code_value] = metadata;
- Line 1692: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: category_index_[metadata.category].push_back(code_value);
- Line 86: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check file/directory permissions\n"
- Line 250: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify tar/gzip availability\n"
- Line 345: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Free up system memory (RAM/VRAM)\n"
- Line 446: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure CUDA/ROCm runtime is available\n"
- Line 476: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Review NVIDIA/AMD documentation for peer access requirements\n"
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure adapter format is supported (safetensors/GGUF)\n"
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "6. Use mixed precision training (fp16/bf16)\n"
- Line 662: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. List available tools with: GET /api/v1/mcp/tools\n"
- Line 690: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check if stdin/stdout are available\n"
- Line 692: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Ensure not running in detached/daemon mode\n"
- Line 849: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Verify table/field names exist\n"
- Line 861: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check query syntax matches AQL/GraphQL specification\n"
- Line 862: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify all brackets/quotes are balanced\n"
- Line 875: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check if referenced tables/indexes exist\n"
- Line 890: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=high; score=0.74
- Line 929: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check window frame bounds (ROWS/RANGE)\n"
- Line 942: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Reduce result set size with additional FILTER/LIMIT clauses\n"
- Line 998: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Verify request format (JSON/GraphQL/etc.)\n"
- Line 1182: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Verify entropy source is available (/dev/urandom)\n"
- Line 1239: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "1. Check user/process permissions\n"
- Line 1240: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Verify file/directory ownership\n"
- Line 1241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "3. Review SELinux/AppArmor policies\n"
- Line 1476: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "2. Ensure a node/edge is not both required and forbidden\n"
- Line 1721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(errors_.at(code));
  Confidence: band=high; score=0.74
- Line 1722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(errors_.at(code));
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second);
- Line 1748: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(category);
  Confidence: band=high; score=0.74
- Line 1761: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["errors"].push_back(pair.second.toJSON());
  Confidence: band=high; score=0.74
- Line 1762: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result["errors"].push_back(pair.second.toJSON());
- Line 140: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
  Confidence: band=medium; score=0.6

### src/utils/build_info.cpp
Total findings: 31

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3830 feat(themis): Modular Build... (2026-03-12) | #3646 fix(themis): comple
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 401: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 508: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.modules.push_back({
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 847: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 858: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mod.name);
- Line 925: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/utils/audit_logger.cpp
Total findings: 24

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 163: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(cfg_.log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 189: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd2 = ::open(cfg_.secondary_log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
- Line 1552: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(cfg_.chain_head_path.c_str(), O_RDONLY);
- Line 1597: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void HashChainAuditWriter::write(nlohmann::json record) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Track per-user counts', '            std::string user = payload.value("user_id", payload.value("user", std::string{"system"}));', '            user_counts[user] = user_counts.value(user, 0) + 1;', '', '        } catch (const nlohmann::json::exception &) {']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 650: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: int sock = socket(AF_INET, SOCK_DGRAM, 0);
- Line 763: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This method is kept for API compatibility
  Confidence: band=high; score=0.8
- Line 1133: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (baseline.avg_frequency_seconds == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(b64_table[(n >> 6) & 63]);
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd2);
- Line 505: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = state["last_timestamp_ms"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 664: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(sock);
- Line 794: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 843: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 941: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1317: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 1407: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 1555: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 642: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: formatted_message = formatAsSyslog(event, event_type);
  Confidence: band=medium; score=0.6

### src/utils/http_client_pool.cpp
Total findings: 23

- Line 90: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Use striped locking to reduce contention', '    size_t stripe_idx = getStripeIndex();', '    auto& stripe = stripes_[stripe_idx];', '', '    std::unique_lock<std::mutex> lock(stripe->mutex);']
  Confidence: band=high; score=0.81
- Line 172: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<HTTPClient> HTTPClientPool::acquireConnection() {
- Line 220: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = now + config_.acquire_timeout;
- Line 247: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 268: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 274: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 290: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 298: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 330: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < stripes_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(stripe->mutex);
- Line 394: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(http::verb::post, url, body.dump(), headers);
- Line 401: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(http::verb::get, url, "", headers);
- Line 404: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: HTTPResponse BeastHTTPClient::execute(
- Line 432: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: beast::get_lowest_layer(stream).connect(results);
- Line 529: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: stream.socket().shutdown(tcp::socket::shutdown_both, ec);
- Line 27: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex url_regex(R"((https?)://([^:/]+)(?::(\d+))?(/.*)?)", std::regex::icase);
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe->connections.push_back(pooled);
  Confidence: band=high; score=0.74

### src/utils/input_validator.cpp
Total findings: 17

- Line 343: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (unsigned char c : std::string(";|&`$(){}[]<>!?*\"\\'")) {
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (!path.empty() && path.back() != '/' && path.back() != '\\') path += "/";
- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!isAsciiControl(c)) out.push_back(c);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto min_len = prop["minLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto max_len = prop["maxLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = schema["properties"].begin();
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static const char* forbidden[] = { "drop ", "truncate ", "alter ", "grant ", "revoke ", "create tabl
- Line 351: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool InputValidator::validateAQLQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  result += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  result += "&amp;";  break;
- Line 448: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  result += "&lt;";   break;
- Line 449: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  result += "&gt;";   break;
- Line 450: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  result += "&quot;"; break;
- Line 451: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': result += "&#x27;"; break;
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '/':  result += "&#x2F;"; break;

### src/utils/capability_auto_generator.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 152: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 173: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(60));
- Line 332: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(sorted_keywords.size(), (size_t)config_.max_keywords); ++i) {
- Line 371: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(capability.domains.begin(), capability.domains.end(), domain) == capability.domains.en
- Line 55: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto analysis = root["rocksdb_analysis"];
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto audit = root["audit"];
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto security = root["security"];
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto output = root["output"];
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data_types.push_back(type);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.keywords.push_back(sorted_keywords[i].first);
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.keywords.push_back(sorted_keywords[i].first);
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capability.domains.push_back(domain);
  Confidence: band=high; score=0.74
- Line 535: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::ofstream log(config_.audit_log_path, std::ios::app);
  Confidence: band=medium; score=0.6

### src/utils/grpc_channel_pool.cpp
Total findings: 15

- Line 84: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<grpc::Channel> GrpcChannelPool::acquireChannel(
- Line 42: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
- Line 73: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // No available channels, check if we can create new one
- Line 140: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats.acquire_timeouts = acquire_timeouts_.load();
- Line 142: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 143: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 158: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 160: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);
- Line 171: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(pools_mutex_);
- Line 173: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> pool_lock(pool->mutex);

### src/utils/pii_detector.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back(static_cast<char>(std::tolower(ch)));
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> PIIDetector::detectInJson(
  Confidence: band=medium; score=0.66
- Line 153: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> result;
  Confidence: band=medium; score=0.66
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: enabled.push_back(engine->getName());
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: enabled.push_back(engine->getName());
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.at("engines").push_back(engine->getMetadata());
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata.at("engines").push_back(engine->getMetadata());
- Line 275: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["global_settings"];
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_node.push_back(item_json);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>>& findings) const {
  Confidence: band=medium; score=0.66
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path_it->second.push_back(std::move(finding));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduplicated.push_back(curr);
  Confidence: band=high; score=0.74

### src/utils/regex_detection_engine.cpp
Total findings: 12

- Line 224: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["engine_type"] = "regex";
- Line 225: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["version"] = signature_.version;
- Line 226: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["enabled"] = enabled_;
- Line 67: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["settings"];
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back(finding);
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: url.regex_str = R"(https?://[^\s]+)";
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: flag_strings.push_back(flag.get<std::string>());
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern.field_hints.push_back(hint.get<std::string>());
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern.field_hints.push_back(hint.get<std::string>());

### src/utils/pii_detection_engine.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4263 PKIClient v1.8.0 + PII Stre... (2026-03-15) | #998 C++ Audit: Eliminate
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(' '); // normalize separator to space
- Line 263: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74

### src/utils/self_awareness.cpp
Total findings: 11

- Line 595: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : std::filesystem::directory_iterator(config_.snapshot_directory)) {
- Line 41: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sa = root["self_awareness"];
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto thresh = sa["thresholds"];
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto snaps = sa["snapshots"];
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MEMORYSTATUSEX mem_status;
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("CRITICAL: CPU usage at " +
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("WARNING: CPU usage at " +
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies.push_back("WARNING: High average query time: " +
- Line 593: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Collect snapshot files sorted by name (which encodes timestamp)
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 598: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path());

### src/utils/tracing.cpp
Total findings: 11

- Line 471: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 2
  Remediation: Fix loop condition or increase array size
  Context: if (value[2] != '-' || value[35] != '-' || value[52] != '-') return false;
- Line 487: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Fix loop condition or increase array size
  Context: if (!hexByte(value[0], value[1], ver) || ver != 0) return false;
- Line 252: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: tcp::socket socket(io);
- Line 253: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket.connect(*results.begin(), ec);
- Line 164: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!out.empty()) out += ',';
- Line 170: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::inject(std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::extract(const std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex re(R"((?:http|https)://([^/:]+)(?::(\d+))?)", std::regex::icase);
- Line 272: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.name", serviceName},
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.version", "0.1.0"}
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::string headerValue(const std::map<std::string, std::string>& headers,
  Confidence: band=high; score=0.74

### src/utils/lek_manager.cpp
Total findings: 10

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4263 PKIClient v1.8.0 + PII Stre... (2026-03-15) | #4216 feat(timeseries): C
- Line 193: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: rotated = nullptr;
  Context: throw std::runtime_error("Failed to delete rotated LEK from RocksDB");
- Line 339: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(rotation_cv_mu_);
- Line 366: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& expired_date : to_revoke) {
  Confidence: band=very_high; score=0.9
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74

### src/utils/logger.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4330 feat(cache): network-backed... (2026-03-19) | #4268 ProvenanceTracker:
- Line 51: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 45: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 46: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 51: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
- Line 95: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 123: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 153: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6

### src/utils/thread_pool_manager.cpp
Total findings: 9

- Line 260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.io_stats = io_pool_->getStatistics();
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.cpu_stats = cpu_pool_->getStatistics();
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.blocking_stats = blocking_pool_->getStatistics();
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.blocking_stats = blocking_pool_->getStatistics();
- Line 41: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::shared_mutex> lock(mutex_);
- Line 44: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: bool available = cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
- Line 61: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: task->execute();
- Line 126: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 27: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this]() { workerLoop(); });
  Confidence: band=high; score=0.74

### src/utils/cursor.cpp
Total findings: 7

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(base64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(base64_chars[(val >> valb) & 0x3F]);
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
- Line 47: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back('=');
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(char((val >> valb) & 0xFF));

### src/utils/hkdf_cache.cpp
Total findings: 7

- Line 158: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 165: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(s.mu);
- Line 211: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& shard : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(shard.mu);
- Line 217: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: size_t ikm_end = raw_key.find('\x00');
  Confidence: band=very_high; score=0.9

### src/utils/hkdf_helper.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_CTX_free(kctx);
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_CTX_free(kctx);
- Line 85: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 112: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 116: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);

### src/utils/ner_detection_engine.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2543 [utils] Upgrade PII detecti... (2026-03-11)
- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, i, name_end - 1, PIIType::PERSON_NAME,
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, i, i, PIIType::ORGANIZATION,
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(makeSpan(tokens, loc_start, loc_end - 1,
- Line 511: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > first) value += ' ';
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > first) value += ' ';

### src/utils/zstd_codec.cpp
Total findings: 7

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 62: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Failed to allocate memory for compression: {}", e.what());
- Line 65: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for compression", max_size)
- Line 163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_ERROR("Failed to allocate memory for decompression: {}", e.what());
- Line 166: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for decompression", decompressed_size)

### src/utils/pii_pseudonymizer.cpp
Total findings: 6

- Line 52: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 6
  Remediation: Fix loop condition or increase array size
  Context: bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
- Line 53: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 8
  Remediation: Fix loop condition or increase array size
  Context: bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 10
- Line 79: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto findings_map = detector_->detectInJson(data);
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto encrypted_json = mapping["original_value_encrypted"];
  Confidence: band=high; score=0.74

### src/utils/serialization.cpp
Total findings: 6

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    std::vector<float> vec(count);', '    const uint8_t* data = &data_[pos_];', '    std::memcpy(vec.data(), data, count * sizeof(float));', '    pos_ += count * sizeof(float);', '']
  Confidence: band=very_high; score=0.93
- Line 20: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: buffer_.reserve(1024); // Pre-allocate
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: value |= static_cast<uint64_t>(data_[pos_++]) << (i * 8);
- Line 209: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string str(reinterpret_cast<const char*>(&data_[pos_]), size);
- Line 227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t* data = &data_[pos_];
- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_.push_back((value >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74

### src/utils/utils_adapters.cpp
Total findings: 6

- Line 84: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(result.categories.begin(), result.categories.end(), cat)
- Line 171: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
- Line 171: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (!stage->execute()) {
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.categories.push_back(cat);
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=high; score=0.74

### src/utils/checksum_utils.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 66: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The deprecated signature is kept for backward-compatible callers that still
  Confidence: band=high; score=0.8
- Line 41: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 49: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 52: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/utils/cron_parser.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2558 [scheduler] Full cron expre... (2026-03-12) | #1178 Verify and document
- Line 282: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (minutes_.find(tm.tm_min) == minutes_.end())      return false;
- Line 283: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (hours_.find(tm.tm_hour) == hours_.end())         return false;
- Line 341: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (minutes_.find(tm.tm_min) == minutes_.end()) {
- Line 345: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: tm
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (hours_.find(tm.tm_hour) == hours_.end()) {

### src/utils/sampled_logger.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 58: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: bool SampledLogger::should_log(Logger::Level level, const char* file, int line) {
  Confidence: band=medium; score=0.6
- Line 99: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void SampledLogger::log(Logger::Level level, const std::string& msg,
  Confidence: band=medium; score=0.6
- Line 102: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!should_log(level, file, line)) {
  Confidence: band=medium; score=0.6

### src/utils/simd_distance.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 372: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
- Line 376: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {

### src/utils/bloom_filter.cpp
Total findings: 4

- Line 101: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Context: void BloomFilter::clear() {
  Confidence: band=medium; score=0.56
- Line 28: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double ln2 = std::log(2.0);
  Confidence: band=medium; score=0.6
- Line 29: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (ln2 * ln2)));
  Confidence: band=medium; score=0.6
- Line 35: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: size_t k = static_cast<size_t>(std::ceil(static_cast<double>(bits) / static_cast<double>(n) * std::log(2.0)));
  Confidence: band=medium; score=0.6

### src/utils/compression_metrics.cpp
Total findings: 4

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: methods.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (stats.bytes_in / 1024.0 / 1024.0) << " MB)\n";
- Line 89: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (stats.bytes_out / 1024.0 / 1024.0) << " MB)\n";
- Line 97: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << stats.compression_throughput_mbps() << " MB/s\n";

### src/utils/consistent_hash.cpp
Total findings: 4

- Line 75: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Context: size_t ConsistentHashRing::nodeCount() const {
  Confidence: band=medium; score=0.56

### src/utils/rate_limiter.cpp
Total findings: 4

- Line 53: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void RateLimiter::acquire(double tokens) {
- Line 42: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool RateLimiter::try_acquire(double tokens) {
- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void RateLimiter::acquire(double tokens) {
- Line 73: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/utils/stopwords.cpp
Total findings: 4

- Line 16: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static std::unordered_set<std::string> make_set(std::initializer_list<const char*> list) {
  Confidence: band=medium; score=0.66
- Line 17: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> s;
  Confidence: band=medium; score=0.66
- Line 45: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> Stopwords::merge(const std::unordered_set<std::string>& base,
  Confidence: band=medium; score=0.66
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> out = base;
  Confidence: band=medium; score=0.66

### src/utils/lz4_codec.cpp
Total findings: 3

- Line 57: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for LZ4 output", bound));
- Line 106: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: fmt::format("Cannot allocate {} bytes for LZ4 decompressed output", original_size));
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    }', '', '    const int src_size = static_cast<int>(size);', '    const int bound    = LZ4_compressBound(src_size);', '    if (bound <= 0) {']
  Confidence: band=medium; score=0.65

### src/utils/pii_stream_scanner.cpp
Total findings: 3

- Line 41: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cfg_.lookahead_bytes = engine_->maxPatternLength();
- Line 146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string hmac_key = cfg_.tenant_id + ":" + lek_mgr_->getCurrentLEK();
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(f));
  Confidence: band=high; score=0.74

### src/utils/retention_manager.cpp
Total findings: 3

- Line 391: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);
- Line 391: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(policy);
  Confidence: band=high; score=0.74

### src/utils/runtime_license_gate.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4518 [WIP] Update developer docu... (2026-04-12) | #3408 Migrate Themis core
- Line 171: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: msg << "license has been " << status << ".";
- Line 177: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: msg << "license status is '" << status << "'.";

### src/utils/saga_logger.cpp
Total findings: 3

- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_array.push_back(entry);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step);
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_ids.push_back(j["batch_id"].get<std::string>());

### src/utils/update_checker.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: releases.push_back(*release);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: releases.push_back(*release);

### src/utils/boost_throw_exception.cpp
Total findings: 2

- Line 19: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw e;
- Line 23: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw e;

### src/utils/normalizer.cpp
Total findings: 2

- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is2(c, d, 0xC3, 0xA4)) { out.push_back('a'); i += 2; continue; } // ä
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(c));

### src/utils/timestamp_utils.cpp
Total findings: 1

- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['', '    // Seconds with millisecond fraction', '    oss << s.count();', '    if (ms_part.count() > 0) {', '        char ms_buf[8];']
  Confidence: band=medium; score=0.65

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
