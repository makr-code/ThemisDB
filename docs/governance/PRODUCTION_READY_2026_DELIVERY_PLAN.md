# Production-Ready 2026 Delivery Plan

<!-- Status: [~] in progress — updated 2026-07-29 -->
<!-- Governance: This document is the canonical delivery track for ThemisDB v2.x production readiness by end-2026. -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · CHANGELOG.md · RELEASE_STRATEGY.md · VERSIONING.md -->

## Purpose

This document is the central delivery track for **ThemisDB production readiness by end of 2026**.
It covers five workstreams (A–E) across three delivery waves and enforces hard Go/No-Go criteria.

All API contracts referenced here are treated as **frozen and signed** for the v2.x major line.
Breaking changes require a major-version bump and must be reflected in CHANGELOG.md.

---

## Go/No-Go Criteria (End-2026 Gate)

A release promotion to GA is blocked unless **all** of the following are satisfied:

| Gate | Criterion | Evidence |
|------|-----------|---------|
| G-01 | Contract-Doku vollständig (0 open contract-freeze checkboxes) | All module `*_api_contract.h` headers + ROADMAP Phase 1 `[x]` |
| G-02 | Deep-Dive-Tests grün (release_critical label stable) | Wave 5/6/8 + module contract-hardening tests |
| G-03 | Benchmark-Gates erfüllt (GATE-* p99 thresholds met) | Wave 7 manifest + per-module GATE tables |
| G-04 | release_critical CI stable (no flakes ≥ 3 runs) | `.github/workflows/09-pr-gates_release-critical-tests.yml` |
| G-05 | GA Sign-Off Batch D human-approved | `docs/governance/GA_PROMOTION_SIGN_OFF.md §9` |
| G-06 | 0 open CRITICAL/HIGH security findings | `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md` |

---

## Workstream A — Contract Documentation

**Goal:** All public interfaces have frozen, source-verifiable API contracts.

**Target:** Q3–Q4 2026

### Wave 1 — Top-Risk Modules (COMPLETE ✅)

| Module | Contract Header | Error Codes | Phase 1 ROADMAP |
|--------|----------------|-------------|-----------------|
| server | `include/server/server_api_contract.h` | 13 codes | `[x]` |
| llm | `include/llm/llm_api_contract.h` | 14 codes | `[x]` |
| sharding | `include/sharding/sharding_api_contract.h` | 13 codes | `[x]` |

### Wave 2 — Security-Critical Modules (COMPLETE ✅)

| Module | Contract Header | Error Codes | Phase 1 ROADMAP |
|--------|----------------|-------------|-----------------|
| security | `include/security/security_api_contract.h` | 20+ codes | `[x]` |
| network | `include/network/network_api_contract.h` | 20+ codes | `[x]` |
| storage | `include/storage/storage_api_contract.h` | 15+ codes | `[x]` |
| geo | `include/geo/geo_api_contract.h` | 15+ codes | `[x]` |

### Wave 3A — Data Pipeline Modules (COMPLETE ✅)

| Module | Contract Header | Error Codes | Phase 1 ROADMAP |
|--------|----------------|-------------|-----------------|
| analytics | `include/analytics/analytics_api_contract.h` | 8+ codes | `[x]` |
| replication | `include/replication/replication_api_contract.h` | 8+ codes | `[x]` |
| temporal | `include/temporal/temporal_api_contract.h` | 8+ codes | `[x]` |
| timeseries | `include/timeseries/timeseries_api_contract.h` | 8+ codes | `[x]` |
| tensor | `include/tensor/tensor_api_contract.h` | 8+ codes | `[x]` |

### Wave 3B — Operations Modules (COMPLETE ✅)

| Module | Contract Header | Error Codes | Phase 1 ROADMAP |
|--------|----------------|-------------|-----------------|
| failover | `include/failover/failover_api_contract.h` | 9 codes | `[x]` |
| observability | `include/observability/observability_api_contract.h` | 9 codes | `[x]` |
| distributed_knowledge | `include/distributed_knowledge/distributed_knowledge_api_contract.h` | 8 codes | `[x]` |
| exporters | `include/exporters/exporters_api_contract.h` | 9 codes | `[x]` |
| importers | `include/importers/importers_api_contract.h` | 10 codes | `[x]` |
| ingestion | `include/ingestion/ingestion_api_contract.h` | 9 codes | `[x]` |

