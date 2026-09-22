# LLM Wiki Module - Production Requirements

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md -->

## Purpose and Scope

This document defines **mandatory production requirements** for the LLM Wiki module. It specifies operational constraints, integration boundaries, performance gates, and deployment prerequisites.

## Document Boundaries (Canonical Split)

- **`src/llm_wiki/PRODUCTION_REQUIREMENTS.md` (this document):** Mandatory production constraints (MUST/MUST NOT), operational limits, deployment gates, current evidence.
- **`src/llm_wiki/README.md`:** Module overview, scope, interfaces, quickstart.
- **`src/llm_wiki/ROADMAP.md`:** Implementation phases 1-6, completion status, production readiness.
- **`src/llm_wiki/FUTURE_ENHANCEMENTS.md`:** Planned features, design constraints, research directions.
- **`src/llm_wiki/ARCHITECTURE.md`:** Design principles, components, data flow, plugin architecture.
- **`src/llm_wiki/CHANGELOG.md`:** Version history and delivered artefacts.

## Mandatory LLM Wiki Production Requirements

### 1. Plugin Architecture & Edition Gating

**MUST:** LLMWikiPlugin must only be instantiable in Enterprise edition or higher.
- Evidence: `include/llm_wiki/llm_wiki_plugin_interface.h` — factory checks edition
- Status: ✅ ENFORCED
- Error: Plugin instantiation fails with explicit error in Community edition

**MUST:** Capability matrix must be enforced at operation level.
- Evidence: Edition gate checks in all public operations (query, ingest)
- Status: ✅ ENFORCED
- Error code: E6500 (edition access denied) on denied operations

**MUST NOT:** Allow silent fallback to reduced functionality in non-supported editions.
- Status: ✅ ENFORCED
- Behavior: Explicit error return; no degradation to Community-equivalent features

### 2. Workspace Isolation & State Integrity

**MUST:** Workspace data must be strictly isolated by workspace ID.
- Evidence: `src/llm_wiki/workspace_state_manager.h` — workspace-scoped containers
- Status: ✅ ENFORCED
- Implication: No cross-workspace data leakage; parameterized access by workspace ID

**MUST:** Workspace state must be validated via checksums on load.
- Evidence: Checksum computation on write, validation on read
- Status: ✅ ENFORCED
- Error code: E6503 (workspace corrupted) if checksum fails

**MUST:** Atomic write-replace semantics for workspace modifications.
- Evidence: Log-based persistence with atomic rename
- Status: ✅ ENFORCED
- Implication: Partial failures do not corrupt workspace state

**MUST:** Recovery from corruption via log-based replay.
- Evidence: Recovery logic in workspace state manager
- Status: ✅ IMPLEMENTED
- Guarantee: Restores last-known-good state or returns recovery error

### 3. Guardrail Pattern Detection

**MUST:** All user-supplied queries must be validated against guardrail patterns before processing.
- Evidence: `src/llm_wiki/guardrail_patterns.h` — 60+ pattern definitions
- Status: ✅ ENFORCED
- Error code: E6502 (guardrail pattern matched) on detection

**MUST NOT:** Allow any guardrail-matched query to proceed to downstream processing.
- Status: ✅ ENFORCED
- Behavior: Reject with explicit error; log matched pattern for audit

**MUST:** Guardrail patterns must cover 5 attack categories.
- Categories: Shell commands, code/syntax, encoding, privilege escalation, control-flow
- Status: ✅ VERIFIED (LWP-05, LWP-17 through LWP-20)
- Evidence: Test coverage for all 5 categories

### 4. Performance Gates

**MUST:** Query latency (P95, single topic) must remain ≤ 200 ms.
- Evidence: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — LW-BM-01
- Status: 🔄 VALIDATING (Phase 5 target)
- Hardware: P95 on release-profile hardware

**MUST:** Ingest throughput must remain ≥ 100 pages/sec (P95).
- Evidence: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — LW-BM-02
- Status: 🔄 VALIDATING (Phase 5 target)
- Condition: Sustained ingestion, no workspace corruption

**MUST:** Workspace isolation overhead must remain ≤ 10%.
- Evidence: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — LW-BM-03
- Status: 🔄 VALIDATING (Phase 5 target)
- Implication: Multi-workspace deployments remain performant

**MUST:** Guardrail pattern matching overhead ≤ 5 ms per query.
- Evidence: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — LW-BM-04
- Status: 🔄 VALIDATING (Phase 5 target)
- Regression tolerance: ≤ 10% vs. baseline

### 5. Concurrency & Thread Safety

**MUST:** Multiple concurrent queries must not corrupt workspace state.
- Evidence: `tests/integration/test_llm_wiki_soak.cpp` — concurrent stability test
- Status: ✅ VERIFIED
- Guarantee: Read-write mutex protection on workspace state

**MUST NOT:** Concurrent ingest and query must serialize; no parallel ingests.
- Status: ✅ ENFORCED
- Behavior: Ingest operations acquire exclusive write lock; queries use shared read lock

**MUST:** Concurrent queries must not introduce deadlocks.
- Status: ✅ VERIFIED (stress testing)
- Locking Strategy: Consistent lock acquisition order; no nested write locks

### 6. Policy Management & Configuration

**MUST:** YAML process policies must be validated against JSON schema on load.
- Evidence: `src/llm_wiki/process/llm_wiki_process_policy.schema.json` — schema definition
- Status: ✅ ENFORCED
- Error: Policy load fails with explicit schema error on violation

**MUST NOT:** Accept or use invalid policies; must fail closed.
- Status: ✅ ENFORCED
- Behavior: Invalid policy rejected; last-known-good retained

