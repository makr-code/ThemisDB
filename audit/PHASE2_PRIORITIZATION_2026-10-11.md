---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Phase 2 Prioritization - In Progress"
---

# Phase 2: Prioritization & Test Strategy (2026-10-11)

## Overview

Based on Phase 1 audit findings, this document defines the prioritized order for test implementation to reach 100% coverage.

---

## Priority Tiers

### TIER A: Release-Critical Modules (Target: >80% Coverage)

These modules are part of the core production system and require extensive test coverage.

#### Priority A1: SERVER (HIGHEST)
- **LOC:** 101,450
- **Current Tests:** 24
- **Ratio:** 1 test per 4,227 LOC
- **Status:** ❌ CRITICALLY SPARSE
- **Gap Estimate:** 80-120 additional tests
- **CTest Categories Required:**
  1. **Function-Level Tests** (50-60 tests)
     - Request parser: valid payloads, malformed input, edge sizes
     - Router: path matching, HTTP method validation, parameter extraction
     - Handler classes: auth check, rate limiting, request context
     - Error handler: all HTTP status codes, custom error formats
  2. **Module-Level Tests** (20-25 tests)
     - Server startup/shutdown sequences
     - Config loading and validation
     - Port binding and reuse
     - Graceful shutdown (in-flight request handling)
  3. **Cross-Module Tests** (5-10 tests)
     - Server→Auth integration (token validation flow)
     - Server→Query integration (request routing to query engine)
     - Server→Transaction integration (ACID compliance via API)
  4. **Happy-Path Tests** (10-15 tests)
     - Simple GET/POST/PUT/DELETE cycles
     - Basic auth flow
     - Normal load scenarios (10-100 concurrent requests)
  5. **Failure/Error-Path Tests** (10-15 tests)
     - Connection timeouts, client disconnects
     - Malformed requests (invalid JSON, oversized payloads)
     - Authentication failures, authorization denials
     - Backend service unavailable
     - Database connection loss during request
  6. **Edge-Case Tests** (5-10 tests)
     - Empty payloads, NULL fields
     - Maximum field lengths, boundary values
     - Unicode/encoding edge cases
     - Concurrent requests on same connection
     - Multiple rapid connect/disconnect cycles
- **Risk:** Core functionality may have hidden bugs

#### Priority A2: SHARDING (HIGH)
- **LOC:** 66,180
- **Current Tests:** 30
- **Ratio:** 1 test per 2,206 LOC
- **Status:** ⚠️ SPARSE
- **Gap Estimate:** 80-120 additional tests
- **CTest Categories Required:**
  1. **Function-Level Tests** (50-60 tests)
     - Shard assignment: hash function correctness, distribution fairness
     - Rebalancing algorithm: move calculations, quorum decisions
     - Partition detection: boundary identification, overlap checking
     - Shard selector: consistent hashing, hot-spot handling
  2. **Module-Level Tests** (20-25 tests)
     - Shard group state machine: transitions, metadata updates
     - Rebalancing state: progress tracking, rollback capability
     - Gossip protocol: message exchange, view convergence
  3. **Cross-Module Tests** (10-15 tests)
     - Sharding→Storage: shard range queries, compaction across shards
     - Sharding→Transaction: cross-shard transaction coordination
     - Sharding→Query: distributed scan, merge results
  4. **Happy-Path Tests** (15-20 tests)
     - Uniform data distribution
     - Balanced rebalancing (add/remove node)
     - Normal failover scenarios
  5. **Failure/Error-Path Tests** (15-20 tests)
     - Node crash during rebalancing
     - Network partition (split brain detection)
     - Quorum loss, minority partition behavior
     - Invalid shard metadata
  6. **Edge-Case Tests** (10-15 tests)
     - Single-shard cluster (no replication)
     - Empty shards, all-data-in-one-shard
     - Boundary keys (min/max hash values)
     - Concurrent rebalancing + failover
- **Risk:** Distributed system bugs are hard to reproduce

