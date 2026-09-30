# ThemisDB Layered Architecture Blueprint

**Author:** ThemisDB Contributors  
**Created:** 2026-09-30  
**Last Updated:** 2026-09-30  
**Status:** active

## Purpose

This document defines a concrete architecture blueprint for ThemisDB that aligns runtime layering with current source documentation and module roadmaps.

Primary alignment sources:
- `src/ARCHITECTURE.md`
- `src/README.md`
- `src/core/FUTURE_ENHANCEMENTS.md`
- `src/query/FUTURE_ENHANCEMENTS.md`
- `src/storage/FUTURE_ENHANCEMENTS.md`
- `src/sharding/FUTURE_ENHANCEMENTS.md`
- `src/llm/FUTURE_ENHANCEMENTS.md`
- `src/server/FUTURE_ENHANCEMENTS.md`
- `src/api/FUTURE_ENHANCEMENTS.md`

## Target Architectural Planes

### 1) Core Runtime / Kernel

Scope:
- `base`, `core`, core interface contracts, runtime lifecycle essentials

Role:
- Keep the kernel small, stable, testable, and deterministic.
- Own foundational contracts and lifecycle boundaries.

### 2) Query and Execution Plane

Scope:
- `query`, `aql`, `execution`, planner/runtime orchestration paths

Role:
- Translate requests into validated execution plans.
- Execute with deterministic resource and failure behavior.

### 3) Storage Reliability Plane

Scope:
- `storage`, `cache`, `metadata`, `transaction`, `temporal`, `timeseries` integration points

Role:
- Provide persistence, durability, recovery, and deterministic maintenance semantics.

### 4) Distributed Infrastructure Plane

Scope:
- `sharding`, `replication`, `failover`, `cdc`, `network`, distributed coordination paths

Role:
- Provide routing, topology, quorum-aware behavior, and bounded rebalance/repair flows.

### 5) AI / RAG / ML Capability Plane

Scope:
- `llm`, `rag`, `retrieval`, `training`, `prompt_engineering`, and related accelerators/adapters

Role:
- Deliver optional AI/ML capabilities through stable interfaces and explicit fallback behavior.

### 6) Application / Protocol Layer

Scope:
- `server`, `api`, protocol handlers, gateway/middleware, plugin-facing application entry points

Role:
- Expose ThemisDB capabilities to clients over HTTP/gRPC/WebSocket/etc. with authn/authz and validation gates.

## Architectural Principles

1. **Kernel stability first**: kernel contracts must remain compact and regression-tested.
2. **Dependency direction is inward**: upper planes depend on lower contracts, not vice versa.
3. **Interface-led integration**: AI and distributed capabilities integrate through interfaces/adapters, not deep kernel coupling.
4. **Fail-closed defaults**: degraded/invalid states must be explicit and observable.
5. **Deterministic operations**: routing, transaction, recovery, and timeout outcomes must be bounded and diagnosable.
6. **Additive evolution**: active major versions preserve compatibility and migrate by versioned contracts.

## Dependency-Direction Rules

Allowed direction (high-level):

`Application/Protocol -> Capability & Distributed & Query -> Storage Reliability -> Core Runtime/Kernel`

Hard rules:
- Core Runtime/Kernel MUST NOT import AI/LLM implementation details.
- Core Runtime/Kernel MUST NOT depend on protocol handlers or server transport details.
- AI/RAG/ML and Distributed modules may consume kernel/storage/query contracts, but must not back-couple implementation dependencies into kernel internals.
- Query plane may reference distributed routing contracts and capability contracts, but should avoid direct feature-plane implementation coupling where interface seams exist.
- Any circular dependency (`llm <-> query`, `llm <-> server`, `sharding <-> transaction`) is design debt and must be reduced via contract extraction and adapter boundaries.

## Module Boundary Clarification (Current-to-Target)

### Core (`src/core`)
Target boundary:
- Own DI/runtime concern orchestration and adapter lifecycle contracts.
- Stay functional without optional AI/distributed feature implementations.