**MUST:** Hot-reload must preserve last-known-good policy on reload failure.
- Evidence: Rollback logic in process_policy_manager
- Status: ✅ IMPLEMENTED
- Guarantee: Policy never becomes inconsistent; rollback is automatic

### 7. Error Handling & Graceful Degradation

**MUST:** All error conditions return explicit error codes (E6500–E6599).
- Error Taxonomy:
  - E6500: Edition access denied
  - E6501: Workspace not found
  - E6502: Guardrail pattern matched
  - E6503: Workspace state corrupted
  - E6504: Policy validation failed
  - E6505: Provenance chain depth exceeded
  - E6506: Confidence below threshold
  - E6507: Evidence origin not allowlisted
- Status: ✅ DOCUMENTED

**MUST NOT:** Silently ignore errors or proceed with corrupted state.
- Status: ✅ ENFORCED
- Implication: Errors logged; callers notified; workspace preserved on failure

**MUST:** Out-of-memory conditions must not corrupt workspace.
- Status: ✅ ENFORCED via bounds checking
- Fallback: Reject new ingests; preserve existing workspace

### 8. Operational Bounds

**Workspace Size Limits:**
- Maximum pages per workspace: Limited by available system memory
- Recommended limit: ≤ 1M pages per workspace
- Maximum provenance chain depth: 10 levels (enforced; error E6505)
- Minimum provenance confidence: 0.7 (configurable; default threshold)

**Query Operation Limits:**
- Maximum query concurrency: ≥ 10 concurrent queries with < 10% latency overhead
- Maximum evidence package size: 100 MB (validated at synthesis time)
- Query timeout: 30 seconds default (configurable per operation)

**Ingest Operation Limits:**
- Maximum pages per ingest batch: 1 000 pages (recommended)
- Maximum page size: 10 MB per page
- Ingest timeout: 120 seconds per batch (configurable)

**Policy Management Limits:**
- Maximum policy file size: 10 MB
- Maximum number of control parameters: 100
- Policy hot-reload timeout: 5 seconds

### 9. Integration Boundaries

**LLM Module Integration:**
- LLM provides synthesized queries to wiki
- Wiki returns evidence + confidence metadata
- **Boundary:** Wiki does not access or cache LLM-internal state; LLM controls synthesis.

**Retrieval Module Integration:**
- Retrieval provides ranked evidence to wiki
- Wiki enforces evidence allowlist and chain-depth validation
- **Boundary:** Retrieval is read-only consumer; wiki owns evidence policy.

**Server Integration:**
- HTTP endpoints invoke wiki with validated edition and parameters
- Query parameters (k, metric) validated before wiki access
- **Boundary:** Server sanitizes input; wiki enforces edition gates.

### 10. Deployment Prerequisites

**Required Dependencies:**
- C++ compiler with C++17 support (gcc 7+, clang 5+, MSVC 2017+)
- LLM module (orchestration and synthesis)
- Retrieval module (optional; for evidence ranking)
- RocksDB (optional; for persistent workspace cache — Phase B+)

**Hardware Recommendations:**
- Minimum RAM: 2 GB (for small workspaces)
- Recommended RAM: ≥ 32 GB (for production deployments)
- CPU: Multi-core (for concurrent query scaling)
- Storage: SSD recommended for workspace persistence

**Configuration Requirements:**
- Edition must be set correctly (Community excluded; Enterprise+ required)
- Process policy YAML must be valid and present
- Workspace directory must be writable and have sufficient space
- Process policy must be validated before deployment

### 11. Monitoring & Observability

**MUST Track:**
- Query/ingest operation latencies (P50, P95, P99)
- Workspace state size and memory consumption
- Guardrail pattern match rates and categories
- Edition gate denial rates
- Concurrent query counts
- Error rates and error codes (E6500–E6599)

**MUST Alert On:**
- Error rate surge (> 5% of operations)
- Latency regression (> 15% vs. baseline)
- Memory pressure (> 80% of limit)
- Workspace corruption detection (E6503)
- Guardrail pattern match surge (potential attack)
- Edition gate denials (potential misconfiguration)

**Optional But Recommended:**
- Workspace fragmentation metrics
- Provenance chain length distribution
- Evidence origin diversity
- Policy hot-reload frequency and success rate

## Wave Model Integration

LLM Wiki module is a **contributing module** in Wave A → B → C → D execution model.

**Wave B Status:** 🔄 IN PROGRESS (2026-09-22)
- Cost-signal expansion and provenance propagation in progress
- Security/governance runtime gates and audit artefacts in progress
- Policy orchestration hot-reload safety and rollback tests pending
- Target: Q4 2026 / Q1 2027 completion

See [ROADMAP.md](ROADMAP.md) and [`../../ROADMAP.md`](../../ROADMAP.md) for full wave model details.

## Deployment Checklist

- [ ] Edition configured to Enterprise or higher
- [ ] Workspace directory created and permissions verified
- [ ] Process policy YAML present and schema-validated
- [ ] Guardrail patterns updated to current threat model
- [ ] Performance targets validated on production hardware
- [ ] Monitoring and alerting configured
- [ ] Concurrent load testing completed
- [ ] Graceful degradation tested (out-of-memory, invalid queries)
- [ ] Workspace recovery procedures tested
- [ ] Audit logging enabled and verified
- [ ] Policy hot-reload tested with valid and invalid policies
- [ ] Edition gate enforcement verified (Community fails, Enterprise+ succeeds)

---

**Status:** Production-candidate for deployment after Phase 5 hardening completion, checklist verification, and maintainer sign-off.