#### Priority A3: STORAGE (MEDIUM-HIGH)
- **LOC:** 44,678
- **Current Tests:** 21
- **Ratio:** 1 test per 2,128 LOC
- **Status:** ⚠️ SPARSE
- **Gap Estimate:** 60-100 additional tests
- **CTest Categories Required:**
  1. **Function-Level Tests** (35-40 tests)
     - Put/Get/Delete primitives: valid keys/values, return codes
     - Compaction triggering: size thresholds, timing
     - Recovery functions: log replay, index rebuild
     - TTL/GC: expiration detection, cleanup
  2. **Module-Level Tests** (15-20 tests)
     - Write path: commit log → memtable → SST
     - Read path: cache → index → disk I/O
     - Compaction process: concurrent read during compaction
     - Recovery from corrupted state
  3. **Cross-Module Tests** (8-12 tests)
     - Storage→Query: range scans with compaction
     - Storage→Sharding: shard-aware compaction
     - Storage→Transaction: MVCC isolation
  4. **Happy-Path Tests** (10-15 tests)
     - Normal write/read cycles
     - Successful compaction
     - Clean recovery after graceful shutdown
  5. **Failure/Error-Path Tests** (10-15 tests)
     - Out of disk space (mid-compaction, mid-write)
     - Corrupted block detection
     - Failed fsync (data durability)
     - Read from deleted key
  6. **Edge-Case Tests** (8-12 tests)
     - Empty database, single key
     - Maximum key/value sizes
     - Concurrent compaction + writes
     - Rapid put-delete-put cycles

#### Priority A4: QUERY (MEDIUM)
- **LOC:** 49,770
- **Current Tests:** 61
- **Ratio:** 1 test per 816 LOC (acceptable)
- **Status:** ✓ BASELINE OK
- **Gap Estimate:** 20-40 additional tests (edge cases)
- **CTest Categories (Increment):**
  1. **Function-Level** (+15-20): Parser edge cases, optimizer rules
  2. **Failure/Error** (+10-15): Syntax errors, type mismatches, overflow
  3. **Edge-Case** (+5-10): Empty result sets, NULL handling, max recursion
  4. **Cross-Module** (+2-5): Integration with Storage and Sharding

#### Priority A5: TRANSACTION (MEDIUM)
- **LOC:** 14,405
- **Current Tests:** 32
- **Ratio:** 1 test per 450 LOC (good)
- **Status:** ✓ GOOD
- **Gap Estimate:** 10-20 additional tests (isolation levels)
- **CTest Categories (Increment):**
  1. **Function-Level** (+5-8): Isolation level enforcement
  2. **Failure/Error** (+5-10): Deadlock scenarios, rollback failures
  3. **Cross-Module** (+2-4): MVCC with Storage, consistency

#### Priority A6: AUTH (MEDIUM)
- **LOC:** 21,538
- **Current Tests:** 26
- **Ratio:** 1 test per 828 LOC (good)
- **Status:** ✓ GOOD
- **Gap Estimate:** 10-20 additional tests (edge cases)
- **CTest Categories (Increment):**
  1. **Function-Level** (+8-12): Token validation, permission checks
  2. **Failure/Error** (+5-8): Expired tokens, invalid signatures, replay attacks
  3. **Edge-Case** (+2-5): Timezone edge cases, large permission sets

---

### TIER C: Minimal-Test Modules (1 test each) → CRITICAL FOR PHASE 3

#### Priority C1: DISTRIBUTED_TENSOR (CRITICAL)
- **LOC:** 3,453
- **Current Tests:** 1
- **Target:** 25-30 tests
- **Effort:** ~2 weeks
- **CTest Categories (Target Distribution):**
  1. **Function-Level** (40%): 10-12 tests
     - Tensor creation (empty, sized, from data)
     - Indexing/slicing (1D/2D/3D, boundary, out-of-bounds)
     - Element access (valid indices, invalid ranges)
     - Reshape operations (same size, different dims, incompatible)
  2. **Module-Level** (20%): 5-6 tests
     - Distributed allocation across nodes
     - Local memory management
     - Serialization state machine
  3. **Cross-Module** (10%): 2-3 tests
     - Integration with GPU module (if applicable)
  4. **Happy-Path** (15%): 4-5 tests
     - Create→reshape→compute→destroy cycle
     - Distributed matrix multiplication
  5. **Failure/Error** (10%): 2-3 tests
     - Shape mismatch errors
     - Out-of-memory handling
     - Invalid dimension arguments
  6. **Edge-Case** (5%): 1-2 tests
     - Empty tensors (shape [0, N])
     - Very large tensors
     - Scalar tensors

