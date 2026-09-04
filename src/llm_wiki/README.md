# LLM Wiki Module

**Status:** PRODUCTION_CANDIDATE  
**Phase:** 4 complete; Wave B hardening in progress  
**Last Updated:** 2026-09-03  
**Owner:** LLM Platform Team

---

## Overview

The LLM Wiki module is a standalone ThemisDB core module for semantic wiki retrieval, provenance tracking, and guardrail-enforced knowledge access. It is directly connected to the `llm` module for orchestration and prompt planning and to `llama_cpp` for local inference-backed retrieval and summarization flows. It integrates Wikipedia data ingestion with prompt injection detection, workspace state isolation, and edition-based access control for secure multi-tenant deployment.

**Key Capabilities:**
- LLM-based Q&A with safe knowledge retrieval
- Prompt injection detection (5-category guardrail patterns)
- Workspace state management with checksum validation
- Edition gating (Community/Enterprise/Hyperscaler/Military)
- Wikipedia ingestion pipeline with partial-failure semantics
- Atomic state persistence with recovery from corruption

---

## Roadmap Status

**Current Phase:** Phase 3-4 (in progress)  
**Latest Delivery:** 2026-08-10 — Phase 3 error handling + partial Phase 4 test suite complete

**For detailed roadmap:** See `ROADMAP.md` in this directory

### Phase Progress
- [x] Phase 1 — Public SDK interface (ILLMWikiPlugin) + plugin manifest
- [x] Phase 2 — Core implementation (LLMWikiPluginImpl) + Python MVP CLI
- [x] Phase 3 — Error handling & edge cases (Guardrails, workspace state, edition gating)
- [x] Phase 4 — Comprehensive test suite (LWP-01..LWP-20 + Wave B focused tests)
- [ ] Phase 5 — Performance hardening (p95 < 200ms target)
- [ ] Phase 6 — Documentation finalization & GA acceptance

## Implementation Strategy

The LLM Wiki is implemented as a standalone core module with direct coupling to `llm`, `llama_cpp`, `prompt_engineering`, `retrieval`, and `metadata`. The strategy is to keep one shared planning and cost model, then specialize it for wiki-specific evidence selection and provenance tracking instead of introducing a second planner.

### 1. Shared Planning Contract
- Extend the shared RAG cost input once and keep it backward-compatible.
- Pass wiki evidence-package size, provenance depth, transform-chain length, re-anchor state, and provenance confidence through the existing orchestrator path.
- Reuse prompt-enhancement retrieval planning as the upstream planner for wiki routing.

### 2. Wiki-Specific Routing
- Route wiki requests through `llm_wiki` as the semantic core, not as a sidecar retriever.
- Use `llm` for orchestration and response assembly, and `llama_cpp` for local inference-backed summarization and extraction.
- Prefer the cheapest evidence package that satisfies the request, then escalate only when confidence or coverage is insufficient.

### 3. Provenance and Revisions
- Persist revision history at the workspace layer.
- Propagate provenance metadata into chunk, claim, and edge representations as the next step after workspace-level persistence.
- Mark long synthetic chains and low-confidence derivations for re-anchor instead of silently accepting them.

### 4. Validation and Gates
- Add regression tests that prove wiki-specific signals alter cost estimates and routing decisions.
- Keep prompt-enhancement planner behavior deterministic for the same wiki inputs.
- Fail closed on malformed provenance, missing revision data, or invalid transform chains.

### 5. Performance Targets
- Keep wiki routing within the existing prompt-enhancement latency envelope.
- Bound provenance traversal cost under sustained ingest/query cycles.
- Avoid duplicate planner execution on the same request path.

### 6. Orchestration and Timing

The LLM Wiki should follow a scientific-method style control loop: observe, extract, synthesize, validate, then re-anchor when confidence drops.

- Ingestion is an asynchronous, idempotent background step. It should accept source material in small batches, normalize it, attach provenance, and enqueue enrichment work instead of blocking interactive queries.
- Extraction runs immediately after ingestion normalization, but only for the minimum evidence needed to form stable page, claim, chunk, and edge candidates. Cheap heuristics should run first; expensive model-backed extraction should only run when the heuristic stage cannot reach a confident result.
- Synthesis is query-time work. It should happen after prompt-enhancement planning and retrieval have produced a candidate evidence package, so the synthesizer only combines already-selected evidence instead of redoing retrieval logic.
- Validation is a gate between extraction and publication. It should reject malformed provenance, invalid transform chains, stale revisions, and low-confidence derivations before they become durable wiki state.
- Re-anchoring is a deferred repair step. It should run when provenance confidence falls below the configured threshold, when transform chains grow too long, or when repeated syntheses converge on unstable evidence.

