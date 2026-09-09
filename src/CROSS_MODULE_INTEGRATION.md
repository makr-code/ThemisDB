# Cross-Module Integration

## Purpose

This source-root companion points from the `src/` aggregation boundary to the canonical cross-module architecture evidence set.

## Current Facts

- `src/` currently contains **72** top-level module paths in this clone.
- Direct include-derived coupling currently spans **392** unique module-to-module edges, the runtime overlay documents release-critical execution paths, and `src/MODULE_FUNCTION_USAGE_MAP.md` provides repository-wide best-effort symbol-consumer coverage above those module-level graphs.
- The include graph still contains one large 50-module SCC, so architecture review should assume meaningful cross-module coupling across the core runtime hubs.
- `src/llm_streaming/` and `src/vector_search/` are **docs-only module paths** today; release evidence must be traced to canonical runtime files outside those directories.
- `src/ai_working/` is a source-root evidence/working area, not a runtime subsystem.

## Canonical Cross-Module References

- [`../docs/architecture/MODULE_ARCHITECTURE.md`](../docs/architecture/MODULE_ARCHITECTURE.md) — all 72 `src/` module paths grouped by layer with footprint signals
- [`../docs/architecture/MODULE_INTEGRATION_CONTRACTS.md`](../docs/architecture/MODULE_INTEGRATION_CONTRACTS.md) — direct inter-module dependency mapping, SCC findings, and runtime integration overlay
- [`../docs/architecture/DATA_FLOW_PATHS.md`](../docs/architecture/DATA_FLOW_PATHS.md) — critical Query/Storage/Transaction, 2PC, RAG, and Server/LLM flows
- [`../docs/architecture/RELEASE_ARCHITECTURE_STATUS.md`](../docs/architecture/RELEASE_ARCHITECTURE_STATUS.md) — release posture, maturity signals, and docs-only path caveats
- [`MODULE_FUNCTION_USAGE_MAP.md`](MODULE_FUNCTION_USAGE_MAP.md) — repository-wide per-module symbol-consumer and call-site companion
- [`ROADMAP.md`](ROADMAP.md) — source-root backlog and release gating context
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — source-root aggregation contract for `src/`
