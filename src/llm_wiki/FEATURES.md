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

# LLM Wiki Capabilities

## Module role

The LLM Wiki module supplies ThemisDB with a typed plugin boundary for wiki-related ingestion, queries, workspace management, and optional Wikipedia dump ingestion. Its current value is the shared interface and the supporting workspace, policy, edition, and guardrail components. Integrating services can use those components to build a controlled knowledge workflow, but the current core implementation does not yet provide complete source parsing or indexed query retrieval.

## Capabilities present in source

- **Plugin contract:** `ILLMWikiPlugin` defines initialization, ingest/query operations, workspace helpers, statistics, and Wikipedia dump ingestion result types.
- **Workspace artifacts:** the workspace orchestrator creates a raw-source directory and wiki page, index, log, and state files; it records page metadata and lightweight provenance/revision data.
- **Input screening:** a plugin-local fixed-string matcher can flag a subset of query and chunk text. A separate `WikiGuardrails` utility offers broader patterns; the plugin-local matcher does not automatically inherit that full set.
- **Policy validation:** an optional YAML process policy is checked against its JSON Schema and semantic invariants; optional reload retains the prior in-memory policy if candidate validation fails.
- **Edition gate:** the core checks a compile-time edition switch. This is not a runtime license or caller-authorization service.
- **Optional persistence building block:** `RocksDbWikiStore` is compiled and linked when the CMake configuration finds a RocksDB target.

## Operational value and boundaries

These components give maintainers a concrete contract and workspace format to extend and test. They do not establish tenant authentication, document-level authorization, cryptographic provenance, an audit service, or production performance SLAs. Current plugin ingestion writes a placeholder chunk rather than parsing file contents; query returns in-memory chunks without applying the declared top-k/min-score options; and Wikipedia dump ingestion returns fixed counts.

Before deploying an integration, validate the exact build and request path, enforce authorization and resource limits in the service boundary, and review [`SECURITY.md`](SECURITY.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), and [`MODULE_GAPS.md`](MODULE_GAPS.md).

## Related documentation

- [`README.md`](README.md) — scope and build/test entry points
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — source-backed component and data-flow map
- [`ROADMAP.md`](ROADMAP.md) — implementation work and readiness gates
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) — benchmark scope and pending targets

> Exact model backend not verifiable from final document text alone.