Derived from module future work:
- Emphasis on thread-safe `ConcernsContext`, adapter validation, hot-swap bounds, and optional cache behavior.

### Query (`src/query`)
Target boundary:
- Own parsing, optimization, execution safety, and federation control semantics.
- Consume capability contracts (e.g., reranking/retrieval) without hard-linking to feature implementations.

Derived from module future work:
- Safety hardening, semantic equivalence (JIT/interpreter), bounded federation behavior, and RAG-aware planner contracts.

### Storage (`src/storage`)
Target boundary:
- Own durability/recovery semantics and deterministic maintenance behavior.
- Expose explicit persistence/recovery interfaces consumed by query/transaction/distributed components.

Derived from module future work:
- Replay/recovery diagnostics parity, deterministic stress reliability, bounded degradation.

### Sharding (`src/sharding`)
Target boundary:
- Own shard routing, topology updates, and rebalance/repair orchestration.
- Integrate with transaction through explicit coordination contracts, not bidirectional implementation coupling.

Derived from module future work:
- Deterministic routing semantics, fail-closed latency evidence behavior, GA proof under quorum/migration stress.

### LLM (`src/llm`)
Target boundary:
- Provide optional inference/routing/policy capability through stable interfaces.
- Avoid introducing mandatory dependencies into kernel-critical paths.

Derived from module future work:
- Stable inference APIs, explicit fallback, lifecycle ownership, distributed execution hardening.

## Externalization Strategy (Optional, Self-Contained Modules)

Goal:
- Keep kernel/reliability planes lean while enabling optional capability growth.

### Externalization candidates (phaseable)

1. **AI/ML optional stacks**
   - `llama_cpp`, `whisper`, `stable_diffusion`, selected `training` toolchains
2. **Specialized capability engines**
   - `vector_search` (docs-only in-tree path today), optional advanced `geo`/`timeseries` accelerator packages
3. **Integration adapters and import/export packs**
   - selected `importers`/`exporters` connectors with heavy third-party coupling

### Repo-boundary rules

- Public contracts stay in this repository (`include/` and module contract docs).
- Externalized repositories implement those contracts and version against them.
- Core release lanes MUST remain operable without externalized optional modules.
- Community workflows remain fail-closed for private-only assets and credentials.

## Migration Roadmap (First Phases)

### Phase A — Contract and boundary freeze (Target: Q4 2026)
- Baseline dependency map and mark all prohibited reverse dependencies.
- Extract/confirm contract seams used by `core`, `query`, `storage`, `sharding`, `llm`.
- Define CI checks for dependency-direction violations at plane boundaries.
- Acceptance signal: dependency-policy checks run in CI and report zero new reverse-dependency violations for plane boundaries.

### Phase B — Circular dependency reduction (Target: Q1 2027)
- Resolve highest-risk cycles first: `llm <-> server`, `llm <-> query`, `sharding <-> transaction`.
- Introduce adapter/facade layers where direct implementation imports remain.
- Keep behavior unchanged while reducing compile-time/runtime coupling.
- Acceptance signal: each tracked cycle has a documented contract seam and at least one removed direct implementation import.

### Phase C — Capability packaging and externalization pilots (Target: Q2 2027)
- Pilot external packaging for optional AI/ML stacks and selected heavy adapters.
- Keep contract compatibility tests in-tree and run cross-repo integration checks.
- Promote only modules that are independently operable and observably bounded.
- Acceptance signal: at least one pilot capability package passes contract compatibility and cross-repo integration checks without kernel contract changes.

## Non-Goals

- No runtime behavior changes in this blueprint.
- No immediate reclassification of every module in `src/`.
- No forced externalization of modules that still own kernel-critical behavior.

## Acceptance Signals for This Blueprint

- Architecture planes and dependency direction are explicit and source-aligned.
- Kernel-vs-capability separation is formally documented.
- Core/query/storage/sharding/llm boundaries are clarified with module-derived rationale.
- Externalization strategy and first migration phases are concrete and additive.
