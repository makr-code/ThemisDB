# Thread-Safety Fixes for LLM Module

## Analysis Summary

### Issues Identified

#### 1. AsyncInferenceEngine

**Data Races:**
- `engine_start_time_` (line 649): Read without synchronization
  - Called from getQueueStats() which is const and public
  - Should be atomic<steady_clock::time_point> or protected by mutex

- `active_plugin_` assignments in constructor (lines 37, 69, 105, 133)
  - While constructor executes before threads start, assignment should explicitly hold plugin_mutex_ for clarity and safety

**Circular Lock Ordering Risks:**
- `queue_mutex_` + `tracking_mutex_` in submit methods
- `plugin_mutex_` + `cache_meta_mutex_` in processRequest
- `tracking_mutex_` + `queue_mutex_` in timeout monitor

**Lock Contention:**
- `latency_mutex_` protects large vector operations
- `tracking_mutex_` used frequently for active_requests_

#### 2. ActiveVRAMAllocator

**Data Races:**
- `stats_` fields updated under lock but some counters like `next_id_` use atomic
- Inconsistent memory ordering on atomic operations

**Lock Ordering:**
- `mu_` protecting allocations_ and stats_ consistently applied

#### 3. LLMPluginManager

**Data Races:**
- `vram_allocator_` member variable accessed from multiple methods
- `vram_handles_` map access under vram_allocator_ but needs explicit mutex
- `adapter_publisher_` raw pointer without synchronization

## Fix Strategy

1. Make engine_start_time_ atomic using a wrapper
2. Standardize atomic memory orderings
3. Add explicit lock acquisition in constructors for clarity
4. Protect all shared state with appropriate synchronization primitives
5. Document lock ordering invariants

## Implementation Plan

Phase 1: AsyncInferenceEngine
- Fix engine_start_time_ data race
- Standardize atomic operations
- Add explicit plugin_mutex_ in constructors

Phase 2: ActiveVRAMAllocator
- Ensure consistent locking on all stats_ access
- Verify atomic operations use correct memory ordering

Phase 3: LLMPluginManager
- Add mutex for vram-related operations
- Protect adapter_publisher_ access

