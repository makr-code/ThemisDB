---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: api-contract
Canonical_Sources: include/process/*.h; src/process/API.md; src/process/IMPLEMENTATION.md; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Public API Documentation Index

**AI provenance:** Exact model backend not verifiable from final document text alone.

The public declarations in this directory are the API authority. For the source-checked integration overview, see [`src/process/API.md`](../../src/process/API.md) and [`src/process/IMPLEMENTATION.md`](../../src/process/IMPLEMENTATION.md). Earlier versions of this page contained unverified sample programs, storage-key lists, AQL queries, and convenience APIs; those examples are intentionally not presented as supported usage here.

## Quickstart

Choose the relevant public header from the map below, read its declaration and contract comments, then check the matching implementation and focused test. Use the linked API and implementation guides for a source-checked integration route; do not copy examples from historical snapshots.

## Public header map

Headers currently present in this directory cover:

- Process models, serializers, and format importers.
- Process graph retrieval, agentic/light retrieval, descriptors, and community detection.
- Linking, concurrency, determinism, diagnostics, and hardening contracts.
- Orchestration profiles, federation, conflict resolution, audit, and telemetry surfaces.
- Object-centric tracing and OCEL export.

This is a category summary, not a closed or exhaustive API inventory. For exact types, signatures, field defaults, and behavior, inspect the relevant header and implementation. An interface or design contract alone is not proof that a feature is production-ready.

## Current caveats

- `ProcessLightRetriever` declares resource limits that its current `retrieve()` implementation does not enforce.
- Orchestration profile activation/rollback is multiple writes without a transaction guarantee.
- Audit and telemetry implementations include simplified behavior; verify durability and external transport before relying on them.
- Federation contracts and implementations do not by themselves establish consensus safety or deployment maturity.

## Build and tests

There is no `src/process/CMakeLists.txt`. Process tests are registered through [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt); process benchmarks are conditionally registered through [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt). Registration does not establish a successful build or test run. Choose commands/targets from the configured root build rather than relying on commands from older versions of this page.

## Related documentation

- [`src/process/API.md`](../../src/process/API.md) · [`src/process/ARCHITECTURE.md`](../../src/process/ARCHITECTURE.md)
- [`src/process/README.md`](../../src/process/README.md) · [`src/process/SECURITY.md`](../../src/process/SECURITY.md)
- [`DOCUMENTATION_GOVERNANCE.md`](../../DOCUMENTATION_GOVERNANCE.md)
