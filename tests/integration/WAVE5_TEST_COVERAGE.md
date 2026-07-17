# Wave 5 Test Coverage — ThemisDB

> **Wave:** 5 — Pre-Production Confidence, E2E Reliability  
> **Delivered:** 2026-07-16  
> **Tests added:** 16 (8 E2E journey + 8 failure/recovery)

---

## Summary

Wave 5 delivers the final pre-production test layer before release: robust
end-to-end coverage of the 8 highest-priority cross-component journeys
(W5-A) and targeted failure-injection / recovery validation for the 8 most
probable production failure modes (W5-B).

All tests are offline, deterministic, and registered as `release_critical`
CTest labels so they are mandatory on every PR.

---

## Coverage Matrix

### W5-A  Production-Critical E2E Journeys

| Test ID | Journey | Covered? | Notes |
|---------|---------|----------|-------|
| E2E-01 | Ingest → index → query → audit (happy path) | ✅ | Full pipeline, single document |
| E2E-02 | Auth: denied token vs. valid token | ✅ | Both paths tested in one case |
| E2E-03 | RAG: embed → retrieve → infer → score | ✅ | Relevance score assertion |
| E2E-04 | Multi-shard TX commit + CDC | ✅ | 3-key atomic commit |
| E2E-05 | Batch checkpoint + resume, idempotent | ✅ | Re-ingest does not duplicate |
| E2E-06 | Security: no data leak on denied query | ✅ | Storage size invariant checked |
| E2E-07 | Concurrent writes, 8 threads | ✅ | TSan-clean with std::atomic |
| E2E-08 | Schema-invalid doc rejected at boundary | ✅ | Missing "title" triggers error |

### W5-B  Failure Injection & Recovery

| Test ID | Failure Mode | Covered? | Recovery Verified |
|---------|-------------|----------|------------------|
| FIR-01 | LLM embedding failure | ✅ | Clean `embedding_failed`, no write |
| FIR-02 | LLM inference failure | ✅ | Embed tried once; no answer stored |
| FIR-03 | Empty transaction | ✅ | `empty_tx` status, no side-effects |
| FIR-04 | Missing document ID | ✅ | Rejected before any storage write |
| FIR-05 | Index miss (term not indexed) | ✅ | Empty hits, no error propagation |
| FIR-06 | Auth recovery: denied → re-authorised | ✅ | Same token succeeds after grant |
| FIR-07 | Mixed-validity batch | ✅ | Valid docs succeed; invalid isolated |
| FIR-08 | RAG no-hits (embed ok) | ✅ | `no_documents`; inference not called |

---

## Known Residual Gaps

The following scenarios are intentionally deferred to a future wave or
require infrastructure not available in offline CI:

| Gap ID | Description | Mitigation | Target Wave |
|--------|-------------|------------|-------------|
| R-01 | Timeout simulation (slow backend) | Covered by proxy: FIR-01/02 cover hard failure | Wave 6 |
| R-02 | Resource exhaustion (OOM / disk full) | Needs platform-level injector | Wave 6 |
| R-03 | Real RocksDB integration | Needs `THEMIS_ENABLE_ROCKSDB_INTEGRATION_TESTS` | Wave 6 |
| R-04 | Multi-node / split-brain replication | Needs Raft mock with partition events | Wave 6 |
| R-05 | Backoff-curve verification (retry delay) | Partially covered by FIR-06 | Wave 6 |

---

## Infrastructure Reuse

Wave 5 tests build exclusively on shared infrastructure from previous waves:

| Component | File | Used by |
|-----------|------|---------|
| `IntegrationTestFixture` | `test_fixture.h` | W5-A, W5-B |
| `InMemoryPipelineStorage` | `test_fixture.h` | W5-A, W5-B |
| `MockPipelineIndex` | `test_fixture.h` | W5-A, W5-B |
| `MockPipelineAuth` | `test_fixture.h` | W5-A, W5-B |
| `MockPipelineLlmBackend` | `test_fixture.h` | W5-A, W5-B (failure injection) |
| `PipelineAuditLog` | `test_fixture.h` | W5-A, W5-B |
| `TestDataGenerator` | `test_data_generator.h` | W5-A, W5-B |

No new shared helpers were required; the existing mock set was sufficient.

---

## Test Execution Commands

```bash
# All Wave 5 tests
ctest --preset linux-release -L "wave5" --output-on-failure

# Only E2E journeys (W5-A)
ctest --preset linux-release -L "w5a" --output-on-failure

# Only failure/recovery (W5-B)
ctest --preset linux-release -L "w5b" --output-on-failure

# All release-critical tests (includes waves 1–5)
ctest --preset linux-release -L "release_critical" --output-on-failure
```

---

## See Also

- [`WAVE5_TEST_GOVERNANCE.md`](WAVE5_TEST_GOVERNANCE.md) — gates, triage runbook, ownership
- [`INTEGRATION_TEST_GUIDELINES.md`](INTEGRATION_TEST_GUIDELINES.md) — conventions and patterns
- [`tests/integration/pipeline/w5a_e2e_critical_journeys_test.cpp`](pipeline/w5a_e2e_critical_journeys_test.cpp)
- [`tests/integration/pipeline/w5b_failure_injection_recovery_test.cpp`](pipeline/w5b_failure_injection_recovery_test.cpp)