### Wave 3C — Infrastructure Modules (COMPLETE ✅)

| Module | Contract Header | Status |
|--------|----------------|--------|
| maintenance | `include/maintenance/maintenance_api_contract.h` | ✅ MTN 8100-8107 |
| plugins | `include/plugins/plugins_api_contract.h` | ✅ PLG 8200-8207 |
| rpc_grpc | `include/rpc_grpc/rpc_grpc_api_contract.h` | ✅ RPC 8300-8307 |
| scheduler | `include/scheduler/scheduler_api_contract.h` | ✅ SCH 8400-8407 |
| scraper | `include/scraper/scraper_api_contract.h` | ✅ SCR 8500-8507 |
| user_storage_encrypted | `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` | ✅ USE 8600-8607 |

### Wave 3D — Utility & Engine Modules (COMPLETE ✅)

| Module | Contract Header | Error Codes | Phase 1 ROADMAP |
|--------|----------------|-------------|-----------------|
| performance | `include/performance/performance_api_contract.h` | 7 codes | `[x]` |
| governance | `include/governance/governance_api_contract.h` | 6 codes | `[x]` |
| utils | `include/utils/utils_api_contract.h` | 6 codes | `[x]` |
| updates | `include/updates/updates_api_contract.h` | 6 codes | `[x]` |
| toolbox | `include/toolbox/toolbox_api_contract.h` | 4 codes | `[x]` |
| process | `include/process/process_api_contract.h` | 5 codes | `[x]` |
| projects | `include/projects/projects_api_contract.h` | 4 codes | `[x]` |
| themis | `include/themis/themis_api_contract.h` | 2 codes | `[x]` |
| metadata | `include/metadata/metadata_api_contract.h` | 4 codes | `[x]` |
| content | `include/content/content_api_contract.h` | 4 codes | `[x]` |

**Reference complete module:** `auth` — `include/auth/auth_principal_contract.h`

---

## Workstream B — Deep-Dive Tests

**Goal:** Every module has deterministic, edge-case-focused, fault-injection-capable tests registered under `release_critical` or module-focused CTest label.

**Pattern:** Each module gets `test_<module>_contract_hardening_focused.cpp` with 8–20 GTest cases, `kSeed=42`, no file I/O.

**Status:** 28+ contract-hardening test files delivered across all completed waves.

**Remaining:** Wave 3C modules (6) — in progress.

---

## Workstream C — Benchmark Gates

**Goal:** Every production-critical code surface has p95/p99/throughput gates in `benchmarks/<module>/bench_<module>_release_gates.cpp`.

**Pattern:** 4–8 `BENCHMARK()` functions, `kCanonicalSeed=42`, `Repetitions(5)->ReportAggregatesOnly(true)`, hard `GATE-*` table in file header.

**Status:** 33+ release-gate benchmark files delivered. All wired via `benchmarks/CMakeLists.txt` with `if(EXISTS ...)` subdirectory guards.

**Remaining:** Wave 3C modules (6) — in progress.

**Wave 7 Integration Gates (pre-existing):**
- `benchmarks/wave7/release_gate_manifest_w7.json` — GATE-W7-01..06
- Read p99 ≤ 200µs, Write ≥ 80k ops/s, Range p99 ≤ 500µs, Batch p99 ≤ 5ms

---

## Workstream D — Future Features (Q4 2026 / Q1 2027)

**Goal:** All items marked "required for production readiness" in `FUTURE_ENHANCEMENTS.md` completed or formally de-scoped.

**Prioritisation:**
1. Reliability + Diagnostics + Skalierung (production blockers) — Q4 2026
2. Optional/experimental enhancements — Q1 2027 or de-scoped if GA-end-2026 is at risk

**Governance:** De-scope decisions require human sign-off in `docs/governance/GA_PROMOTION_SIGN_OFF.md`.

---

## Workstream E — Category-D Template Closure

**Template per module (Phase 1–6):**