#### Priority C2: LLAMA_CPP (CRITICAL)
- **LOC:** 1,506
- **Current Tests:** 1
- **Target:** 20-25 tests
- **Effort:** ~1-2 weeks
- **CTest Categories (Target Distribution):**
  1. **Function-Level** (45%): 9-11 tests
     - Load model (valid path, missing file, corrupted)
     - Tokenize (valid text, empty, special chars)
     - Inference run (valid context, insufficient memory)
  2. **Module-Level** (20%): 4-5 tests
     - Context management (create, reset, limit)
     - State persistence (save/load checkpoint)
  3. **Happy-Path** (15%): 3-4 tests
     - Load→tokenize→infer→cleanup
     - Streaming token generation
  4. **Failure/Error** (12%): 2-3 tests
     - Model not found
     - OOM during inference
     - Invalid prompt format
  5. **Edge-Case** (8%): 1-2 tests
     - Empty prompt, max context length
     - Special tokens in tokenization

#### Priority C3: STABLE_DIFFUSION (CRITICAL)
- **LOC:** 1,060
- **Current Tests:** 1
- **Target:** 15-20 tests
- **Effort:** ~1-2 weeks
- **CTest Categories (Target Distribution):**
  1. **Function-Level** (45%): 7-9 tests
     - Generate image (valid prompt, empty, too long)
     - Set parameters (valid ranges, boundary values)
     - Model loading
  2. **Module-Level** (20%): 3-4 tests
     - Pipeline state (init→generate→cleanup)
  3. **Happy-Path** (15%): 2-3 tests
     - Text→image generation
  4. **Failure/Error** (12%): 2 tests
     - GPU memory exceeded
     - Invalid prompt encoding
  5. **Edge-Case** (8%): 1-2 tests
     - Empty prompt, max prompt length
     - Concurrent generation (thread safety)

#### Priority C4: EXECUTION (HIGH)
- **LOC:** 494
- **Current Tests:** 1
- **Target:** 10-15 tests
- **Effort:** ~1 week
- **CTest Categories (Target Distribution):**
  1. **Function-Level** (50%): 5-8 tests
     - Execute task (valid, invalid, timeout)
     - Cancel execution
  2. **Module-Level** (20%): 2-3 tests
     - Task queue state management
  3. **Happy-Path** (15%): 1-2 tests
     - Queue→execute→complete cycle
  4. **Failure/Error** (10%): 1 test
     - Task failure, cancellation
  5. **Edge-Case** (5%): 1 test
     - Empty task, max timeout

---

## CTest-Style Test Taxonomy (REQUIRED FOR ALL PHASES)

### Test Categories (Mandatory per Module)

Every test implementation must categorize tests using this framework:

#### 1. **Function-Level Tests** (Core API coverage)
- Test individual public functions in isolation
- Mock dependencies (databases, external services)
- Verify input validation, output correctness
- Cover all parameters and return types
- Example: `test_query_parser_valid_sql()`, `test_storage_write_empty_key()`

#### 2. **Module-Level Tests** (Component integration)
- Test classes/subsystems as cohesive units
- Use real internal state (e.g., hash tables, caches)
- Mock only external boundaries (I/O, network)
- Verify state transitions and invariants
- Example: `test_server_startup_shutdown()`, `test_storage_flush_cycle()`

#### 3. **Cross-Module Tests** (Dependency coupling)
- Test interaction between 2+ modules
- Verify contracts at module boundaries
- Use realistic data flow (not simplified mocks)
- Validate error propagation across boundaries
- Example: `test_server_auth_integration()`, `test_query_storage_scan()`

#### 4. **Happy-Path Tests** (Normal operation)
- Most common use case with valid input
- Expected success outcome
- Minimal error handling
- Performance baseline (no stress)
- Example: `test_auth_valid_token()`, `test_transaction_simple_commit()`

#### 5. **Failure/Error-Path Tests** (Error handling)
- Invalid input (type errors, out-of-range)
- Resource exhaustion (OOM, connection limits)
- Dependency failures (service unavailable, timeout)
- Partial failures (mid-transaction crash)
- Verify error codes, messages, logging
- Example: `test_query_invalid_syntax()`, `test_storage_write_no_space()`

