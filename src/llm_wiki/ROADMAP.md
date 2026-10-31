---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Module Roadmap

> Project-level release and GA posture is canonical in the root [`ROADMAP.md`](../../ROADMAP.md). This module roadmap describes the source in this checkout and does not promote release readiness.

## Current Status

The repository contains a public `ILLMWikiPlugin` contract, a `themis_llm_wiki` CMake target, a workspace orchestrator, an edition gate, a policy loader, a separate workspace-state manager, and an optional RocksDB wrapper. The plugin API implementation is incomplete relative to its declared behavior: ingestion writes a placeholder chunk, plugin query does not retrieve/rank results or honor query options, and Wikipedia dump ingestion returns fixed counts. Treat the module as under implementation/hardening, not production-ready.

## In Progress

- [~] Document and test the distinction between declared SDK behavior and the current plugin implementation (Target: Q4 2026)
- [~] Validate workspace persistence and policy reload failure behavior with focused tests (Target: Q4 2026)
- [ ] Establish production-path retrieval and ingestion behavior before publishing performance claims (Target: Q4 2026)

## Planned Features

- [ ] Parse eligible source files into content chunks and report partial failures accurately (Target: Q1 2027)
- [ ] Connect plugin queries to the intended index reader and honor top-k, score, workspace, and save-page options (Target: Q1 2027)
- [ ] Implement Wikipedia dump ingestion through the declared pipeline and feature gate (Target: Q1 2027)
- [ ] Define and validate a migration/compatibility contract for persisted workspace and entity data (Target: Q1 2027)
- [ ] Establish representative-hardware benchmarks for the production request path (Target: Q1 2027)

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define public plugin request/result structures and lifecycle operations in `include/llm_wiki/llm_wiki_plugin_interface.h`.
- [ ] Reconcile API comments and supported behavior for every operation before treating the contract as stable (Target: Q4 2026).

### Phase 2 — Core Implementation
- [~] Keep the module library, edition gate, workspace components, policy manager, and plugin implementation build-integrated.
- [ ] Replace placeholder source ingestion with content parsing and deterministic chunk generation (Target: Q1 2027).
- [ ] Connect plugin query to a real index-reader path and apply declared options (Target: Q1 2027).
- [ ] Replace fixed-result Wikipedia handling with actual pipeline delegation or explicitly remove that operation from the supported surface (Target: Q1 2027).

### Phase 3 — Error Handling & Edge Cases
- [~] Preserve explicit status/result categories for initialization, edition denial, policy errors, and workspace failures.
- [ ] Define behavior for missing/unreadable source files, unsupported glob patterns, partial writes, and policy reload failures (Target: Q1 2027).
- [ ] Remove or strictly constrain fail-open behavior before relying on pattern screening as a security control (Target: Q1 2027).

### Phase 4 — Tests
- [~] Maintain focused tests under `tests/llm_wiki/` and LLM-specific tests under `tests/llm/`.
- [ ] Add regression assertions for content-based ingestion, option-aware retrieval, and real dump-import results (Target: Q1 2027).
- [ ] Record passing test results only from a configured build with tests actually discovered and executed (Target: Q1 2027).

### Phase 5 — Performance / Hardening
- [ ] Benchmark the actual ingestion and retrieval paths on representative hardware; do not use synthetic benchmark stubs as production evidence (Target: Q1 2027).
- [ ] Establish memory, input-size, timeout, concurrency, and persistence failure limits for supported deployment paths (Target: Q1 2027).
- [ ] Close security and durability review items in [`SECURITY.md`](SECURITY.md) and [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) (Target: Q1 2027).

### Phase 6 — Documentation & Acceptance
- [x] Document the current source behavior and known implementation gaps.
- [ ] Review the module documentation against the final implementation and obtain maintainer sign-off (Target: Q1 2027).
- [ ] Attach configured test and representative benchmark evidence before declaring module readiness (Target: Q1 2027).

## Production Readiness Checklist

- [ ] Ingestion persists actual source content and reports failures correctly.
- [ ] Query performs retrieval and honors its public options.
- [ ] Wikipedia ingestion reads the requested input and returns measured results.
- [ ] Edition/feature authorization and caller/workspace authorization are validated for the shipped integration.
- [ ] Workspace persistence has documented integrity, recovery, resource, and concurrency guarantees.
- [ ] Focused tests are configured, discovered, and pass in the supported build lanes.
- [ ] Production-path performance evidence is recorded against [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
- [ ] Security and operational requirements are reviewed and signed off by maintainers.

## Known Issues & Limitations

See [`MODULE_GAPS.md`](MODULE_GAPS.md) for the source-verified list. The most significant current limitations are placeholder ingestion content, non-retrieval plugin query behavior, fixed Wikipedia results, compile-time-only edition selection, and synthetic-only benchmark implementations.

## Breaking Changes

- Changes to the public request/result structures or plugin lifecycle are API changes and require compatibility review.
- No version or ABI stability guarantee is inferred from the current header's version comments.

> Exact model backend not verifiable from final document text alone.
