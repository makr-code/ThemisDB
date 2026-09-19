# Test Suite Navigation & Gate Definitions

**Status:** Batch 6 Phase 6.3 — Test Suite Navigation  
**Date:** 2026-08-14  
**Scope:** Test organization, gate definitions, and module-to-test mapping

---

## Overview

This document provides comprehensive navigation of the ThemisDB test suite, including test organization, gate definitions, module-to-test mapping, and execution guides for developers and CI/CD systems.

---

## Test Suite Organization

### Directory Structure

```
tests/
├── access_model/          # Access control & RBAC tests
├── acceleration/          # GPU/CUDA acceleration tests  
├── analytics/             # Analytics & aggregation tests
├── auth/                  # Authentication & token tests
├── cache/                 # Cache concurrency & eviction tests
├── cdc/                   # Change data capture tests
├── content/               # Content storage & versioning tests
├── distributed_knowledge/ # Knowledge graph sync tests
├── distributed_tensor/    # Distributed tensor operation tests
├── document/              # Document versioning tests
├── ethics_ai/             # AI ethics & bias detection tests
├── evaluation/            # Model evaluation tests
├── failover/              # Failover detection & promotion tests
├── governance/            # Policy & compliance tests
├── gpu/                   # GPU operations & fallback tests
├── graph/                 # Graph storage & query tests
├── image_analysis/        # ONNX/CLIP inference tests
├── importers/             # Format-specific import tests
├── index/                 # Indexing strategy tests
├── ingestion/             # Ingestion pipeline tests
├── llm/                   # LLM inference tests
├── llm_streaming/         # LLM streaming tests
├── llm_wiki/              # RAG-augmented LLM tests
├── maintenance/           # Maintenance & compaction tests
├── network/               # RPC/gRPC protocol tests
├── observability/         # Observability & tracing tests
├── process/               # Transaction & process tests
├── prompt_engineering/    # Prompt template tests
├── query/                 # Query planning & execution tests
├── rag/                   # RAG pipeline tests
├── replication/           # Replication & WAL tests
├── retrieval/             # Vector & sparse search tests
├── search/                # Full-text search tests
├── security/              # Security & encryption tests
├── sharding/              # Sharding & topology tests
├── storage/               # Storage engine & recovery tests
├── timeseries/            # Time series tests
├── updates/               # Update coordination tests
├── voice/                 # Voice & audio tests
└── <module>/
    ├── test_<module>_core_unit.cpp              # Unit tests for core functionality
    ├── test_<module>_integration_focused.cpp    # Integration tests (focused, <50 tests)
    ├── test_<module>_chaos_focused.cpp          # Chaos/fault-injection tests
    ├── test_<module>_thread_safety_focused.cpp  # Concurrency & thread-safety tests
    ├── test_<module>_edge_cases_focused.cpp     # Edge case & error handling tests
    ├── test_<module>_performance_focused.cpp    # Performance & benchmark tests
    ├── test_<module>_wave_<X>_gate_<N>.cpp      # Wave-specific gate tests
    └── README.md                                # Test suite documentation
```

---

## Test Gate Definition Model

### Gate Naming Convention

```
<MODULE_PREFIX>-<CATEGORY>-<NUMBER>
```

**Examples:**
- `STR-01` — Storage Engine, Phase/Feature 1
- `REPL-Recovery-01` — Replication, Recovery category
- `AUTH-Token-03` — Auth, Token category
- `SGRG-01` — Sharding Gate (Regression), gate 1

### Test Gate Categories

| Category | Abbreviation | Purpose | Example Gates |
|----------|--------------|---------|----------------|
| Unit | `U` | Single-function correctness | `STR-U-01..10` |
| Integration | `I` | Multi-module interaction | `STR-I-01..05` |
| Chaos/Fault-Injection | `C` | Failure scenario recovery | `STR-C-01..03` |
| Thread-Safety | `TS` | Concurrency correctness | `STR-TS-01..05` |
| Wave A Gate | `A` | Runtime reliability criteria | `STR-A-01..04` |
| Wave B Gate | `B` | Performance consolidation | `REPL-B-01..03` |
| Wave C Gate | `C` | Security validation | `SEC-C-01..06` |
| Regression | `GRG` | Benchmark baseline regression | `SGRG-01..10` |

---

## Gate Definition: Storage Module (Example)

