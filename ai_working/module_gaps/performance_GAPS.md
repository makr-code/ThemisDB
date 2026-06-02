# performance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: performance
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 428
- Actionable Findings (Critical + High): 254
- Affected Files: 29

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 128 |
| High | 126 |
| Medium | 173 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| concurrency | 90 |
| performance_patterns | 77 |
| reliability | 60 |
| raii | 48 |
| container | 46 |
| exception_safety | 23 |
| determinism | 22 |
| memory | 15 |
| observability | 10 |
| performance | 9 |
| input_validation | 8 |
| security | 7 |
| platform | 6 |
| uninitialized | 3 |
| audit_logging | 2 |
| distributed_consistency | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/performance/phase4/io_uring_zero_copy.cpp | 71 | 59 | 7 | 5 | 0 |
| src/performance/adaptive_query_compiler.cpp | 57 | 3 | 19 | 35 | 0 |
| src/performance/phase3/bwtree.cpp | 30 | 11 | 13 | 6 | 0 |
| src/performance/phase3/diskann.cpp | 30 | 6 | 12 | 12 | 0 |
| src/performance/phase4/pmu_counters.cpp | 28 | 11 | 3 | 14 | 0 |
| src/performance/numa_topology.cpp | 22 | 1 | 2 | 19 | 0 |
| src/performance/wisckey.cpp | 21 | 9 | 6 | 5 | 1 |
| src/performance/ligra.cpp | 20 | 3 | 7 | 10 | 0 |
| src/performance/hardware_accelerator.cpp | 19 | 0 | 7 | 12 | 0 |
| src/performance/phase4/pmem_storage.cpp | 18 | 7 | 7 | 4 | 0 |
| src/performance/numa_memory_manager.cpp | 16 | 1 | 11 | 4 | 0 |
| src/performance/prometheus_exporter.cpp | 11 | 9 | 0 | 2 | 0 |
| src/performance/async_metrics_exporter.cpp | 10 | 1 | 9 | 0 | 0 |
| src/performance/chimera_exporter.cpp | 9 | 4 | 0 | 5 | 0 |
| src/performance/rabitq.cpp | 9 | 0 | 5 | 4 | 0 |
| src/performance/cicada.cpp | 8 | 0 | 6 | 2 | 0 |
| src/performance/intelligent_prefetcher.cpp | 7 | 0 | 3 | 4 | 0 |
| src/performance/workload_adaptive_optimizer.cpp | 6 | 0 | 2 | 4 | 0 |
| src/performance/advanced_cache_manager.cpp | 5 | 0 | 2 | 3 | 0 |
| src/performance/phase3/gunrock.cpp | 5 | 0 | 2 | 3 | 0 |
| src/performance/phase3/per_query_cost_model.cpp | 5 | 0 | 0 | 5 | 0 |
| src/performance/phase3/splinterdb.cpp | 5 | 2 | 0 | 3 | 0 |
| src/performance/phase3/adaptive_batch_tuner.cpp | 4 | 0 | 3 | 1 | 0 |
| src/performance/phase3/memory_pressure.cpp | 3 | 0 | 0 | 3 | 0 |
| src/performance/cycle_metrics.cpp | 2 | 0 | 0 | 2 | 0 |
| src/performance/phase2_feature_flags.cpp | 2 | 0 | 0 | 2 | 0 |
| src/performance/phase3/bao.cpp | 2 | 1 | 0 | 1 | 0 |
| src/performance/phase3/feature_flags.cpp | 2 | 0 | 0 | 2 | 0 |
| src/performance/phase4/feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/performance/phase4/io_uring_zero_copy.cpp
Total findings: 71

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->ring_fd = fd;
- Line 218: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->ring_fd = -1;
- Line 221: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ptr       = sq_ptr;
- Line 222: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_mmap_size = sq_ring_sz;
- Line 222: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_mmap_size = sq_ring_sz;
- Line 225: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_head         = reinterpret_cast<unsigned*>(sq_base + params.sq_off.head);
- Line 226: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_tail         = reinterpret_cast<unsigned*>(sq_base + params.sq_off.tail);
- Line 227: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ring_mask    = reinterpret_cast<unsigned*>(sq_base + params.sq_off.ring_mask);
- Line 228: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ring_entries = reinterpret_cast<unsigned*>(sq_base + params.sq_off.ring_entries);
- Line 229: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_flags        = reinterpret_cast<unsigned*>(sq_base + params.sq_off.flags);
- Line 230: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_dropped      = reinterpret_cast<unsigned*>(sq_base + params.sq_off.dropped);
- Line 231: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_array        = reinterpret_cast<unsigned*>(sq_base + params.sq_off.array);
- Line 242: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->ring_fd  = -1;
- Line 243: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ptr   = nullptr;
- Line 246: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqe_ptr      = sqe_ptr;
- Line 247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqe_mmap_size = sqe_sz;
- Line 247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqe_mmap_size = sqe_sz;
- Line 248: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqes = static_cast<struct io_uring_sqe*>(sqe_ptr);
- Line 248: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqes = static_cast<struct io_uring_sqe*>(sqe_ptr);
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->ring_fd  = -1;
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ptr   = nullptr;
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqe_ptr  = nullptr;
- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_ptr       = cq_ptr;
- Line 267: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_mmap_size = cq_ring_sz;
- Line 267: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_mmap_size = cq_ring_sz;
- Line 270: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_head         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.head);
- Line 271: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_tail         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.tail);
- Line 272: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_ring_mask    = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_mask);
- Line 273: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_ring_entries = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_entries);
- Line 274: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cqes = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);
- Line 293: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ring_->ring_fd >= 0) ::close(ring_->ring_fd);
- Line 294: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->ring_fd  = -1;
- Line 295: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_ptr   = nullptr;
- Line 296: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sqe_ptr  = nullptr;
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->cq_ptr   = nullptr;
- Line 313: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = io_uring_register(ring_->ring_fd,
- Line 335: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = io_uring_register(ring_->ring_fd,
- Line 354: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (available_ && ring_->ring_fd >= 0) {
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned tail   = *ring_->sq_tail;
- Line 356: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned mask   = *ring_->sq_ring_mask;
- Line 359: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring_->sqes[index];
- Line 364: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len         = static_cast<uint32_t>(len);
- Line 365: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->buf_index   = static_cast<uint16_t>(buf_index);
- Line 370: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data   = static_cast<uint64_t>(buf_index);
- Line 372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_array[index] = index;
- Line 406: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (available_ && ring_->ring_fd >= 0) {
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned tail   = *ring_->sq_tail;
- Line 408: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned mask   = *ring_->sq_ring_mask;
- Line 411: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring_->sqes[index];
- Line 416: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len       = static_cast<uint32_t>(max_len);
- Line 417: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->buf_index = static_cast<uint16_t>(buf_index);
- Line 421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data = static_cast<uint64_t>(buf_index);
- Line 423: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_->sq_array[index] = index;
- Line 449: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = io_uring_enter(ring_->ring_fd, 0, min_completions,
- Line 454: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned head = *ring_->cq_head;
- Line 455: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: unsigned mask = *ring_->cq_ring_mask;
- Line 459: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: while (head != *ring_->cq_tail) {
- Line 460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_cqe* cqe = &ring_->cqes[head & mask];
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        sqe->opcode      = IORING_OP_SEND;', '        sqe->fd          = fd;', '        sqe->addr        = reinterpret_cast<uint64_t>(buffers_[buf_index].data());', '        sqe->len         = static_cast<uint32_t>(len);', '        sqe->buf_index   = static_cast<uint16_t>(buf_index);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Fallback: standard blocking send()', '    ssize_t ret = ::send(fd, buffers_[buf_index].data(), len,', '#ifdef MSG_NOSIGNAL', '                         MSG_NOSIGNAL']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        sqe->opcode    = IORING_OP_RECV;', '        sqe->fd        = fd;', '        sqe->addr      = reinterpret_cast<uint64_t>(buffers_[buf_index].data());', '        sqe->len       = static_cast<uint32_t>(max_len);', '        sqe->buf_index = static_cast<uint16_t>(buf_index);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '    // Fallback: standard blocking recv()', '    ssize_t ret = ::recv(fd, buffers_[buf_index].data(), max_len, 0);', '    if (ret < 0) return -errno;', '    fallback_recvs_.fetch_add(1, std::memory_order_relaxed);']
  Confidence: band=high; score=0.78
- Line 231: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring_->sq_array        = reinterpret_cast<unsigned*>(sq_base + params.sq_off.array);
- Line 388: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t ret = ::send(fd, buffers_[buf_index].data(), len,
- Line 469: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: cq_completed_.fetch_add(count, std::memory_order_relaxed);
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffers_.emplace_back(config_.buffer_size);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 241: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 260: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 523: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);

### src/performance/adaptive_query_compiler.cpp
Total findings: 57

- Line 374: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 399: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 738: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rit = hash_table.find(lkstr);
- Line 92: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::EQ:   return lhs == rhs;
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case Predicate::Op::NEQ:  return lhs != rhs;
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return lhs == pattern;
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: asm_str << "  prefetcht0 [rdi]\n";
- Line 361: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: QueryResult execute(const ParsedQuery& query,
- Line 418: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (entry.baseline_row_count == 0.0)
  Confidence: band=very_high; score=0.9
- Line 499: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: it->second.compiled->execute != nullptr;
- Line 656: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (groups.find(gkey) == groups.end())
- Line 697: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fn == "MIN")   return acc.min_v != std::numeric_limits<double>::max()
  Confidence: band=very_high; score=0.9
- Line 699: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fn == "MAX")   return acc.max_v != std::numeric_limits<double>::lowest()
  Confidence: band=very_high; score=0.9
- Line 987: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (groups.find(gk) == groups.end()) order.push_back(gk);
- Line 1112: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: QueryResult AdaptiveQueryCompiler::execute(const ParsedQuery& query,
- Line 1115: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(query, schema, params);
- Line 1118: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: QueryResult AdaptiveQueryCompiler::execute(const CompiledQuery& compiled,
- Line 1126: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return compiled.execute(params);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.column_names.push_back(col.name);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.column_names.push_back(col.name);
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.values.push_back(QueryValue{static_cast<int64_t>(row_idx)});
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.values.push_back(QueryValue{static_cast<double>(row_idx) * 1.5});
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.values.push_back(QueryValue{(row_idx % 2) == 0});
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.values.push_back(
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row.values.push_back(QueryValue{std::monostate{}});
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proj_row.column_names.push_back(col_name);
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: proj_row.column_names.push_back(col_name);
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: proj_row.values.push_back(v ? *v : QueryValue{std::monostate{}});
- Line 628: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_row.column_names.push_back(query.agg_function + "_result");
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_row.column_names.push_back(query.agg_function + "_result");
- Line 639: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, AggAccum> groups;
  Confidence: band=medium; score=0.66
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_order.push_back(gkey);
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_row.column_names.push_back(query.group_by_column);
  Confidence: band=high; score=0.74
- Line 680: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_row.column_names.push_back(query.group_by_column);
- Line 681: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_row.column_names.push_back(query.agg_function + "_result");
- Line 682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_row.values.push_back(QueryValue{gkey});
- Line 683: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_row.values.push_back(QueryValue{
- Line 745: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined.column_names.push_back(
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: joined.column_names.push_back(
  Confidence: band=high; score=0.74
- Line 746: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: joined.column_names.push_back(
- Line 748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: joined.values.push_back(rit->second.values[ci]);
- Line 865: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preds.push_back({p.column, p.op, p.value, p.param_name, ct});
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) base.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 949: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (pass) base.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 969: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.column_names.push_back(agg_fn + "_result");
  Confidence: band=high; score=0.74
- Line 970: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.column_names.push_back(agg_fn + "_result");
- Line 971: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.values.push_back(QueryValue{applyAggFunction(
- Line 979: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, AggAccum> groups;
  Confidence: band=medium; score=0.66
- Line 987: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (groups.find(gk) == groups.end()) order.push_back(gk);
  Confidence: band=high; score=0.74
- Line 1009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(out));
  Confidence: band=high; score=0.74

### src/performance/phase3/bwtree.cpp
Total findings: 30

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 47: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: auto root = new LeafPage();
- Line 47: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto root = new LeafPage();
- Line 90: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: auto delta = new DeltaInsert(key, value);
- Line 90: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto delta = new DeltaInsert(key, value);
- Line 121: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: auto delta = new DeltaDelete(key);
- Line 121: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto delta = new DeltaDelete(key);
- Line 136: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BwTreePage* page = mapping_table_->get(root_pid_);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BwTreePage* page = mapping_table_->get(pid);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 36: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::memory_order_acquire
- Line 52: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: root = nullptr;
  Context: delete root;
- Line 90: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: auto delta = new DeltaInsert(key, value);
- Line 99: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: delta = nullptr;
  Context: delete delta;
- Line 131: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: delta = nullptr;
  Context: delete delta;
- Line 255: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: current = current->next_delta.load(std::memory_order_acquire);
- Line 332: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: current = current->next_delta.load(std::memory_order_acquire);
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: consolidation_epoch_.load(std::memory_order_acquire)});
- Line 354: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: consolidation_epoch_.load(std::memory_order_acquire);
- Line 372: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: BwTreePage* next = current->next_delta.load(std::memory_order_acquire);
- Line 373: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: current = nullptr;
  Context: delete current;
- Line 99: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete delta;
- Line 131: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete delta;
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({k, v});
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({k, v});
- Line 222: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: consolidated.release();
- Line 373: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete current;

### src/performance/phase3/diskann.cpp
Total findings: 30

- Line 308: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->read(reinterpret_cast<char*>(node.vector.data()),
- Line 313: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->read(reinterpret_cast<char*>(&neighbor_count), sizeof(uint32_t));
- Line 317: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->read(reinterpret_cast<char*>(node.neighbors.data()),
- Line 333: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->write(reinterpret_cast<const char*>(node.vector.data()),
- Line 338: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->write(reinterpret_cast<const char*>(&neighbor_count), sizeof(uint32_t));
- Line 341: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: graph_file_->write(reinterpret_cast<const char*>(node.neighbors.data()),
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 28: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: size_t node_size_estimate = sizeof(VectorID) + dimension * sizeof(float) + 64 * sizeof(VectorID);
- Line 108: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& node : nodes) {
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(file_mutex_);
- Line 333: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graph_file_->write(reinterpret_cast<const char*>(node.vector.data()),
- Line 465: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->id = vectors[start].first;
- Line 466: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->vector = vectors[start].second;
- Line 475: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));
- Line 481: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->threshold = sorted_dists[sorted_dists.size() / 2];
- Line 486: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (compute_distance(node->vector, vectors[i].second) < node->threshold) {
- Line 493: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->left = build_tree(vectors, start + 1, mid);
- Line 494: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->right = build_tree(vectors, mid, end);
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.neighbors.push_back(nearest.top().second);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.neighbors.push_back(nearest.top().second);
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({id, dist});
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({id, dist});
- Line 381: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: best_candidates.push_back({dist, current_id});
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: distances.push_back(compute_distance(node->vector, vectors[i].second));

### src/performance/phase4/pmu_counters.cpp
Total findings: 28

- Line 62: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PmuCounter::open(uint32_t type, uint64_t config) noexcept {
- Line 98: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 101: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (::read(fd_, &value, sizeof(value)) != sizeof(value)) {
- Line 346: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
- Line 364: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 500: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
- Line 516: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 626: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
- Line 640: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: uint64_t PmuCounter::read() const noexcept {
- Line 746: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool PmuCounter::open(uint32_t type, uint64_t config) noexcept {
- Line 767: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: uint64_t PmuCounter::read()  const noexcept {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['// Per-thread kpc counter snapshot buffers', 'constexpr uint32_t kKpcBufSize = 32;', 'static thread_local uint64_t tl_kpc_baseline[kKpcBufSize] = {};', '', '} // anonymous namespace']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    auto& api = KpcApi::instance();', '    if (api.loaded) {', '        uint64_t current[kKpcBufSize] = {};', '        api.get_thread_counters(0, kKpcBufSize, current);', '        m.l1d_read_misses       = current[0] - tl_kpc_baseline[0];']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    auto& api = KpcApi::instance();', '    if (api.init()) {', '        uint64_t probe[kKpcBufSize] = {};', '        return api.get_thread_counters(0, kKpcBufSize, probe) == 0;', '    }']
  Confidence: band=high; score=0.81
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 63: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 107: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PmuCounter::close() noexcept {
- Line 109: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 347: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 371: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 501: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 523: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 627: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 647: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PmuCounter::close() noexcept { fd_ = -1; }
- Line 757: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 776: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 797: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 812: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/performance/numa_topology.cpp
Total findings: 22

- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 303: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cs) == 0;
- Line 347: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pthread_ without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cs) != 0) return {};
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: try { cpus.push_back(std::stoi(token)); } catch (...) {}
- Line 82: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cpus.push_back(std::stoi(token)); } catch (...) {}
- Line 88: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { kb = std::stoull(tok); } catch (...) {}
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (DWORD i = 0; i < si.dwNumberOfProcessors; ++i) node0.cpu_ids.push_back(static_cast<int>(i));
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.cpu_ids.push_back(cpu_base + bit);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.cpu_ids.push_back(cpu_base + bit);
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.cpu_ids.push_back(cpu_base + bit);
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (unsigned int i = 0; i < nproc; ++i) node0.cpu_ids.push_back(static_cast<int>(i));
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.nodes.push_back(node0);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (CPU_ISSET(static_cast<unsigned int>(i), &cs)) cpus.push_back(i);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (CPU_ISSET(static_cast<unsigned int>(i), &cs)) cpus.push_back(i);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cpus.push_back(base + bit);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cpus.push_back(base + bit);
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (unsigned int i = 0; i < nproc; ++i) cpus.push_back(static_cast<int>(i));

### src/performance/wisckey.cpp
Total findings: 21

- Line 33: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: current_offset_ = log_file_->tellg();
- Line 33: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: current_offset_ = log_file_->tellg();
- Line 64: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: log_file_->write(value.data(), value.size());
- Line 71: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: std::optional<std::string> ValueLog::read(const ValueAddress& addr) {
- Line 78: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: log_file_->read(&value[0], addr.size);
- Line 114: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: log_file_->read(&value[0], addr.size);
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ValueAddress addr = value_log_->append(value);
- Line 183: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return value_log_->read(addr);
- Line 194: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.value_log_size = value_log_->size();
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
  Context: * PR History (last 5): #967 [WIP] Implement garbage col... (2026-03-11) | #1122 Eliminate lock overl
- Line 122: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Write value to new log
- Line 23: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: test.close();
- Line 39: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: log_file_->close();
- Line 117: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_log.close();
- Line 137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_log.close();
- Line 140: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: log_file_->close();
- Line 101: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::fstream temp_log(temp_log_path, std::ios::out | std::ios::binary);
  Confidence: band=medium; score=0.6

### src/performance/ligra.cpp
Total findings: 20

- Line 63: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 86: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 159: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Simple parallel processing (would use thread pool in production)', '    std::vector<std::thread> threads;', '    size_t chunk_size = (active.size() + num_threads_ - 1) / num_threads_;', '', '    auto it = active.begin();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        const auto& active = frontier.get_sparse();', '        std::vector<std::thread> threads;', '        size_t chunk_size = (active.size() + num_threads_ - 1) / num_threads_;', '', '        auto it = active.begin();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 104: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: atomic_dense[i].store(false, std::memory_order_relaxed);
- Line 112: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: atomic_dense[dst].store(true, std::memory_order_relaxed);
- Line 121: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: if (atomic_dense[i].load(std::memory_order_relaxed)) {
- Line 212: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Reset new ranks
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([it, chunk_end, &func]() {
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([it, chunk_end, &func]() {
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([start, end, &frontier, &func]() {
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([start, end, &frontier, &func]() {
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([t, it, chunk_end, &adj_list, &func, &thread_buffers]() {
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([t, it, chunk_end, &adj_list, &func, &thread_buffers]() {
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thread_buffers[t].push_back(dst);  // Collision-free thread index
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: thread_buffers[t].push_back(dst);  // Collision-free thread index
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks_.push_back(task);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tasks_.push_back(task);
  Confidence: band=high; score=0.74

### src/performance/hardware_accelerator.cpp
Total findings: 19

- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = ht.find(key);
- Line 156: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = ht.find(key);
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [col](const Row& a, const Row& b) {
- Line 617: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator&    op,
- Line 700: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
- Line 700: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(op, config_.default_device_config);
- Line 97: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<size_t>>
  Confidence: band=medium; score=0.66
- Line 99: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<size_t>> ht;
  Confidence: band=medium; score=0.66
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[rows[i][key_col]].push_back(i);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(joined));
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (match) r.match_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 700: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
  Confidence: band=high; score=0.74

### src/performance/phase4/pmem_storage.cpp
Total findings: 18

- Line 50: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* mounts = fopen("/proc/mounts", "r");
- Line 57: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while (fscanf(mounts, "%254s %254s %62s %510s %d %d",
- Line 195: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: fd_ = ::open(config.path.c_str(), flags, 0644);
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: header_->pool_size     = pool_size;
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: header_->bitmap_offset = header_end;
- Line 336: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.total_bytes  = header_->pool_size;
- Line 360: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void* PMemStorageLayout::write(const std::string& /*key*/,
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_->data_offset   = data_offset;
- Line 266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: header_->bump_offset   = data_offset; // start of free arena
- Line 286: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* PMemPool::allocate(size_t size) noexcept {
- Line 286: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* PMemPool::allocate(size_t size) noexcept {
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* dst = pool_.allocate(padded);
- Line 73: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(mounts);
- Line 133: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (fd_ >= 0) ::close(fd_);
- Line 202: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 315: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PMemPool::free(void* /*ptr*/, size_t /*size*/) noexcept {

### src/performance/numa_memory_manager.cpp
Total findings: 16

- Line 55: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* f = fopen(path, "r");
- Line 109: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Check: if (ptr != nullptr) before dereferencing
  Context: ptr = std::malloc(size);
- Line 148: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* NUMAMemoryManager::allocate_on_node(size_t size, int node) {
- Line 150: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = do_allocate(size, resolved, false);
- Line 156: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* NUMAMemoryManager::allocate_local(size_t size) {
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return allocate_on_node(size, get_current_node());
- Line 160: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
- Line 160: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* NUMAMemoryManager::allocate(size_t size, const AllocationHint& hint) {
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void* ptr = do_allocate(size, resolved, hint.use_huge_pages);
- Line 168: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
- Line 168: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void NUMAMemoryManager::deallocate(void* ptr, size_t size) noexcept {
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: s.local_accesses  = stat_local_.load(std::memory_order_relaxed);
- Line 42: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
- Line 54: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(path, sizeof(path), "/sys/devices/system/node/node%zu/meminfo", i);
- Line 65: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(f);
- Line 191: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(ptr);

### src/performance/prometheus_exporter.cpp
Total findings: 11

- Line 78: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_hnsw += m->hnsw_search_cycles;
- Line 79: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_pointer += m->pointer_passing_cycles;
- Line 80: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_llm += m->llm_inference_cycles;
- Line 81: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_cache += m->cache_miss_cycles;
- Line 81: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_cache += m->cache_miss_cycles;
- Line 82: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_pcie_h2d += m->pcie_host_to_device_cycles;
- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_pcie_d2h += m->pcie_device_to_host_cycles;
- Line 84: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_total += m->total_cycles;
- Line 85: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_cpu_eff += m->cpu_efficiency_ratio;
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);

### src/performance/async_metrics_exporter.cpp
Total findings: 10

- Line 228: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: aggregated_metrics_.insert(
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 112: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: export_thread_ = std::thread([this]() {
  Confidence: band=very_high; score=0.9
- Line 178: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 180: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 181: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(export_interval_), [this]() {
- Line 182: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return !running_.load(std::memory_order_acquire);

### src/performance/chimera_exporter.cpp
Total findings: 9

- Line 81: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_hnsw += m->hnsw_search_cycles;
- Line 82: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_pointer += m->pointer_passing_cycles;
- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_llm += m->llm_inference_cycles;
- Line 84: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_total += m->total_cycles;
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aggregated[entry.operation_name].push_back(&entry.metrics);
- Line 116: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << ((double)avg_hnsw / avg_total * 100.0) << ",\n";
- Line 118: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << ((double)avg_pointer / avg_total * 100.0) << ",\n";
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << ((double)avg_llm / avg_total * 100.0) << "\n";

### src/performance/rabitq.cpp
Total findings: 9

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #1005 [REFACTOR] Quantizer analys... (2026-03-11) | #1072 Add Vector Indexing
- Line 315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_centroids[cluster][dim] += subvec_data[i][dim];
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvec_data[weighted(rng)]);
  Confidence: band=high; score=0.74

### src/performance/cicada.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 24: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Write the new data value into the record while the write lock is held
- Line 55: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry.record->set_data(entry.data);
- Line 68: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool CicadaTransaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: void CicadaTransaction::abort() {
  Confidence: band=very_high; score=0.9
- Line 17: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: read_set_.push_back({record, version_read});
- Line 24: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool CicadaTransaction::execute(const TransactionFunc& func) {
  Confidence: band=high; score=0.74

### src/performance/intelligent_prefetcher.cpp
Total findings: 7

- Line 23: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   each candidate stride, and normalising to a [0,1] confidence score.
- Line 115: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stat_useful_prefetches_.fetch_add(1, std::memory_order_relaxed);
- Line 184: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stat_total_prefetches_.fetch_add(1, std::memory_order_relaxed);
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(static_cast<uint64_t>(addr));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, size_t> stride_counts;
  Confidence: band=medium; score=0.66
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_.addresses.push_back(history_[i].address);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern_.addresses.push_back(history_[i].address);

### src/performance/workload_adaptive_optimizer.cpp
Total findings: 6

- Line 28: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void WorkloadAdaptiveOptimizer::record_query(bool is_write, double complexity,
- Line 187: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 56: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> table_counts;
  Confidence: band=medium; score=0.66
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.hot_tables.push_back(tvec[i].first);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profile.hot_tables.push_back(tvec[i].first);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: profile.hot_tables.push_back(tvec[i].first);

### src/performance/advanced_cache_manager.cpp
Total findings: 5

- Line 462: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& p : partitions_) {
  Confidence: band=very_high; score=0.9
- Line 463: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(p->mtx);
- Line 213: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 294: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions_.push_back(std::move(ps));
  Confidence: band=high; score=0.74

### src/performance/phase3/gunrock.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->column_indices.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->column_indices.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->row_offsets.push_back(impl_->column_indices.size());

### src/performance/phase3/per_query_cost_model.cpp
Total findings: 5

- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(records_[pos]);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> factors;
  Confidence: band=medium; score=0.66
- Line 261: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> type_time_sum;
  Confidence: band=medium; score=0.66
- Line 262: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> type_count;
  Confidence: band=medium; score=0.66
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times_ms.push_back(r.execution_time_ms);
  Confidence: band=high; score=0.74

### src/performance/phase3/splinterdb.cpp
Total findings: 5

- Line 39: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cv_.wait(lock, [this]() { return !tasks_.empty() || shutdown_; });
- Line 111: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/performance/phase3/adaptive_batch_tuner.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    size_t p99_idx = static_cast<size_t>(', '        0.99 * static_cast<double>(latencies.size() - 1));', '    s.p99_latency_ms = latencies[p99_idx];', '', '    return s;']
  Confidence: band=high; score=0.81
- Line 77: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (ema_throughput_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: } else if (throughput_improving || prev_ema_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(r.latency_ms);
  Confidence: band=high; score=0.74

### src/performance/phase3/memory_pressure.cpp
Total findings: 3

- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_call.push_back(entry.callback);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_call.push_back(entry.callback);
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_call.push_back(entry.callback);

### src/performance/cycle_metrics.cpp
Total findings: 2

- Line 181: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 197: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/performance/phase2_feature_flags.cpp
Total findings: 2

- Line 28: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto phase2 = config["performance"]["phase2"];
  Confidence: band=high; score=0.74
- Line 46: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/performance/phase3/bao.cpp
Total findings: 2

- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.avg_speedup = impl_->queries_optimized > 0
- Line 25: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Context: std::mt19937 rng;
  Confidence: band=high; score=0.74

### src/performance/phase3/feature_flags.cpp
Total findings: 2

- Line 29: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto phase3 = config["performance"]["phase3"];
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/performance/phase4/feature_flags.cpp
Total findings: 1

- Line 41: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