Recommended timing tiers:

- Interactive path: query planning, retrieval, and synthesis happen synchronously in the request path, but only against already-indexed material.
- Near-real-time path: extraction and provenance enrichment run shortly after ingestion in a bounded background window.
- Batch path: compaction, consistency sweeps, and low-priority re-anchor jobs run on a schedule or when the system is idle.

Operational rule:

- Do not run a second planner inside the wiki module. The prompt-enhancement layer owns retrieval planning; the wiki consumes its plan, enriches it with provenance and confidence signals, and then synthesizes the final answer.

Practical trigger order:

1. Ingest source material and persist workspace provenance.
2. Extract minimal claims, chunks, and links.
3. Compute confidence and detect chain growth.
4. Validate and publish the evidence package.
5. Synthesize only from published evidence.
6. Re-anchor later if confidence or chain length crosses the threshold.

### 7. Security and Governance Orchestration

Security and governance should be enforced as timed control gates, not as post-processing.

- Pre-ingest gate: verify source policy, workspace policy, and edition entitlement before any data enters wiki state.
- Pre-extraction gate: run guardrail normalization and pattern checks on raw and normalized text to block prompt-injection payloads early.
- Pre-synthesis gate: enforce evidence allowlist rules (trusted origin classes, minimum provenance confidence, maximum synthetic chain length).
- Post-synthesis gate: attach decision metadata for auditability (policy version, gate outcomes, reason codes, and re-anchor requirement).

Governance timing model:

- Synchronous controls: entitlement checks, guardrail checks, and deny decisions run in the interactive request path.
- Deferred controls: policy drift scans, provenance integrity sweeps, and governance conformance reports run in near-real-time or batch windows.
- Release controls: route and policy behavior must be locked by focused regression tests before a module release gate is marked complete.

Minimum governance evidence per request:

1. Policy snapshot identifier used for the decision.
2. Edition/feature gate outcome.
3. Guardrail decision and matched category (if blocked).
4. Provenance confidence and chain-depth metrics.
5. Re-anchor flag and rationale.
6. Final allow/deny decision with reason code.

### 8. YAML Process Orchestration and ML Control

LLM Wiki process behavior is controlled through a versioned YAML policy so ML can optimize process parameters without changing code.

- Process policy: `src/llm_wiki/process/llm_wiki_process_policy.yaml`
- Policy schema: `src/llm_wiki/schema/llm_wiki_process_policy.schema.json`

How ML works in this model:

1. It reads telemetry and optimization goals from policy and runtime outcomes.
2. It tunes only policy-approved knobs (for example evidence size or confidence thresholds).
3. It stays within hard bounds defined in policy.
4. It must run updates in shadow/canary before enforced rollout.
5. It cannot change non-tunable safety invariants (guardrails, entitlement gates, fail-closed validation, planner ownership).

---

## Architecture & Key Components

The module uses a **plugin-based architecture** with public SDK boundary and private enterprise implementation:

**Core Module Coupling:**
- `llm` — orchestration, prompt routing, and response assembly
- `llama_cpp` — local model execution for retrieval and summarization
- `prompt_engineering` — retrieval-planning handoff and prompt enhancement
- `retrieval` / `metadata` — ranking, provenance, and audit support

**Public Interface (include/llm_wiki/):**
- `llm_wiki_plugin_interface.h` — ILLMWikiPlugin contract; editions + capabilities
- `plugin.json` — Manifest with visibility, allowed editions (enterprise/hyperscaler/military)

**Core Implementation (src/llm_wiki/):**
- `guardrail_patterns.h` — 60+ injected-command patterns; shell/code/encoding/privilege/control-flow categories
- `workspace_state_manager.h` — Workspace isolation; checksum-based state validation
- `edition_gate.h/.cpp` — Edition-gated access control enforcement
- `process_policy_manager.cpp` — YAML process policy loader + runtime invariant validation
- `workspace_state_manager.cpp` — Atomic write-replace + log-based recovery

