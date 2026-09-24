---
Author: copilot-swe-agent[bot]
Created: 2026-09-23
Last Updated: 2026-09-23
Status: active
---

# Phase 3: Refinement Tasks — Detailed Specification

**Purpose:** Complete remaining 12 actionable TODOs across Tensor, Governance, LLM, and Server modules.

**Status:** Ready for background agent execution (awaiting Phase 2B completion)

---

## Module 1: Tensor (5 TODOs)

### Task T1.1: Wire to actual TensorTrainDecomposer
**File:** `src/tensor/compression_strategy.cpp`  
**Line:** 48  
**Current:** `// TODO(tracked): Wire to actual TensorTrainDecomposer — see src/tensor/ROADMAP.md`

**Implementation Requirements:**
- Replace placeholder with actual TensorTrainDecomposer instance initialization
- Check `src/tensor/tensor_train_decomposer.h` for API
- Wire the decomposer into the compression strategy pattern
- Ensure proper initialization + error handling
- Reference: src/tensor/ROADMAP.md for design details

**Acceptance Criteria:**
- Decomposer instance created and stored
- No nullptr dereferences possible
- Proper error propagation for decomposition failures
- No new warnings on compilation

---

### Task T1.2: Implement strategy registry
**File:** `src/tensor/compression_strategy.cpp`  
**Line:** 344  
**Current:** `// TODO(tracked): Implement strategy registry — see src/tensor/ROADMAP.md`

**Implementation Requirements:**
- Create registry mechanism for compression strategies
- Support dynamic strategy lookup by ID/name
- Return null/error if strategy not found
- Thread-safe if registry is mutable
- Reference: src/tensor/ROADMAP.md, existing strategy patterns

**Acceptance Criteria:**
- Registry can store multiple strategies
- Lookup returns correct strategy or error
- No memory leaks (smart pointers)
- Singleton or thread-local pattern appropriate to module design

---

### Task T1.3: Parse created_at and age-based freshness decay
**File:** `src/tensor/tensor_routing_strategy.cpp`  
**Line:** 81  
**Current:** `// TODO(tracked): Parse created_at and compute age-based freshness decay`

**Implementation Requirements:**
- Parse `created_at` timestamp from tensor metadata
- Calculate age in seconds/minutes/hours as appropriate
- Compute freshness score: older tensors = lower freshness
- Implement decay formula (linear, exponential, or specified in ROADMAP)
- Return freshness score as float (0.0 = stale, 1.0 = fresh)

**Acceptance Criteria:**
- Timestamp parsed correctly from metadata
- Freshness score in range [0.0, 1.0]
- Decay formula matches specification
- Handle missing/invalid timestamps gracefully

---

### Task T1.4: Compute age-based freshness from timestamp
**File:** `src/tensor/tensor_routing_strategy.cpp`  
**Line:** 98  
**Current:** `// TODO(tracked): Compute age-based freshness from timestamp`

**Implementation Requirements:**
- Calculate tensor age (current time - created_at)
- Apply age thresholds (e.g., < 1 hour = fresh, > 24 hours = stale)
- Return normalized freshness metric
- Handle time zone / clock skew issues

**Acceptance Criteria:**
- Age calculation correct (verified with unit tests)
- Freshness metrics consistent with T1.3
- No undefined behavior on edge cases (very old tensors, future timestamps)

---

### Task T1.5: Adaptive learning with metrics tracking
**File:** `src/tensor/tensor_routing_strategy.cpp`  
**Line:** 290  
**Current:** `// TODO(tracked): Implement adaptive learning with metrics tracking`

**Implementation Requirements:**
- Track key metrics: hit rate, latency, cache misses, routing decisions
- Update routing strategy weights based on observed performance
- Implement learning algorithm (e.g., gradient descent, reinforcement learning, exponential moving average)
- Persist learned weights to storage if required
- Add logging for debugging + observability

**Acceptance Criteria:**
- Metrics collected without performance regression
- Strategy weights updated dynamically
- Learning converges over time
- No data races (use atomic operations or locks if multithreaded)
- Logging sufficient for debugging

---

## Module 2: Governance (2 TODOs)

### Task G2.1: Implement proper p95/p99 tracking with histogram
**File:** `src/governance/audit_batch_writer.cpp`  
**Line:** 578  
**Current:** `// TODO: Implement proper p95/p99 tracking with histogram`

**Implementation Requirements:**
- Create or use existing histogram class for latency/duration tracking
- Record operation latencies into histogram
- Calculate percentiles (p50, p95, p99) from histogram
- Export metrics in standard format (Prometheus, JSON, or module-specific)
- Track per operation type or globally as appropriate

**References:**
- Check if spdlog/fmt provides histogram functionality
- Review existing metrics infrastructure in src/governance/
- May need to add histogram library if not present

