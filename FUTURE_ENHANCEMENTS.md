# ThemisDB Future Enhancements (Root Index)

> Status: Active
> Last Updated: 2026-08-28

This root file is the canonical entry point for future enhancement planning.
Detailed implementation backlogs live close to code and are aggregated in module-level documents.

## Root Planning Scope

- Cross-module enhancement direction and release prioritization are tracked in `ROADMAP.md`.
- Module-specific enhancement details are tracked in `src/FUTURE_ENHANCEMENTS.md` and `src/<module>/FUTURE_ENHANCEMENTS.md`.
- Archived enhancement snapshots are stored in `docs/ARCHIVED/ai-working-history/`.

## Overall v2.5.0 → v3.0.0 module evolution concept

The module issue portfolio is not meant to be a collection of isolated documentation tasks. It forms a single evidence-first rollout for the project-wide transition from v2.5.0 to v3.0.0.

### Goal

Convert each module from a vague or partially documented status into an explicit, source-backed engineering target with measurable validation and release gate conditions.

### Core principle

Every module issue must answer the same questions with direct evidence:

- What is the actual current state of the module today?
- Which source files, contracts, and tests define that state?
- What is the required v3.0.0 target state and why does it matter?
- Which runtime constraints, failure modes, and safety gates must hold?
- What validation proves the module is ready for the next release phase?

### Release wave model

The rollout is organized as phases instead of a single monolithic roadmap item:

1. Phase 1 — evidence and state normalization
   - confirm the real source state for each module
   - align docs, tests, and implementation around current evidence
   - identify missing or misleading statements in roadmap/module docs

2. Phase 2 — contract and architecture hardening
   - define the public contract and internal boundaries
   - remove ambiguity from API, lifecycle, and failure semantics
   - establish explicit compatibility and degraded-state behavior

3. Phase 3 — performance and reliability improvements
   - tie backend selection, queue policies, caching, retrieval, and execution to measurable targets
   - enforce bounded retry/fallback behavior and fail-safe transitions
   - prioritize modules with direct latency or correctness risk

4. Phase 4 — security, resilience, and validation
   - verify authorization, policy enforcement, trigger safety, and recovery design
   - require fail-closed behavior under invalid or partial trust state
   - prove resilience and failure handling under representative load

5. Phase 5 — release-grade evidence and module handoff
   - complete benchmark, tests, and operational proof for each module
   - document unresolved risks and dependencies explicitly
   - hand off to the release-ready branch only after evidence is stable

### Module rollout order

The first priority set contains the modules that define the core system behaviour and the highest risk for release trust:

- acceleration
- query
- search
- llm
- rag
- gpu
- sharding
- replication
- auth
- security

The next wave covers integration and platform layers:

- access_model
- ai
- ai_working
- analytics
- api
- aql
- cache
- cdc
- chaos
- config
- content
- core
- distributed_knowledge
- distributed_tensor
- execution
- failover
- governance
- graph
- index
- ingestion
- maintenance
- observability
- performance
- plugins
- process
- projects
- scheduler
- server
- storage
- training
- transaction

The remaining modules follow the same rule: each issue must be module-specific, evidence-based, and tied back to the release wave plan instead of treated as standalone documentation churn.

### Acceptance gates for the overall plan

A module is ready to move forward only if all of the following are true:

- the current state is verified against source and tests
- invalid, partial, or degraded execution states are explicit
- performance and safety claims are tied to measurable thresholds
- documentation and code agree on the public contract and failure behavior
- rollback, validation, and remaining risks are recorded

This is the baseline for the v2.5.0 → v3.0.0 release transition: engineering evidence first, then implementation, then final verification and handoff.

## Source-Verified Roadmap Reality (2026-09-07)

The enhancement plan must follow the same evidence discipline as the roadmap itself: no status claim is treated as release evidence unless backed by source, tests, or CI artifacts.

### Wave model, reality-based