**Private Plugin (plugins/private/themisdb_llm_wiki/):**
- `wikipedia/llm_wiki_plugin_impl.cpp` — LLMWikiPluginImpl; Wikipedia ingestion bridge
- `LLMWikiPluginImpl` — Query + ingest operations with guardrail checks

**Python CLI (scripts/llm_wiki_mvp.py):**
- MVP interface for index/query/workspace commands

---

## Gate Evidence & Testing

**Focused Tests:**
- `tests/llm/test_llm_wiki_phase4_roundtrip.cpp` — ingest/query roundtrip coverage
- `tests/llm/test_llm_wiki_edition_gates.cpp` — edition/feature gate validation
- `tests/llm/test_wave_next_llm_wiki_rocksdb.cpp` — RocksDB backend + fallback-path coverage
- TIMEOUT 120s per test (standard Phase 4+ gate)

**Test Categories (LWP-XX gates):**
- LWP-01..04 — SDK interface (create, query, ingest, status)
- LWP-05 — Guardrail injection detection
- LWP-06 — Workspace state + checksum validation
- LWP-07 — Edition gating (allowed/denied paths)
- LWP-08 — Error handling (invalid input, state corruption)
- LWP-09..16 — Workspace lifecycle (create, delete, orphan detection)
- LWP-17..20 — Guardrail pattern comprehensive coverage

**Performance Gates (Phase 5 target):**
- Query p95 < 200ms (at 5k chunks)
- Ingest throughput > 1k pages/sec

---

## Documentation & APIs

**API Reference:**
- `include/llm_wiki/llm_wiki_plugin_interface.h` — Public C++ contract; factory export (themisdb_llm_wiki_create)
- Doxygen comments cover all public methods

**Design Docs:**
- `ROADMAP.md` — Detailed phase breakdown and delivery evidence
- `schema/llm_wiki_entity.schema.json` — Versioned stable-core + extension contract for wiki entities
- `process/llm_wiki_process_policy.yaml` — YAML control plane for stage timing, gates, and ML knobs
- `schema/llm_wiki_process_policy.schema.json` — Validation schema for process policy YAML
- Phase 3 error handling: `src/llm_wiki/guardrail_patterns.h` (pattern taxonomy)
- Phase 3 workspace: `src/llm_wiki/workspace_state_manager.h` (state lifecycle)
- Phase 3 gating: `src/llm_wiki/edition_gate.h` (edition contract enforcement)

---

## Known Issues & Limitations

**Reference:** See `FUTURE_ENHANCEMENTS.md` and `ROADMAP.md` Phase 5-6 sections

**Open Gate Items:**
- Representative hardware performance baselines (p95/p99) for Wave B exit
- Wikipedia ingest throughput evidence on CI/HW lanes

**Out of Scope (Phase 2+):**
- Custom NLP model training or fine-tuning
- Non-Wikipedia knowledge sources (may be added as plugins)
- Real-time collaboration or multi-user sessions

---

## Dependency Graph

**Depends On:**
- `llm` module — LLM inference backend
- `retrieval` module — Vector search (optional, for semantic retrieval)
- `utils` module — String utilities, JSON serialization
- RocksDB (optional; for Phase B persistent cache)

**Depended By:**
- API layer (`api/grpc/`) — Exposes LLM Wiki as gRPC service
- Enterprise plugins — Build on public ILLMWikiPlugin interface

---

## Build & Integration

**CMake Targets:**
- `themis_llm_wiki_plugin` — Public SDK library
- `module_llm_wiki_*_focused` — Focused test targets (Phase 4+)
- `bench_llm_wiki_plugin` — Performance benchmarks (Phase 5+)

**Edition Gating:**
- Community: No LLM Wiki (feature not included)
- Enterprise+: Full LLM Wiki with guardrails + workspace isolation

**Feature Flags:**
- `THEMISDB_WIKI_PHASE_B` — Enables RocksDB-backed retrieval path when available
- `fail_open=true` (config) — explicit test/degraded in-memory fallback when RocksDB init fails

---

**Last Updated:** 2026-09-03  
**Next Review:** 2026-09-30 (Wave B gate validation)