### Unit Tests (STR-U-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **STR-U-01** | Basic write/read correctness | test_storage_core_unit.cpp | Write 100 keys, read all correctly |
| **STR-U-02** | ACID property: Atomicity | test_storage_core_unit.cpp | Transaction rollback restores initial state |
| **STR-U-03** | ACID property: Consistency | test_storage_core_unit.cpp | No partial writes visible |
| **STR-U-04** | Key collision handling | test_storage_core_unit.cpp | Collision resolution deterministic |
| **STR-U-05** | Large value (LOB) handling | test_storage_core_unit.cpp | Store/retrieve 1GB values |

### Integration Tests (STR-I-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **STR-I-01** | Cache + Storage integration | test_storage_integration_focused.cpp | Cache hits reduce storage I/O |
| **STR-I-02** | Index + Storage integration | test_storage_integration_focused.cpp | Index queries use storage correctly |
| **STR-I-03** | Replication + Storage integration | test_storage_integration_focused.cpp | Replicated writes consistent |
| **STR-I-04** | Process (transaction) + Storage | test_storage_integration_focused.cpp | Multi-statement transaction atomicity |
| **STR-I-05** | Maintenance + Storage | test_storage_integration_focused.cpp | Compaction preserves data integrity |

### Chaos Tests (STR-C-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **STR-C-01** | Crash recovery: Full restart | test_storage_chaos_focused.cpp | Recover all data post-crash via WAL |
| **STR-C-02** | Partial write on crash | test_storage_chaos_focused.cpp | Incomplete transaction rolled back |
| **STR-C-03** | Corruption detection | test_storage_chaos_focused.cpp | Detect & report corrupted blocks |

### Thread-Safety Tests (STR-TS-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **STR-TS-01** | Concurrent writes (100 threads) | test_storage_thread_safety_focused.cpp | No data races (ThreadSanitizer) |
| **STR-TS-02** | Writer-reader contention | test_storage_thread_safety_focused.cpp | Readers don't block on write locks |
| **STR-TS-03** | Lock ordering determinism | test_storage_thread_safety_focused.cpp | No deadlocks over 10,000 iterations |

### Wave A Gate (STR-A-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **STR-A-01** | Crash recovery determinism | test_storage_wave_a_gate_1.cpp | 10× restart cycles, identical recovery |
| **STR-A-02** | Fail-closed on corruption | test_storage_wave_a_gate_2.cpp | Corruption prevents writes (write gate) |
| **STR-A-03** | Representative HW p95/p99 | test_storage_wave_a_gate_3.cpp | <10ms p95, <50ms p99 on RTX3090 |
| **STR-A-04** | `release_critical` CI integration | (CI workflow) | Gate CI status PASS |

### Regression (SGRG-XX)

| Gate | Definition | Test File | Acceptance Criteria |
|------|-----------|-----------|-------------------|
| **SGRG-01** | Throughput: 100K writes/sec (target) | benchmarks/storage_gate.cpp | ≥95K writes/sec baseline |
| **SGRG-02** | Latency p95: <10ms (target) | benchmarks/storage_gate.cpp | Maintain <10ms p95 |
| **SGRG-03** | Memory footprint <2GB (target) | benchmarks/storage_gate.cpp | Track regression |
| **SGRG-04** | Cache hit rate: 80%+ | benchmarks/storage_gate.cpp | Maintain cache efficiency |

---

## Module-to-Test Mapping (35 Documented Modules)

### Batch 1 Modules

| Module | Core Unit | Integration | Chaos | Thread-Safety | Wave Gates | Focused Tests Count |
|--------|-----------|-------------|-------|---------------|-----------|------------------|
| **access_model** | ✅ | ✅ | ⚠️ | ✅ | B-01..03 | 24 focused |
| **base** | ✅ | ✅ | ⚠️ | ✅ | — | 18 focused |
| **distributed_tensor** | ✅ | ✅ | ✅ | ✅ | A-01..03 | 28 focused |
| **evaluation** | ✅ | ✅ | ⚠️ | ✅ | — | 16 focused |
| **gpu** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **llm_wiki** | ✅ | ✅ | ⚠️ | ✅ | B-01..03 | 24 focused |
| **retrieval** | ✅ | ✅ | ✅ | ✅ | B-01..04 | 32 focused |

### Batch 2–5 Modules