- **Wave A — Runtime reliability**: dedicated transaction/GPU CI execution is green again on `develop`; the next gap is no longer missing CI lanes but missing authoritative representative-hardware and chaos/recovery artifacts from those lanes. Latest evidence follow-up: transaction artifact jobs needed a mandatory sccache-policy fix and the GPU representative-hardware path now runs only when explicitly requested on available self-hosted hardware.
- **Wave B — Performance consolidation**: materially complete for search, access model, and LLM wiki, but representative-hardware and benchmark closure remain incomplete.
- **Wave C — Security and audit validation**: implementation is present and useful, but end-to-end certification remains partial.
- **Wave D — Operability and soak hardening**: mostly planning; active implementation is still low compared to earlier WAVE claims.

### Realistic target sequence

1. Close the remaining Query FTS gate plus GPU/Transaction representative-hardware evidence gaps.
2. Complete representative-hardware benchmarks for critical modules.
3. Convert audit and security evidence from design-validation into end-to-end proof.
4. Finish Wave D operability and soak-test work only after Wave A/B evidence is stable.

### Wave-Closure implementation sequence (active)

1. **Wave A residual gates first:** Transaction + GPU authoritative evidence, `release_critical` closure, and p95/p99 refresh for Wave-A modules.
2. **Wave B residual hardening second:** close remaining high-pressure modules (`query`, `acceleration`, `llm_wiki`, `llm`, `index`, `rag`, `search`, `updates`) plus Query↔Index↔Storage and LLM↔RAG↔LLM_Wiki integration evidence.
3. **Wave C sign-off third:** consolidate sustained-load `auth`/`security`/`governance` security evidence and fail-closed boundary validation into final compliance package.
4. **Wave D sign-off fourth:** close operability/runbook/diagnostics/alerts/recovery and operator-readiness gates with refreshed chaos/failover/recovery baselines.
5. **Only after 1–4:** final root-governance sync and human GA/program sign-off request.

### Findings-fix execution order (2026-09-09)

1. Build a source-validated remediation backlog from `ROADMAP.md`, module roadmaps, and current CI logs.
2. For each open Wave gate, publish and track a closure package manifest (`*_closure_manifest.json` + `*_closure_manifest.md`) with run IDs, artifacts, and source-validation flags before status promotion.
3. Repair the Wave-B transaction CI lane on `develop` so it provisions the same required dependency set as the working mainline/release workflows. ✅ closed 2026-09-09 (`34313051247`)
4. Repair the Wave-A GPU CI lane on `develop` so public/community execution does not fetch unavailable private submodules and so target compilation status is reported correctly. ✅ closed 2026-09-09 (`34313042741`)
5. Query FTS performance gate (`<=100ms` on 100K documents) is now closed with reproducible local benchmark evidence from `benchmarks/rag/bench_fts_phase_b.cpp` on 2026-09-09; next work is cross-environment baseline capture, not missing executor functionality.
6. Harvest authoritative representative-hardware and p95/p99 artifacts from the refreshed GPU/Transaction evidence lanes after the patched reruns complete; in parallel, broad auth/query/voice baseline refresh automation is now queued on `Build: Benchmarks` run `34345454063`.
7. Execute the remaining high-risk hardening backlog in `llm`, `server`, `training`, and `llm_wiki`.
8. Re-sync root governance docs only after blocker status is re-validated from code, tests, CI, and benchmarks. ✅ completed for the CI-green + Query FTS benchmark revalidation
9. Request human GA/program sign-off only after no critical blocker remains open.

## Required Enhancement Sources

- [src/FUTURE_ENHANCEMENTS.md](src/FUTURE_ENHANCEMENTS.md)
- [src/README.md](src/README.md)
- [ROADMAP.md](ROADMAP.md)
- [CHANGELOG.md](CHANGELOG.md)
- [docs/MODULE_EVOLUTION_ISSUE_BLUEPRINT.md](docs/MODULE_EVOLUTION_ISSUE_BLUEPRINT.md)

All v2.5.0 → v3.0.0 work must follow the issue blueprint to keep module-level enhancement requests concrete, evidence-based, and rollout-ready.

## Governance Constraints

- Enhancement entries should include measurable targets, failure behavior, and test strategy.
- Ambiguous placeholders like "improve" or "optimize" must be replaced with concrete acceptance criteria.
- Branch and release assumptions must align with `BRANCHING_STRATEGY.md`, `RELEASE_STRATEGY.md`, and `VERSIONING.md`.
