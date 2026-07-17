# Wave 5 Test Governance — ThemisDB

> **Wave:** 5 — Pre-Production Confidence, E2E Reliability, Governance Maturity  
> **Status:** Active  
> **Last Updated:** 2026-07-16  
> **Owners:** QA / Platform Engineering

---

## 1. Purpose

Wave 5 establishes the release-mandatory test gates for production-critical
journeys and failure-recovery scenarios.  All tests in this wave must pass
on every PR targeting `develop`, `community`, or `enterprise` before merge.

---

## 2. Mandatory Release Gates

| Gate Label          | CTest Filter           | Required to Pass? |
|---------------------|------------------------|-------------------|
| `release_critical`  | `-L release_critical`  | ✅ YES — blocks merge |
| `wave5`             | `-L wave5`             | ✅ YES — blocks merge |
| `w5a`               | `-L w5a`               | ✅ YES — E2E journeys |
| `w5b`               | `-L w5b`               | ✅ YES — failure/recovery |
| `pipeline_integration` | `-L pipeline_integration` | ✅ YES — all pipeline suites |

### How to run gates locally

```bash
# Run only Wave 5 release-critical tests
ctest --preset linux-release -L "release_critical" --output-on-failure

# Run all Wave 5 tests
ctest --preset linux-release -L "wave5" --output-on-failure

# Run E2E journey tests only
ctest --preset linux-release -L "w5a" --output-on-failure

# Run failure/recovery tests only
ctest --preset linux-release -L "w5b" --output-on-failure
```

---

## 3. Test Inventory

### 3.1 W5-A — Production-Critical E2E Journeys (`w5a_e2e_critical_journeys_test.cpp`)

| Test ID | Test Name | Journey Covered | Assertion Focus |
|---------|-----------|----------------|-----------------|
| E2E-01 | `E2E01_IngestIndexQueryAuditHappyPath` | Full ingest → index → query → audit | Storage, index hits, audit trail |
| E2E-02 | `E2E02_AuthDeniedTokenRejectsQuery` | Auth denied vs. auth allowed | 401 on deny; data returned on allow |
| E2E-03 | `E2E03_RagPipelineEmbedRetrieveInferScore` | RAG: embed → retrieve → infer → score | Answer non-empty, score > 0 |
| E2E-04 | `E2E04_TransactionCommitMultiShardCdc` | Multi-shard TX commit + CDC | All keys stored; CDC count == writes |
| E2E-05 | `E2E05_BatchIngestCheckpointResumeIdempotent` | Batch checkpoint + resume | Idempotent: storage.Size() == 6 |
| E2E-06 | `E2E06_SecurityDeniedTokenLeavesNoArtifact` | Security: no data leak on deny | Storage unchanged after denied query |
| E2E-07 | `E2E07_ConcurrentWritesConvergeConsistentState` | 8-thread concurrent ingest | No crash; storage.Size() == 16 |
| E2E-08 | `E2E08_SchemaInvalidDocumentRejectedAtBoundary` | Schema-invalid doc rejected | Not in storage; audit schema_error |

### 3.2 W5-B — Failure Injection & Recovery (`w5b_failure_injection_recovery_test.cpp`)

| Test ID | Test Name | Failure Mode | Recovery Expectation |
|---------|-----------|-------------|----------------------|
| FIR-01 | `FIR01_EmbeddingFailureRagReturnsError` | LLM embedding backend fails | RAG returns `embedding_failed`; no storage write |
| FIR-02 | `FIR02_InferenceFailureNoPartialAnswer` | LLM inference fails (embed ok) | RAG returns `inference_failed`; embed called once |
| FIR-03 | `FIR03_EmptyTransactionRejectedCleanly` | Zero-write commit | Rejected with `empty_tx`; storage unchanged |
| FIR-04 | `FIR04_MissingIdRejectedBeforeStorageWrite` | Document missing "id" | Rejected before write; storage unchanged |
| FIR-05 | `FIR05_IndexMissReturnsEmptyHitsNoError` | Term never indexed | Empty hits, no error (query succeeds) |
| FIR-06 | `FIR06_AuthRecoveryDeniedThenReauthorised` | Token denied then granted | Re-auth succeeds without restart |
| FIR-07 | `FIR07_PartialBatchFailureValidDocsSucceed` | Mixed valid/invalid batch | Valid docs ingested; invalid rejected individually |
| FIR-08 | `FIR08_RagNoHitsRecoveryReportsCleanError` | RAG: embed ok but no index hits | `no_documents` error; inference NOT called |

---

## 4. Signal Quality — Anti-Flake Rules

Wave 5 tests are written to the following determinism standards:

1. **No wall-clock sleeps** — Use `WaitForCondition()` with timeout, never
   `std::this_thread::sleep_for()` as a synchronisation primitive.
2. **No external service dependencies** — All LLM, auth, storage, and index
   components are injected mocks.
3. **Deterministic mock state** — All failure modes are injected via explicit
   calls (`SetEmbeddingFailure(true)`) before the act under test.
4. **Independent tests** — Each test uses freshly constructed pipeline
   objects in `SetUp()`; no shared state between test cases.
5. **Unique document IDs** — IDs include the test prefix (`e2e01_`, `fir03_`)
   to prevent accidental cross-test collisions.

---

## 5. Triage Runbook

### 5.1 A Wave 5 gate fails in CI

1. Identify the failing test name in the CTest output.
2. Reproduce locally:
   ```bash
   ctest --preset linux-release -R "<test_name>" --output-on-failure -V
   ```
3. For mock failures check the `SetEmbeddingFailure / SetInferenceFailure`
   injection call in the test body — these are the only failure injection
   points.
4. For concurrency failures (E2E-07) run under TSan:
   ```bash
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ...
   ctest -R w5a_e2e_critical_journeys_test
   ```
5. Escalate if the failure is non-deterministic across three consecutive runs.

### 5.2 Adding a new Wave 5 test

1. Place the file in `tests/integration/pipeline/w5{a|b|c}_*.cpp`.
2. Add an `add_integration_test()` block and the labels  
   `"integration;pipeline_integration;wave5;w5{a|b};release_critical"` in
   `tests/integration/CMakeLists.txt`.
3. Assign the next free test ID (`E2E-09`, `FIR-09`, …).
4. Update `WAVE5_TEST_COVERAGE.md` with the new entry.

---

## 6. Known Gaps & Prioritised Follow-ups

| Gap | Priority | Notes |
|-----|----------|-------|
| Timeout simulation (artificial slow backend) | Medium | Requires a `SetLatencyMs()` mock control |
| Resource-exhaustion tests (OOM, storage full) | Medium | Needs platform-level mock or bounded allocator |
| End-to-end over real RocksDB (not in-memory) | High | Gated on `THEMIS_ENABLE_ROCKSDB_INTEGRATION_TESTS` flag |
| Multi-node replication failure (split-brain) | High | Requires Raft mock to inject partition events |
| Retry/backoff verification (exponential policy) | Low | Covered partially by FIR-06; backoff curve not asserted |

---

## 7. Ownership

| Area | Owner |
|------|-------|
| Wave 5 tests | QA / Platform Engineering |
| `test_fixture.h` mock infrastructure | Core Platform |
| CMakeLists.txt gate registration | Build / CI |
| This document | QA Lead |