#### 6. **Edge-Case Tests** (Boundary conditions)
- Empty inputs/outputs (zero-length arrays, NULL values)
- Boundary values (INT_MAX, MIN, size limits)
- Special characters (encoding, escaping)
- Concurrent access patterns
- Memory pressure (large allocations)
- Example: `test_parser_empty_string()`, `test_index_max_key_size()`

### Test Distribution Guidelines

For each module, distribute tests as follows:

| Test Category | Minimal Module | Standard Module | Critical Module |
|---|---|---|---|
| Function | 40% | 50% | 40% |
| Module | 20% | 15% | 20% |
| Cross-Module | 10% | 10% | 15% |
| Happy-Path | 15% | 15% | 10% |
| Failure/Error | 10% | 5% | 10% |
| Edge-Case | 5% | 5% | 5% |
| **TOTAL** | 100% | 100% | 100% |

### Required Test Coverage per CTest Category

**Tier A (Release-Critical):**
- Function: ≥90% of public functions tested
- Module: ≥80% state transitions covered
- Cross-Module: ≥70% boundary contracts tested
- Happy-Path: ≥95% of normal workflows passing
- Failure/Error: ≥80% of error codes/exceptions triggered
- Edge-Case: ≥60% of boundary values tested

**Tier B (Important):**
- Function: ≥80% of public functions tested
- Module: ≥70% state transitions covered
- Cross-Module: ≥60% boundary contracts tested
- Happy-Path: ≥90% of normal workflows passing
- Failure/Error: ≥70% of error codes/exceptions triggered
- Edge-Case: ≥50% of boundary values tested

**Tier C (Optional):**
- Function: ≥60% of public functions tested
- Module: ≥50% state transitions covered
- Cross-Module: ≥40% boundary contracts tested
- Happy-Path: ≥80% of normal workflows passing
- Failure/Error: ≥50% of error codes/exceptions triggered
- Edge-Case: ≥30% of boundary values tested

---

### Phase 3 Batch 1: Minimal-Test Modules (2-3 weeks)
**Modules:** distributed_tensor, llama_cpp, stable_diffusion, execution
**Target Tests:** ~80-90 new tests
**Approach:**
1. For each module, analyze 5-10 core entry points
2. Create unit test fixtures for common operations
3. Add edge case tests (empty inputs, boundary values, errors)
4. Verify test isolation (no shared state)
5. Validate test runtime (ensure quick feedback loop)

**Definition of Done:**
- Each module has ≥20 tests
- Each test takes <100ms (for quick iteration)
- All core public APIs have at least 2 tests (happy path + error)

### Phase 3 Batch 2: Sparse Tier A Coverage (4-6 weeks)
**Modules:** Server, Sharding, Storage
**Target Tests:** ~200-300 new tests
**Approach:**
1. Profile which code paths are currently untested (coverage report needed)
2. Create test scenarios for high-impact paths:
   - Server: connection handling, request routing, error responses
   - Sharding: rebalancing, cross-shard consistency, failover
   - Storage: compaction, recovery, concurrent access
3. Add stress tests (high concurrency, large data)
4. Create regression tests for known bugs

**Definition of Done:**
- Server: ≥100 tests, ≥80% line coverage
- Sharding: ≥100 tests, ≥80% line coverage
- Storage: ≥80 tests, ≥80% line coverage

### Phase 3 Batch 3: TODO→Test Conversion (3-4 weeks)
**Focus:** rag module (30 actionable TODOs)
**Target Tests:** ~50-100 new tests
**Approach:**
1. For each TODO in ACTIONABLE_TODOS_2026-10-01.md:
   - Read the TODO description
   - Create a unit test that verifies the expected behavior
   - Convert stub/mock code to real implementation if needed
2. Prioritize by impact:
   - Serialization (high impact on data consistency)
   - Persistence (high impact on recovery)
   - Telemetry (low impact, good for quick wins)
3. Use property-based testing for serialization
4. Create fixtures for common test data

**Definition of Done:**
- All 30 TODOs have corresponding tests
- rag module has ≥120 tests total (from 70→190)

### Phase 3 Batch 4: Integration & Soak Tests (2-3 weeks)
**Modules:** Cross-module workflows
**Target Tests:** ~30-50 new tests
**Approach:**
1. Create end-to-end scenarios:
   - Server→Query→Transaction workflow
   - Ingestion→Storage→Sharding→Serving path
   - Multi-node failover with data recovery