**Acceptance Criteria:**
- Percentile calculations accurate (verified against reference implementation)
- No performance regression in audit operations
- Memory efficient (histogram doesn't grow unboundedly)
- Metrics can be queried/exported
- Unit tests validate percentile accuracy

---

### Task G2.2: Implement actual rollback operation with policy manager
**File:** `src/governance/policy_change_manager.cpp`  
**Line:** 791  
**Current:** `// TODO: Implement actual rollback operation with policy manager`

**Implementation Requirements:**
- Implement rollback logic for policy changes
- Restore previous policy state from audit trail or savepoint
- Update all affected entities to previous policy version
- Log rollback operation for compliance + audit trail
- Handle failures gracefully (partial rollback recovery)

**References:**
- Check src/governance/policy_manager.h for transaction/savepoint API
- Review audit trail format in audit_batch_writer.cpp
- May need to coordinate with transaction module

**Acceptance Criteria:**
- Rollback restores exact previous state
- No orphaned policy entities after rollback
- Rollback operation is logged and auditable
- Error handling for partial rollback failures
- Unit/integration tests verify rollback correctness

---

## Module 3: LLM (1 TODO)

### Task L3.1: Migrate to binary/protobuf serialization
**File:** `src/llm/ssm_state_rocksdb_store.cpp`  
**Line:** 305  
**Current:** `// TODO(tracked): Migrate to binary/protobuf serialization — see src/llm/ROADMAP.md`

**Implementation Requirements:**
- Define protobuf schema for SSM state (or use existing .proto file)
- Implement serialization: SSM state C++ object → protobuf binary
- Implement deserialization: protobuf binary → SSM state C++ object
- Update RocksDB put/get calls to use new binary format
- Ensure backward compatibility if needed (version field in protobuf)
- Add migration path if old format data exists

**References:**
- src/llm/ROADMAP.md for design specification
- Look for src/llm/*.proto files for existing schemas
- Review existing protobuf usage patterns in codebase

**Acceptance Criteria:**
- Protobuf schema defined (single or multiple messages as appropriate)
- Serialization produces correct binary format
- Deserialization round-trip preserves all SSM state
- Size reduction vs current format (if applicable)
- Performance acceptable (benchmark if significant)
- No undefined behavior on corrupted protobuf data

---

## Module 4: Server (2 TODOs)

### Task S4.1: Wire setAggregatesProvider() after construction (W9-5)
**File:** `src/server/timeseries_api_handler.cpp`  
**Line:** 39  
**Current:** `// TODO(W9-5): Wire setAggregatesProvider() after construction so that...`

**Implementation Requirements:**
- Call `setAggregatesProvider()` on the timeseries API handler
- Execute this wiring after object construction completes
- Pass appropriate aggregates provider instance
- Ensure provider is valid + ready before setting
- Update Wave 9 Step 5 tracking if applicable

**References:**
- Check server/timeseries_api_handler.h for setAggregatesProvider() signature
- Determine where handler is constructed in server initialization flow
- May require passing provider through dependency injection

**Acceptance Criteria:**
- setAggregatesProvider() called exactly once during initialization
- Provider properly initialized before setAggregatesProvider() call
- No segfaults or use-of-uninitialized-value issues
- Wave 9 Step 5 requirement satisfied per ROADMAP.md
- Unit/integration tests verify aggregation works post-wiring

---

### Task S4.2: Remove after migration to cpp-httplib
**File:** `src/server/http_server.cpp`  
**Line:** 113  
**Current:** `#include "server/http_type_adapter.h"  // TODO: Remove after migration to cpp-httplib`

**Implementation Requirements:**
- Check if cpp-httplib migration is complete
- Replace http_type_adapter.h dependency with cpp-httplib equivalent
- Update all calls to http_type_adapter to use cpp-httplib APIs
- Remove the #include directive if no longer needed
- Update any dependent code

**References:**
- HTTP_SERVER_REFACTORING_ACTION_PLAN.md for migration status
- Review cpp-httplib API for feature parity
- Check if http_type_adapter.h can be completely removed

**Acceptance Criteria:**
- http_type_adapter.h no longer included (if migration complete)
- All HTTP operations work with cpp-httplib equivalent
- No API breakage
- Build succeeds with no unused include warnings
- HTTP functionality unchanged from user perspective

---

## Execution Model

### Background Agent Execution

**Agent:** phase-3-refinement-completion  
**Parallelism:** Executes all 12 TODOs in parallel where possible  
**Estimated Duration:** 20-30 minutes

### Dependencies & Ordering

- **T1.1 & T1.2** (Tensor decomposer + registry): Can run in parallel
- **T1.3, T1.4, T1.5** (Tensor routing): Depend on T1.1; can run in parallel after
- **G2.1 & G2.2** (Governance): Independent; can run in parallel
- **L3.1** (LLM serialization): Independent; can run in parallel
- **S4.1 & S4.2** (Server): Independent; can run in parallel

### Sequential Blocker (if any)

None identified. All 12 TODOs can execute in parallel without blocking dependencies.

---

## Quality Checklist (per TODO)

For each implementation:

- [ ] No TODO/FIXME/STUB markers remain
- [ ] No manual new/delete (smart pointers only)
- [ ] Error handling via Result<T> or equivalent
- [ ] Doxygen documentation present
- [ ] No new compiler warnings
- [ ] Unit tests added (or existing tests still pass)
- [ ] Integration tests verify end-to-end functionality
- [ ] Code review comments addressed

---

## Success Criteria (Phase 3 Complete)

- All 12 TODOs → production code
- Total hardening: **39/39 TODOs complete (100%)**
- Build succeeds with no new warnings
- All existing tests PASS
- No regressions vs prior commit
- Ready for PR merge + GA promotion

---

## References

- Audit baseline: `audit/ACTIONABLE_TODOS_2026-09-21.md`
- Phase 1 summary: `docs/governance/HARDENING_PHASE_1_2_SUMMARY_2026_09_23.md`
- Roadmap: `ROADMAP.md`, module-specific `src/*/ROADMAP.md`

---

_Specification prepared 2026-09-23T17:13Z_  
_Ready for Phase 3 background agent execution after Phase 2B (Neo4j) completion_