| Phase | Deliverable | Acceptance Criterion |
|-------|-------------|---------------------|
| 1 | `include/<mod>/<mod>_api_contract.h` | ≥4 error codes, §Purpose, §Contracts, §Error Taxonomy, §Threading |
| 2 | Core implementation hardened | No TODO/FIXME in hot paths |
| 3 | Error handling unified | All error paths return typed error code, no silent failures |
| 4 | `tests/<mod>/test_<mod>_contract_hardening_focused.cpp` | ≥8 GTest cases, kSeed=42, no I/O |
| 5 | `benchmarks/<mod>/bench_<mod>_release_gates.cpp` | ≥4 GATE-* entries, Repetitions(5) |
| 6 | ROADMAP Phase 1/4/5/6 `[x]`, docs updated | 0 open Phase 1–6 checkboxes |

**Validation per module: Source-Existence + Testtiefe + Benchmark-Gates**

---

## Module Completion Matrix

| Module | P1 Contract | P4 Tests | P5 Bench | P6 Docs | Status |
|--------|------------|---------|---------|---------|--------|
| auth | ✅ | ✅ | ✅ | ✅ | ✅ Complete |
| ethics_ai | ✅ | ✅ | ✅ | ✅ | ✅ Complete |
| server | ✅ | ✅ | ✅ | ✅ | ✅ Wave 1 |
| llm | ✅ | ✅ | ✅ | ✅ | ✅ Wave 1 |
| sharding | ✅ | ✅ | ✅ | ✅ | ✅ Wave 1 |
| security | ✅ | ✅ | ✅ | ✅ | ✅ Wave 2 |
| network | ✅ | ✅ | ✅ | ✅ | ✅ Wave 2 |
| storage | ✅ | ✅ | ✅ | ✅ | ✅ Wave 2 |
| geo | ✅ | ✅ | ✅ | ✅ | ✅ Wave 2 |
| analytics | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3A |
| replication | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3A |
| temporal | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3A |
| timeseries | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3A |
| tensor | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3A |
| failover | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| observability | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| distributed_knowledge | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| exporters | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| importers | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| ingestion | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3B |
| performance | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| governance | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| utils | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| updates | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| toolbox | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| process | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| projects | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| themis | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| metadata | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| content | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3D |
| maintenance | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| plugins | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| rpc_grpc | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| scheduler | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| scraper | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| user_storage_encrypted | ✅ | ✅ | ✅ | ✅ | ✅ Wave 3C |
| aql | ⬜ | ⬜ | ⬜ | ⬜ | Roadmap |
| base | ✅ | ✅ | ✅ | ✅ | Pre-existing |
| core | ✅ | ✅ | ✅ | ✅ | Pre-existing |
| llama_cpp | ✅ | ✅ | ✅ | ✅ | Pre-existing |

Legend: ✅ Done · 🔄 In Progress · ⬜ Planned · ❌ Blocked

---

## Release Timeline

| Milestone | Target | Gate Criteria |
|-----------|--------|--------------|
| Wave 1–2 complete | 2026-07-29 | Contract headers + tests + benchmarks for server/llm/sharding/security/network/storage/geo |
| Wave 3A–3D complete | 2026-07-29 | Same for 21 additional Category-D modules |
| Wave 3C complete | 2026-08-Q3 | maintenance/plugins/rpc_grpc/scheduler/scraper/user_storage_encrypted |
| Phase 2/3 hardening | Q4 2026 | Core implementation + error handling closure for all modules |
| Future Features D | Q4 2026 | Production-required items from FUTURE_ENHANCEMENTS.md |
| GA Gate Check | 2026-12-31 | All G-01..G-06 gates green, Batch D human sign-off |

---

## Synchronisation Requirement

After each wave completion, the following root files MUST be updated:

- `ROADMAP.md` — wave closure reflected in Phase 0–6 status
- `FUTURE_ENHANCEMENTS.md` — completed items archived
- `CHANGELOG.md` — wave entry added under [Unreleased]
- `RELEASE_STRATEGY.md` — release-gate evidence links updated
- `VERSIONING.md` — pre-release version bump if applicable

---

*Last updated: 2026-07-29 | Maintained by: ThemisDB Delivery Governance*
