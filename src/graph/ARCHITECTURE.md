---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: architecture-governance
Canonical source references:
  - ../../include/graph/
  - ../../src/graph/
  - ../../include/index/graph_index.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Architecture - Graph Module

<!-- Historical source-structure validation: 2026-09-09; see the status notice above. -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS.md -->

> **Status notice:** The 2026-06 scanner report below is historical and not current audit
> evidence. Dependencies and integration statements further down are summaries; verify ownership
> and exact contracts in current headers/source. See [`API.md`](API.md) for the current boundary
> summary.

## Historical L0 Verification Report (2026-06-25; not current audit evidence)

> The following values and conclusions reproduce an older scanner summary; they were not
> independently revalidated during this documentation update and do not establish readiness.

**Reported risk level: INFO — 0 verified gaps; scanner classified findings as defensive patterns**

| Dimension | Finding | Impact |
|-----------|---------|--------|
| **Completeness** | 9 initial detections; 8 GUARDED_STUB + 1 FALSE_POSITIVE (reported) | Historical scanner conclusion; not revalidated |
| **Severity** | All downgraded from CRITICAL/HIGH → INFO (reported) | Historical scanner conclusion; not revalidated |
| **Release Status** | Marked production-ready by the snapshot | Not a current release decision |

**Pattern Examples:**
- `explain_plan::toDot()` (line 68): Empty plan → empty DOT output (correct semantics)
- `ontology_manager::parseString()` (line 73): Parse error → empty string (documented behavior)
- `rotate_completion::entityEmbedding()` (line 95): Untrained model → empty vector (defensive guard)

**Analysis recorded by the snapshot**: Findings were described as defensive patterns. This
analysis was not rechecked against current code. An archived copy of the historical scan is
available at
[`gap_scanner_verified_graph.json`](../../docs/ARCHIVED/ai-working-history/gap_scanner_verified_graph.json);
it is not current audit evidence.

---

## Overview

The graph module composes planning, traversal, constraints, and advanced graph-processing features into a bounded execution subsystem for ThemisDB.

## Main Execution Planes

1. Planning and optimization plane
- cost-based query planning and algorithm selection
- rewrite and explain-plan generation surfaces

2. Traversal and execution plane
- BFS/DFS/shortest-path style execution and parallel traversal
- distributed and GPU-assisted traversal routes with fallback controls

3. Semantic and reasoning plane
- ontology-aware constraints and knowledge-graph reasoning behavior
- refresh and watermark flows for evolving graph states

4. Tensor graph utility plane
- tensor-fingerprint graph similarity operations
- deduplication/persistence support for tensor graph data

## Core Contracts

| Contract | Behavior |
|---|---|
| planning contract | deterministic plan generation under explicit constraints |
| traversal contract | bounded traversal semantics across local/parallel/distributed routes |
| semantic contract | explicit ontology/reasoning validation and explainability surfaces |
| tensor utility contract | deterministic fingerprint similarity and dedup behavior |

## Failure Semantics

- invalid constraints or inconsistent semantic input fail with explicit outcomes.
- unsupported/degraded acceleration paths (GPU/distributed) degrade via bounded fallback behavior.
- execution errors surface deterministically rather than silently degrading correctness.

## Sourcecode Verification (Module: graph/architecture)

> The dated verification notes in this section record an earlier review. They are not a current
> source audit or proof of the listed integration guarantees.

- Verified files:
  - src/graph/graph_query_optimizer.cpp
  - src/graph/path_constraints.cpp
  - src/graph/parallel_traversal.cpp
  - src/graph/distributed_graph.cpp
  - src/graph/gpu_traversal.cpp
  - src/graph/graph_query_rewriter.cpp
  - src/graph/explain_plan.cpp
  - src/graph/knowledge_graph_reasoner.cpp
  - src/graph/tensor_fingerprint_graph.cpp
- Verified architecture claims:
  - explicit planning/traversal/semantic/tensor planes
  - deterministic failure and fallback behavior boundaries
  - module-local ownership of graph orchestration surfaces
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`

## Module Dependencies

> The tables below are an older integration inventory. They are not a complete or current
> dependency graph; validate an edge against the named interface and its current owner.

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/` | Persists and retrieves graph vertices, edges, and property data |
| index | `include/index/` (spatial_index.h and related) | Spatial and attribute indexing for efficient graph traversal lookups |
| query | `include/query/` (graph_functions, path-query planning) | Receives path-query plans; exposes graph sub-expression cost estimates |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/graph_api_handler.h` | Exposes traversal, shortest-path, and ontology query APIs over HTTP/wire |
| query | `include/graph/` (graph_functions) | Query planner issues graph sub-queries and receives plan/cost estimates |
| llm | `include/graph/knowledge_graph_reasoner.h` | LLM reasoning layer uses graph ontology paths for context grounding |
| rag | `include/graph/` | RAG module uses graph retrieval for graph-RAG context assembly |

## Integration Points

### Critical Integration: Storage Graph Persistence
**Files:** `src/graph/graph_query_optimizer.cpp`, `src/graph/distributed_graph.cpp` ↔ `storage/`
**Contract:** All vertex/edge persistence and reads go through storage interfaces; graph module does not own raw block or WAL surfaces.
**Thread Safety:** Graph traversal workers use read transactions; mutations are serialised through storage write paths.

### Critical Integration: Query Graph Functions
**Files:** `src/graph/graph_query_rewriter.cpp` ↔ `query/graph_functions`
**Contract:** Query engine calls graph sub-expression resolvers; graph module returns typed result sets compatible with the query columnar row format.
**Thread Safety:** Plan objects are immutable after handoff; graph executor owns its traversal state independently.

### Critical Integration: LLM Knowledge Graph Reasoner
**Files:** `src/graph/knowledge_graph_reasoner.cpp` ↔ `llm/`
**Contract:** LLM requests ontology path expansions; graph module returns ranked entity / relation lists; callers must not mutate returned lists.
**Thread Safety:** Reasoner caches under an internal read-write lock; concurrent reads proceed in parallel.