# Wave 3 Test Coverage: Quality Gates & Flow Traceability

> **Version:** 0.0.1 | **Wave:** 3 | **Branch:** develop
> **Last updated:** 2026-07-16

## Purpose

This document maps critical system flows to the test suites that cover them,
defines quality gates for each critical component, and lists known coverage
gaps as of Wave 3 completion.

---

## 1. Critical Flow → Test Suite Mapping

### 1.1 Full-Function Critical Flows (W3-A)

| Test ID | Flow Description | Success Path | Failure Path | State Assertion |
|---------|-----------------|:------------:|:------------:|:---------------:|
| FFW-01  | Happy-path: Auth → Ingest → Index → LLM → Export | ✅ | — | ✅ |
| FFW-02  | Unauthorized ingest: no state mutation | — | ✅ | ✅ |
| FFW-03  | Query on missing index term: domain error | — | ✅ | ✅ |
| FFW-04  | Token revocation blocks all subsequent actions | ✅ (pre-revoke) | ✅ (post-revoke) | ✅ |
| FFW-05  | Concurrent ingest: no lost writes or duplicates | ✅ | — | ✅ |
| FFW-06  | LLM degradation fallback + recovery | ✅ (after recovery) | ✅ (during degradation) | ✅ |
| FFW-07  | Export snapshot content matches ingested event count | ✅ | — | ✅ |
| FFW-08  | Multi-tenant complete flow: strict cross-tenant isolation | ✅ | ✅ (cross-tenant read blocked) | ✅ |

**File:** `tests/integration/pipeline/w3a_full_function_critical_flows_test.cpp`
**CTest label:** `pipeline_integration;wave3;w3a`

---

### 1.2 Data/State Integrity & Recovery (W3-B)

| Test ID | Scenario | Idempotent | Recovery | Consistency |
|---------|----------|:----------:|:--------:|:-----------:|
| DIR-01  | Write-commit-snapshot round-trip | ✅ | — | ✅ |
| DIR-02  | Idempotent journal replay (double recovery) | ✅ | ✅ | ✅ |
| DIR-03  | Partial-failure: committed entries survive crash | — | ✅ | ✅ |
| DIR-04  | Uncommitted entries never appear in store | ✅ | ✅ | ✅ |
| DIR-05  | Snapshot checksum detects state mutation | — | — | ✅ |
| DIR-06  | Concurrent writes maintain journal uniqueness | ✅ | — | ✅ |
| DIR-07  | Empty-journal recovery is a no-op | ✅ | ✅ | ✅ |
| DIR-08  | Multi-key partial rollback preserves committed entries | — | ✅ | ✅ |

**File:** `tests/integration/pipeline/w3b_data_integrity_recovery_test.cpp`
**CTest label:** `pipeline_integration;wave3;w3b`

---

### 1.3 Prior Wave Coverage (Reference)

| Suite | Test IDs | Focus |
|-------|----------|-------|
| Application Profile E2E (Wave 1) | APP-01..APP-13 | User journey, tenant isolation, circuit breaker, retry |
| Query Execution Pipeline | QP-01..QP-05 | Auth, cache, index, storage, concurrent reads |
| Ingestion Pipeline | IP-01..IP-04 | Ingest, validation, dedup, CDC |
| RAG/AI Pipeline | RAG-01..RAG-04 | Embedding, retrieval, inference |
| Transaction/Replication | TXR-01..TXR-04 | 2PC, WAL, replication |
| Security Pipeline | SEC-01..SEC-06 | Auth, encryption, audit |
| Analytics/Export | AEP-01..AEP-03 | OLAP, streaming window, Arrow export |

---

## 2. Quality Gates

### 2.1 Per-Component Targets (Wave 3 Additions)