2. Soak tests: run realistic workload for hours
3. Chaos tests: inject failures at each step

**Definition of Done:**
- ≥5 end-to-end scenarios with 5+ variations each
- ≥3 soak test configurations
- All tests pass consistently (no flakes)

---

## Coverage Goals by Phase

| Phase | Tier A Avg | Tier B Avg | Tier C Avg | Total Est. Tests |
|-------|-----------|-----------|-----------|------------------|
| **Current** | 60% | 75% | 5% | 1,562 binaries |
| **After P3.1** | 60% | 75% | 60%+ | +80-90 |
| **After P3.2** | 80%+ | 75% | 60% | +200-300 |
| **After P3.3** | 80%+ | 85%+ | 60% | +50-100 |
| **After P3.4** | 80%+ | 85%+ | 65%+ | +30-50 |
| **FINAL (P4)** | 85%+ | 90%+ | 75%+ | ~2,000 binaries |

---

## Effort Allocation

| Module | Owner Estimate | Tests | Effort |
|--------|---|---|---|
| distributed_tensor | 25 tests | ~3-4 wks |
| llama_cpp | 20 tests | ~2-3 wks |
| stable_diffusion | 15 tests | ~2-3 wks |
| execution | 10 tests | ~1 wk |
| **Subtotal (Batch 1)** | **70 tests** | **8-11 wks** |
| server | 100 tests | ~4-5 wks |
| sharding | 100 tests | ~4-5 wks |
| storage | 80 tests | ~3-4 wks |
| **Subtotal (Batch 2)** | **280 tests** | **11-14 wks** |
| rag (STUB→Test) | 50 tests | ~3-4 wks |
| other modules | 50 tests | ~2-3 wks |
| **Subtotal (Batch 3)** | **100 tests** | **5-7 wks** |
| integration + soak | 40 tests | ~2-3 wks |
| **Subtotal (Batch 4)** | **40 tests** | **2-3 wks** |
| **CI/CD Gate (Phase 4)** | - | ~1-2 wks |
| **GRAND TOTAL** | **~490 tests** | **27-37 weeks (~2-3 mo)** |

---

## Risk Mitigation

### Risk: Test Maintenance Burden
**Mitigation:**
- Use shared test fixtures to reduce duplication
- Create test helper libraries for common scenarios
- Automate flake detection and fix

### Risk: Coverage Gaps Remain Hidden
**Mitigation:**
- Require coverage reports for each batch
- Use mutation testing to validate test quality
- Property-based testing for critical paths

### Risk: Tests Don't Catch Real Bugs
**Mitigation:**
- Chaos testing (failure injection)
- Soak tests (long-running stability)
- Fuzzing for parsers and protocol handlers

### Risk: Testing External Dependencies (GPU, LLM)
**Mitigation:**
- Mock heavy dependencies in unit tests
- Create separate integration test configs with real dependencies
- Use CI/CD matrix to test both paths

---

## Success Criteria

### For Phase 2 (THIS DOCUMENT):
- [x] Identified priority modules by tier and coverage ratio
- [x] Defined test targets for each module
- [ ] Estimated effort (2-3 months total)
- [ ] Created prioritized backlog for Phase 3

### For Phase 3 (IMPLEMENTATION):
- [ ] Batch 1: distributed_tensor, llama_cpp, stable_diffusion, execution reach 20-30 tests each
- [ ] Batch 2: Server, Sharding, Storage reach 80-100 tests each with 80%+ coverage
- [ ] Batch 3: All 30 RAG TODOs converted to concrete tests
- [ ] Batch 4: 40+ integration/soak tests passing consistently

### For Phase 4 (CI/CD):
- [ ] Coverage gate active (fail if Tier A < 75%)
- [ ] Nightly coverage reports generated
- [ ] Coverage badge in README
- [ ] All PRs show coverage delta

---

## Next Steps

1. **Immediate:** Team review and approval of priorities
2. **Week 1:** Set up coverage measurement locally (gcov/lcov)
3. **Week 2:** Start Phase 3 Batch 1 (minimal modules)
4. **Weeks 3-4:** Parallel work on Batch 2 (Server, Sharding)
5. **Ongoing:** CI/CD integration in background

---

*Report Generated: 2026-10-11 09:30 UTC*
*Status: Ready for Phase 3 Implementation*
