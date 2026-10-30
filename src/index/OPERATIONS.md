---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "src/index/ROADMAP.md"
  - "src/index/PERFORMANCE_EXPECTATIONS.md"
  - "src/index/PRODUCTION_REQUIREMENTS.md"
  - "include/index/ann_frontdoor.h"
  - "include/index/vector_index.h"
  - "include/index/gpu_vector_index.h"
  - "include/index/index_manager.h"
  - "docs/operability/RUNBOOK_INDEX_ENGINE.md"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index operations and readiness

## Purpose and reader

This document is for operators and release owners responsible for index
availability, retrieval quality, resource use, and lifecycle safety. It
describes what to observe and validate from supported interfaces; deployment
configuration, alert thresholds, service commands, and recovery tooling remain
owned by the actual deployment.

## Operational ownership

The index module owns index structures, backend selection/results, and exposed
index lifecycle methods. Storage durability and backup/restore, identity and
authorization, embedding production/version selection, query planning, and GPU
device health are integration boundaries. Establish an owner and runbook for
each boundary before enabling a new index path.

## Routine checks

- Track vector index initialization dimension/metric, vector count and
  `getStatistics()` status; compare with expected ingestion state.
- Track ANN `strategy_used`, route/fallback reason, result cardinality, and
  distributed shard success/failure/partial-result metadata. Choose
  deployment-specific alerts; the API does not define an SLO threshold.
- When using `GPUVectorIndex`, inspect active backend, GPU-active state, query
  statistics, VRAM and (if enabled) oversubscription partition/load/eviction
  statistics. Increased fallback or resource pressure can change latency and
  service behavior.
- Confirm registration is complete before concurrent frontdoor queries.
- For manifest-enabled integrations, inspect current model/dimension/chunking/
  schema version and last reindex status. Remember the manifest serializer does
  not itself persist to RocksDB or enforce the adjacent JSON Schema.
- Keep backups and an approved restore path for persistent index data. A
  metadata rollback predicate is not a data restore operation.

## Change and release checks

Before deploying an index/backend change:

1. Validate interface preconditions and failure paths with focused tests from
   [`tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt).
2. Run the benchmark cases mapped by
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) against the
   same release baseline and report dataset, metric, build flags, backend, and
   hardware. Do not infer a production latency target from a test default.
3. Compare approximate output against an exact baseline and check recall,
   ordering, result count, and invalid-distance handling.
4. Validate rebuild/reindex and restore procedures on representative data
   before changing the serving index.
5. Confirm logs/metrics/traces used by deployment alerts are actually emitted
   by that build. The downstream
   [`RUNBOOK_INDEX_ENGINE.md`](../../docs/operability/RUNBOOK_INDEX_ENGINE.md)
   contains proposed operational signals and commands that must be checked
   against the target deployment before use.

The module roadmap records GPU backend parity on representative hardware as
pending. Do not mark GPU parity or all-backend production readiness complete
from CPU/stub test results alone.

## Capacity and degradation decisions

Dataset size and index configuration affect backend choice and memory/latency
trade-offs. AnnFrontdoor's size thresholds are configurable defaults, not
capacity guarantees. GPU memory oversubscription can page/stream partitions
and incurs different residency behavior; monitor the reported counters where
that option is active. Set operational limits using deployment measurements,
not unsupported universal numbers.

When degradation is observed, consult [RUNBOOK.md](RUNBOOK.md). If CPU fallback
is permitted, record it as a changed operating mode; if fail-closed behavior
is required, do not toggle fallback to hide an unavailable accelerator.

## Canonical sources

- [Index architecture and ownership](ARCHITECTURE.md)
- [Current status and open gates](ROADMAP.md)
- [Performance benchmark mapping](PERFORMANCE_EXPECTATIONS.md)
- [Vector manager API](../../include/index/vector_index.h)
- [GPU backend policy and statistics](../../include/index/gpu_vector_index.h)
- [ANN result/route metadata](../../include/index/ann_frontdoor.h)
- [Production deployment requirements](PRODUCTION_REQUIREMENTS.md)
