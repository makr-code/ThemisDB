---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `src/sharding/`, `tests/sharding/CMakeLists.txt`
Related issues: #7093, #7167

# Sharding module guide

## Audience and ownership

For maintainers working on routing, cluster coordination, cross-shard transactions, WAL, or shard migration. This is a distributed correctness boundary; module contract text is in `include/sharding/sharding_api_contract.h`.

## Start with the change boundary

- Routing: `shard_router.h`, `src/sharding/shard_router.cpp`.
- Cross-shard state: transaction/coordination headers and their implementation.
- WAL or migration: identify its exact header and corresponding source; do not assume a router change covers it.
- Contract regression evidence: `tests/sharding/test_sharding_contract_hardening_focused.cpp`.

## Maintainer workflow

1. Read the relevant contract section before changing behavior.
2. Follow the state/return path through the implementation, including timeout, unavailable-shard, and recovery cases.
3. Preserve explicit outcomes for exhausted fallback, quorum/coordination failure, WAL corruption, and migration failure.
4. Add focused tests and run the matching target from `tests/sharding/CMakeLists.txt`.
5. Update `API.md` when callers observe a contract change; update `IMPLEMENTATION.md` when invariants or source mapping change.

See `ARCHITECTURE.md`, `ROADMAP.md`, and `RUNBOOK.md` for broader topology and operations context.