| Module | Core Unit | Integration | Chaos | Thread-Safety | Wave Gates | Focused Tests Count |
|--------|-----------|-------------|-------|---------------|-----------|-----------------|
| **auth** | ✅ | ✅ | ✅ | ✅ | C-01..06 | 36 focused |
| **cache** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 28 focused |
| **content** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 26 focused |
| **failover** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **governance** | ✅ | ✅ | ✅ | ✅ | C-01..04 | 24 focused |
| **importers** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 26 focused |
| **index** | ✅ | ✅ | ✅ | ✅ | B-01..04 | 28 focused |
| **ingestion** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 28 focused |
| **llm** | ✅ | ✅ | ⚠️ | ✅ | B-01..03 | 24 focused |
| **maintenance** | ✅ | ✅ | ✅ | ✅ | A-01..03 | 26 focused |
| **network** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 30 focused |
| **process** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **query** | ✅ | ✅ | ✅ | ✅ | B-01..04 | 32 focused |
| **rag** | ✅ | ✅ | ⚠️ | ✅ | B-01..03 | 24 focused |
| **replication** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **search** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 28 focused |
| **security** | ✅ | ✅ | ✅ | ✅ | C-01..06 | 36 focused |
| **sharding** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **storage** | ✅ | ✅ | ✅ | ✅ | A-01..04 | 32 focused |
| **updates** | ✅ | ✅ | ✅ | ✅ | A-01..03 | 26 focused |
| **analytics** | ✅ | ✅ | ⚠️ | ✅ | B-01..03 | 24 focused |
| **distributed_knowledge** | ✅ | ✅ | ✅ | ✅ | B-01..03 | 26 focused |
| Other modules | ✅ | ✅ | ⚠️ | ✅ | — | 16–24 each |

---

## Test Execution Guides

### Running All Tests

```bash
# Configure and build (Windows)
cmake --preset windows-release
cmake --build --preset windows-release --parallel 16

# Run all tests
ctest --preset windows-release --output-on-failure -j 1 --timeout 60

# Or run specific module tests
ctest --preset windows-release -R "storage" --output-on-failure -j 1
```

### Running Wave-Specific Gates

```bash
# Wave A gates (runtime reliability)
ctest --preset windows-release -R "wave_a_gate" --output-on-failure

# Wave B gates (performance)
ctest --preset windows-release -R "wave_b_gate" --output-on-failure

# Wave C gates (security)
ctest --preset windows-release -R "wave_c_gate" --output-on-failure
```

### Running Focused Test Suites

```bash
# Integration tests only (fast, ~2 min)
ctest --preset windows-release -R "integration_focused" --output-on-failure -j 4

# Chaos tests (slow, ~15 min)
ctest --preset windows-release -R "chaos_focused" --output-on-failure -j 1

# Thread-safety tests (slow, ~20 min)
ctest --preset windows-release -R "thread_safety_focused" --output-on-failure -j 1
```

### CI Integration

The `release_critical` workflow automatically runs:
- All Wave A gates (high priority)
- All Wave B gates (performance)
- Representative module tests (sample)
- Regression benchmarks

See `.github/workflows/09-pr-gates_release-critical-tests.yml` for CI configuration.

---

## Test Metrics

### Coverage by Module (35 Documented)

| Metric | Value | Status |
|--------|-------|--------|
| Total focused tests | 900+ | ✅ Complete |
| Avg tests per module | 26 focused | ✅ Good |
| Wave A gate coverage | 100% (11 modules) | ✅ Complete |
| Wave B gate coverage | 100% (14 modules) | ✅ Complete |
| Wave C gate coverage | 100% (3 modules) | ✅ Complete |
| Thread-safety coverage | 35/35 modules | ✅ Complete |
| Chaos coverage | 25/35 modules | ✅ 71% (good) |

### Test Execution Time (on ci-build Windows runner)

| Category | Count | Est. Time | Priority |
|----------|-------|-----------|----------|
| Unit tests | 350+ | <5 min | P0 (fast) |
| Integration focused | 200+ | 2–3 min | P0 (fast) |
| Thread-safety focused | 150+ | 15–20 min | P1 (slower) |
| Chaos focused | 100+ | 10–15 min | P1 (slower) |
| Performance focused | 50+ | 5–10 min | P2 (longest) |
| **Total** | **~850** | **40–60 min** | Sequential run |

---

## Related Documents

| Document | Purpose |
|----------|---------|
| BENCHMARK_GATES_REFERENCE.md | Benchmark gate definitions and SLO targets |
| WAVE_GATE_DASHBOARD.md | Wave A/B/C/D gate fulfillment tracking |
| Each module's ROADMAP.md | Phase-specific gate definitions |
| .github/workflows/09-pr-gates_release-critical-tests.yml | CI gate configuration |

---

**Batch 6 Status:** Phase 6.3 continuing. Moving to Benchmark Gates Reference.
