> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: current | generated: 2026-09-30 | source: recursive src/*/ROADMAP.md + module deep-dive + sprint backlog -->
<!-- Use this file as the master backlog for GitHub Issue creation. -->
<!-- Each item row maps directly to one GitHub Issue. -->

# ThemisDB — Consolidated Source Roadmap

> **Purpose:** This document aggregates the current hardening backlog and planned feature work
> from module roadmaps in `src/*/ROADMAP.md` and a direct source validation pass.
> It combines a source-validated posture with the sprint-sized work items that can be handed to
> a coding agent for implementation.

## Module Status Snapshot

| Status | Module groups | Notes |
|---|---|---|
| Production-ready / mostly closed | `server`, `storage`, `network`, `auth`, `security`, `cache`, `analytics`, `failover`, `maintenance`, `updates`, `process`, `execution` | These modules already show significant closure evidence and are not the main backlog driver. |
| Active hardening | `themis`, `transaction`, `query`, `index`, `sharding`, `replication`, `graph`, `cdc`, `llm`, `rag`, `gpu`, `acceleration`, `geo`, `voice`, `access_model`, `ethics_ai`, `search`, `training` | Main source of roadmap execution risk; all still carry open performance/hardening gates. |
| Docs-only / source mismatch | `llm_streaming`, `vector_search` | Local module directories currently contain docs-only scaffolding; roadmap claims still describe production-like implementation state. |
| Planned / externalization | `chimera`, plugin externalization tracks | Planning exists, but final implementation boundary still needs a real source path and owner. |

---

## Recursive Roadmap Deep-Dive Snapshot (2026-09-30)

- **Scope checked:** recursive `src/*/ROADMAP.md` review plus source-path validation for the highest-risk modules.
- **Current operating posture:** core platform modules are substantially implemented, but several frontier modules still carry open performance gates, benchmark evidence gaps, and source-traceability mismatches.
- **Highest remaining backlog pressure (open checkbox count, from latest source roadmap scan):**
  - `query` (59), `acceleration` (42), `llm_wiki` (40), `llm` (40), `transaction` (32), `index` (28), `updates` (27), `search` (27), `rag` (24).
- **Source-code deep-dive findings requiring follow-up:**
  - `src/llm_streaming/` remains docs-only (`README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `.gitkeep`), while its roadmap still describes production implementation.
  - `src/vector_search/` remains docs-only (`README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `.gitkeep`), while its roadmap still describes production implementation.
  - The canonical continuation is either to bind the roadmap to real implementation/test/benchmark paths or to downgrade the local roadmap claims to planned/externalized state until sources exist.

### Immediate continuation track (next updates)
- [ ] Resolve source-traceability for `llm_streaming` and `vector_search` by binding roadmap claims to concrete `src/` + `tests/` + `benchmarks/` paths or by reclassifying to planned/externalized state (Target: Q4 2026).
- [ ] Execute release-critical backlog closure in `acceleration`, `query`, `transaction`, `index`, `gpu`, and `rag` with representative-hardware evidence refresh (Target: Q4 2026).
- [ ] Re-run recursive roadmap deep-dive after each Wave A/B closure batch and sync this consolidated source roadmap (Target: ongoing).

### Wave Closure Implementation Sequence (2026-09-30)
- [ ] Wave A completion: finalize Transaction + GPU authoritative representative-hardware and chaos/recovery evidence, then close remaining `release_critical` + p95/p99 gates for Wave-A modules (Target: Q4 2026).
- [ ] Wave B finish: close remaining hardening blocks in `query`, `acceleration`, `llm_wiki`, `llm`, `index`, `rag`, `search`, `updates`, including Query↔Index↔Storage and LLM↔RAG↔LLM_Wiki integration evidence (Target: Q4 2026).
- [ ] Wave C validation: consolidate sustained-load security/compliance evidence for `auth`/`security`/`governance` and fail-closed boundary behavior into final sign-off package (Target: Q4 2026).
- [ ] Wave D operability: close runbook/diagnostics/alerts/recovery and operator-readiness gates with refreshed chaos/failover/recovery evidence (Target: Q1 2027).
- [ ] Enforce final order: Wave-A residual gates → Wave-B hardening → Wave-C security sign-off → Wave-D operability sign-off → final governance sync + human GA sign-off (Target: Q1 2027).

---

## Sprint-Ready Implementation Backlog for Coding Agent

The following work items are explicit, bounded, and mapped to source-backed roadmap gaps. Each item is intended to become a GitHub issue and be implementable by a coding agent in a single sprint.

### Sprint 1 — Source Traceability Cleanup
- [ ] `llm_streaming` + `vector_search` source-path repair: bind roadmap claims to real source/test/benchmark locations or downgrade to planned/externalized status. (Target: Q4 2026)
- [ ] Implement a roadmap-source validation check in repo automation that fails if a module README/ROADMAP claims production delivery without matching implementation/test paths. (Target: Q4 2026)

### Sprint 2 — Acceleration hardening closure
- [ ] Close the outstanding GPU/CPU parity and benchmark evidence gaps for acceleration and GPU paths; ensure `benchmarks/acceleration` and `benchmarks/index` are tied to real execution evidence. (Target: Q4 2026)
- [ ] Finalize fail-closed validation for the remaining Category B acceleration edges (geo/BFS/Dijkstra parity + bounded fallback paths) and ensure the roadmap reflects the actual execution status. (Target: Q4 2026)

### Sprint 3 — Query hardening and Phase C closure
- [ ] Close the remaining query planner/optimizer hardening items: planner reliability, federation timeout/retry determinism, and query resource-limit observability. (Target: Q4 2026)
- [ ] Enable/validate Phase C parallel optimization gate for the query module and document the exact evidence required for sign-off. (Target: Q4 2026)

### Sprint 4 — Transaction verification closure
- [ ] Finish build verification and execution evidence for transaction phases 1–4, including benchmark artifact generation and release-critical gating. (Target: Q4 2026)
- [ ] Close the remaining crash-recovery and Byzantine/cross-shard fault-injection evidence items, with roadmaps updated to actual pass/fail status. (Target: Q4 2026)

### Sprint 5 — Wave A/B closure sync
- [ ] Reconcile the root roadmap with the latest module roadmaps after Wave A/B closure batches and remove stale “complete” claims that lack execution evidence. (Target: Q4 2026)
- [ ] Build the final release gate package: benchmark evidence, CI artifacts, fail-closed validations, and operator runbooks for the remaining active modules. (Target: Q4 2026)

---

## Notes

- This document is intentionally source-driven: module roadmap claims are treated as evidence only when they are backed by concrete implementation, tests, or benchmark artifacts.
- Closure progress is tracked by module and by wave, not by static roadmap age alone.
- The central objective for the next sprint cycle is to convert the highest-pressure backlog items into issue-scoped, agent-executable tasks.

---

## Suggested GitHub Issue Titles

1. `Sprint: source-traceability cleanup for llm_streaming and vector_search`
2. `Sprint: acceleration GPU/CPU parity and benchmark evidence closure`
3. `Sprint: query planner and federation hardening closure`
4. `Sprint: transaction build/test/benchmark verification closure`
5. `Sprint: Wave A/B roadmap sync and release gate evidence package`

The remainder of the historic issue-priority tables can remain in place for reference, but the sprint-ready backlog above represents the actionable, current backlog that should be converted into GitHub Issues first.