| Component | Gate | Covered by W3 |
|-----------|------|:-------------:|
| Auth (session, revocation) | All auth paths have a failure assertion | ✅ FFW-01..04, FFW-08 |
| Ingest (write, dedup, CDC) | Dedup idempotency + auth-reject no-mutation | ✅ FFW-02, DIR-01..02 |
| Index (full-text lookup) | Index miss returns domain error (not panic) | ✅ FFW-03 |
| LLM (inference + fallback) | Fallback activates on failure; LLM re-used on recovery | ✅ FFW-06 |
| Export (snapshot) | Snapshot file content byte-checked | ✅ FFW-07 |
| WAL Recovery | Committed-only replay after crash; uncommitted invisible | ✅ DIR-03..04, DIR-08 |
| Concurrent Safety | Concurrent writes: no lost writes, no duplicates | ✅ FFW-05, DIR-06 |
| Cross-Tenant Isolation | Both tenants' queries verified independently | ✅ FFW-08 |

### 2.2 Gate Criteria

A gate is considered **passing** when all of the following hold:

1. **Domain error clarity** — every failure path returns a named `error_code`
   or a typed error, not only `false` / null.
2. **No silent state mutation** — unauthorized or failed operations must leave
   storage, index, and CDC in the same state as before the call.
3. **Audit traceability** — every significant pipeline action is captured in
   the audit log (`PipelineAuditLog`).
4. **Snapshot consistency** — `ExportSnapshot()` output matches in-memory
   counters (event count == CDC count when ingest is the only write path).
5. **Recovery idempotency** — replaying the journal twice must produce the
   same state as replaying once.

---

## 3. Running Wave 3 Tests

### Run all Wave 3 tests

```bash
# Configure (Linux)
cmake --preset linux-release

# Build
cmake --build --preset linux-release --parallel

# Run W3-A + W3-B only
ctest --preset linux-release -L "wave3" --output-on-failure

# Run all pipeline integration tests (includes W3)
ctest --preset linux-release -L "pipeline_integration" --output-on-failure
```

### Run a single test binary

```bash
cd build/linux-release
./tests/integration/pipeline/w3a_full_function_critical_flows_test
./tests/integration/pipeline/w3b_data_integrity_recovery_test
```

### Timeout budget

Both Wave 3 suites run offline with deterministic in-memory mocks.
Expected execution time: **< 5 seconds** per binary.
CTest timeout is set to **300 seconds** (shared with all integration tests).

---

## 4. Known Coverage Gaps

| Area | Gap Description | Priority | Planned Wave |
|------|----------------|----------|-------------|
| RocksDB WAL | Real on-disk WAL replay not yet tested (requires `linux-release` build with RocksDB) | High | Wave 4 |
| gRPC service layer | End-to-end HTTP/gRPC request path not covered by pipeline tests | High | Wave 4 |
| LLM circuit breaker reset | Circuit breaker reset (from open → closed) after timeout not validated | Medium | Wave 4 |
| Token expiry | Time-based token expiry (as opposed to explicit revocation) not modelled | Medium | Wave 4 |
| Snapshot file corruption | Recovery from a corrupted snapshot file (partial write) | Medium | Wave 4 |
| Cross-shard consistency | Distributed shard-to-shard CDC consistency under partial failure | Low | Wave 5+ |

---

## 5. Assertion Style Guide

Wave 3 tests use **domain-level assertions**, not no-crash-only checks:

```cpp
// ✅ Domain-level assertion — preferred
EXPECT_EQ(qr.error_code, "index_miss")
    << "Query on missing term must return 'index_miss', not an empty result";

// ✅ Equivalent short form (semicolon on same line)
EXPECT_EQ(qr.error_code, "index_miss");

// ❌ No-crash-only — insufficient
EXPECT_FALSE(qr.ok);
```

All `EXPECT_*` and `ASSERT_*` calls include a failure message that names the
test ID and the exact invariant being checked.

---

*Maintained by the ThemisDB testing team as part of the Wave 3 test hardening
initiative. Update this document whenever new test IDs are added or gates
are adjusted.*
