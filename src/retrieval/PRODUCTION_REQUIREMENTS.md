> **Status:** 2026-07-18 – with current Retrieval code (LoRA artifacts Phase 3 complete, Hybrid rollout Phase A ready).

# ThemisDB Retrieval Module - Production Requirements

## Purpose and Scope

This document is the **canonical reference for production minimum requirements** of the Retrieval module.
It defines mandatory operational and security requirements for LoRA artifact management, hybrid retrieval stack,
and distributed tensor coordination.

## Document Boundaries (Canonical Split)

- **`src/retrieval/PRODUCTION_REQUIREMENTS.md` (this document):** mandatory production requirements (MUST/MUST NOT), security assumptions, operational boundaries.
- **`src/retrieval/README.md`:** functional overview, architecture context, API and usage examples.
- **`src/retrieval/ROADMAP.md`:** delivery phases, open/closed features, readiness planning.
- **`src/retrieval/FUTURE_ENHANCEMENTS.md`:** mid-term and long-term enhancements and research fields.

## Mandatory Production Requirements

### LoRA Artifacts (Phase 1-7 Complete)
- **MUST:** LoRAManifestStore initialized with persistent storage backend before production use.
- **MUST:** Artifact integrity verification enabled; failed verification blocks deployment.
- **MUST:** Signature verification callback must be wired before processing untrusted artifacts.
- **MUST:** LoRA package lifecycle states (Draft → Approved → Deployed → Retired) enforced by deployment policy.
- **MUST NOT:** Bypass integrity checks or signature verification in production paths.

### Hybrid Retrieval Rollout (Phase A Approved)
- **MUST:** Phase A (exact-first) graph truth layer must be CPU-first and exact; no advisory-only results returned as ground truth.
- **MUST:** Single-shard exact retrieval routing tested and verified end-to-end before enablement.
- **MUST:** All graph traversal error injection test cases passing before Phase A production deployment.
- **MUST:** Phase B and Phase C gates must pass validation benchmarks before enabling advisory acceleration or multi-shard coordination.
- **MUST NOT:** Enable GPU or distributed-shard paths until Phase C/D readiness gates are met.

## Mandatory Security Requirements

### Artifact Provenance and Integrity
- Artifact metadata (name, version, hash, signature) must be immutable once stored.
- Tampering detection must propagate as explicit errors; no silent permission grants.
- Signature verification failures must block deployment; fallback to unsigned artifacts is not permitted.

### Access Control
- LoRA package lifecycle transitions must be gated by deployment policy.
- Model switch workflows must validate source package authorization before applying adapter.

### Audit Logging
- Artifact store operations (import, export, verify) must log change events in production deployments.
- Policy violations and signature failures must be logged with full context.

## Operational Boundaries

### Configuration
- Manifest store backend connection string and credentials must be configured externally; no hardcoded defaults.
- Signature verification callback must be provided; missing callback blocks artifact verification.
- Retrieval strategy (exact vs advisory) must be set explicitly per deployment tier.

### Resource Limits
- LoRA manifest in-memory cache must be bounded to prevent unbounded growth.
- Artifact batch import operations must have configurable batch size and timeout limits.
- Graph traversal depth and result cardinality must respect deployment-level limits.

### External Dependencies
- Backend storage (RocksDB, filesystem, etc.) must be accessible and configured with explicit timeouts.
- If distributed retrieval is enabled, shard coordinator must be reachable with configured retry policies.

## Minimal Production Verification Checklist (Audit-capable)

- [ ] LoRAManifestStore initialized with persistent backend
- [ ] Artifact integrity verification enabled and tested
- [ ] Signature verification callback wired and tested
- [ ] Phase A exact-first retrieval gates passing all tests
- [ ] All graph traversal error paths tested with injection
- [ ] Configuration externalized (no hardcoded defaults)
- [ ] Resource limits explicitly configured (no unlimited defaults)
- [ ] Audit logging active in production deployments
- [ ] Fallback behavior verified (CPU-first, exact path preserved)
- [ ] Production mode set via `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT`

## Production Phases and Approval Gates

### Phase A (Approved ✅)
- **Entitlement:** Single-shard exact retrieval with CPU-only processing.
- **Verification:** `test_ann_frontdoor_single_shard` passing; exact graph traversal validated.
- **Deployment:** LoRA artifacts ready for production import/export.

### Phase B (Pending ⏳)
- **Entitlement:** ANN frontdoor with CPU parity validation layer (advisory-only acceleration).
- **Verification:** `test_ann_cpu_parity` and benchmarks passing in target environment.
- **Deployment:** Not approved until environment validation complete.

### Phase C (Not Started)
- **Entitlement:** Multi-shard exact coordination with tensor summaries.
- **Verification:** `test_sharding_multishard_exact` and benchmarks passing.
- **Deployment:** Not approved until Phase C gates met.

### Phase D (Future)
- **Entitlement:** GPU break-even evaluation and optional GPU acceleration.
- **Verification:** Benchmark results reviewed and published.
- **Deployment:** Not planned for current release.

## Review / Sourcecode Audit Evidence

### Affected Files in Review
- `src/retrieval/PRODUCTION_REQUIREMENTS.md` (this document)
- `src/retrieval/include/lora_package.h`
- `src/retrieval/src/lora_package.cc`
- `tests/epic1_retrieval/lora_package_test.cc` (55 test cases)
- `benchmarks/epic1_retrieval/lora_loading_bench.cc` (12 benchmark scenarios)

### Evidence Summary
- LoRA artifact implementation: 100% Doxygen API coverage, full error handling, thread-safe manifest store
- Phase A gate test: `test_ann_frontdoor_single_shard` verifies end-to-end single-shard exact retrieval
- Phase A benchmark: CPU-first performance baseline established
- Integration: CMake targets enabled, library compiles independently

---

**Document Validation:** This requirements document is kept in sync with the actual module implementation
and the ROADMAP.md roadmap phases. Any production deployment must verify all items on the checklist above.
