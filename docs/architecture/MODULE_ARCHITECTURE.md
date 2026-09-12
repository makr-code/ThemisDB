# Cross-Module Architecture Inventory

## Scope

- This document inventories all **72** top-level `src/` module paths in the current repository clone.
- Code-footprint counts come from colocated C/C++ files under `src/<module>/`; direct dependency counts come from module-prefix `#include` edges found under `src/**` and `include/**`.
- The grouping below is a cross-module review aid; it does not replace module-local contracts in `src/<module>/ARCHITECTURE.md` and `src/<module>/ROADMAP.md`.

## Layer Summary

| Layer | Modules | Notes |
|---|---:|---|
| Core Infrastructure | 5 | source-backed grouping for cross-module review |
| Storage & Persistence | 7 | source-backed grouping for cross-module review |
| Query & Processing | 9 | source-backed grouping for cross-module review |
| Index & Search | 3 | includes docs-only module paths |
| LLM & AI | 18 | includes docs-only module paths |
| Server & API | 5 | source-backed grouping for cross-module review |
| Security & Auth | 4 | source-backed grouping for cross-module review |
| Distributed Systems | 9 | source-backed grouping for cross-module review |
| Observability & Hardening | 3 | source-backed grouping for cross-module review |
| Data Integration | 7 | source-backed grouping for cross-module review |
| Extensibility & Lifecycle | 1 | source-backed grouping for cross-module review |
| Source-root Docs & Analysis | 1 | non-runtime support path under `src/` |

## Core Infrastructure

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `base` | 10 | 4 | 0 | runtime code-bearing path | `src/base/ARCHITECTURE.md` · `src/base/ROADMAP.md` · `include/base/` |
| `config` | 6 | 1 | 5 | runtime code-bearing path | `src/config/ARCHITECTURE.md` · `src/config/ROADMAP.md` · `include/config/` |
| `core` | 12 | 9 | 7 | runtime code-bearing path | `src/core/ARCHITECTURE.md` · `src/core/ROADMAP.md` · `include/core/` |
| `themis` | 11 | 9 | 23 | high-integration runtime hub | `src/themis/ARCHITECTURE.md` · `src/themis/ROADMAP.md` · `include/themis/` |
| `utils` | 48 | 8 | 51 | high-integration runtime hub | `src/utils/ARCHITECTURE.md` · `src/utils/ROADMAP.md` · `include/utils/` |

## Storage & Persistence

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `access_model` | 6 | 1 | 2 | runtime code-bearing path | `src/access_model/ARCHITECTURE.md` · `src/access_model/ROADMAP.md` · `include/access_model/` |
| `cache` | 17 | 8 | 5 | runtime code-bearing path | `src/cache/ARCHITECTURE.md` · `src/cache/ROADMAP.md` · `include/cache/` |
| `metadata` | 12 | 6 | 7 | runtime code-bearing path | `src/metadata/ARCHITECTURE.md` · `src/metadata/ROADMAP.md` · `include/metadata/` |
| `storage` | 64 | 13 | 35 | high-integration runtime hub | `src/storage/ARCHITECTURE.md` · `src/storage/ROADMAP.md` · `include/storage/` |
| `temporal` | 15 | 1 | 2 | runtime code-bearing path | `src/temporal/ARCHITECTURE.md` · `src/temporal/ROADMAP.md` · `include/temporal/` |
| `timeseries` | 26 | 2 | 4 | runtime code-bearing path | `src/timeseries/ARCHITECTURE.md` · `src/timeseries/ROADMAP.md` · `include/timeseries/` |
| `tensor` | 24 | 5 | 4 | runtime code-bearing path | `src/tensor/ARCHITECTURE.md` · `src/tensor/ROADMAP.md` · `include/tensor/` |

## Query & Processing

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `analytics` | 30 | 7 | 10 | high-integration runtime hub | `src/analytics/ARCHITECTURE.md` · `src/analytics/ROADMAP.md` · `include/analytics/` |
| `aql` | 25 | 11 | 4 | high-integration runtime hub | `src/aql/ARCHITECTURE.md` · `src/aql/ROADMAP.md` · `include/aql/` |
| `execution` | 2 | 0 | 0 | thin code-bearing path | `src/execution/ARCHITECTURE.md` · `src/execution/ROADMAP.md` · `include/execution/` |
| `graph` | 17 | 10 | 4 | high-integration runtime hub | `src/graph/ARCHITECTURE.md` · `src/graph/ROADMAP.md` · `include/graph/` |
| `process` | 25 | 5 | 0 | runtime code-bearing path | `src/process/ARCHITECTURE.md` · `src/process/ROADMAP.md` · `include/process/` |
| `query` | 73 | 17 | 15 | high-integration runtime hub | `src/query/ARCHITECTURE.md` · `src/query/ROADMAP.md` · `include/query/` |
| `retrieval` | 2 | 0 | 0 | thin code-bearing path | `src/retrieval/ARCHITECTURE.md` · `src/retrieval/ROADMAP.md` · `include/retrieval/` |
| `scheduler` | 10 | 9 | 3 | runtime code-bearing path | `src/scheduler/ARCHITECTURE.md` · `src/scheduler/ROADMAP.md` · `include/scheduler/` |
| `search` | 22 | 9 | 0 | runtime code-bearing path | `src/search/ARCHITECTURE.md` · `src/search/ROADMAP.md` · `include/search/` |

## Index & Search

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `geo` | 25 | 5 | 6 | runtime code-bearing path | `src/geo/ARCHITECTURE.md` · `src/geo/ROADMAP.md` · `include/geo/` |
| `index` | 44 | 11 | 25 | high-integration runtime hub | `src/index/ARCHITECTURE.md` · `src/index/ROADMAP.md` · `include/index/` |
| `vector_search` | 0 | 0 | 0 | docs-only module path; no colocated C/C++ implementation files | `src/vector_search/ARCHITECTURE.md` · `src/vector_search/ROADMAP.md` |

## LLM & AI

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `acceleration` | 29 | 6 | 8 | runtime code-bearing path | `src/acceleration/ARCHITECTURE.md` · `src/acceleration/ROADMAP.md` · `include/acceleration/` |
| `ai` | 2 | 4 | 0 | thin code-bearing path | `src/ai/ARCHITECTURE.md` · `src/ai/ROADMAP.md` · `include/ai/` |
| `distributed_tensor` | 37 | 3 | 0 | runtime code-bearing path | `src/distributed_tensor/ARCHITECTURE.md` · `src/distributed_tensor/ROADMAP.md` · `include/distributed_tensor/` |
| `ethics_ai` | 30 | 4 | 3 | runtime code-bearing path | `src/ethics_ai/ARCHITECTURE.md` · `src/ethics_ai/ROADMAP.md` · `include/ethics_ai/` |
| `evaluation` | 14 | 0 | 0 | runtime code-bearing path | `src/evaluation/ARCHITECTURE.md` · `src/evaluation/ROADMAP.md` · `include/evaluation/` |
| `gpu` | 44 | 3 | 1 | runtime code-bearing path | `src/gpu/ARCHITECTURE.md` · `src/gpu/ROADMAP.md` · `include/gpu/` |
| `image_analysis` | 2 | 1 | 0 | thin code-bearing path | `src/image_analysis/ARCHITECTURE.md` · `src/image_analysis/ROADMAP.md` · `src/image_analysis/` |
| `llama_cpp` | 8 | 3 | 1 | runtime code-bearing path | `src/llama_cpp/ARCHITECTURE.md` · `src/llama_cpp/ROADMAP.md` · `include/llama_cpp/` |
| `llm` | 189 | 22 | 18 | high-integration runtime hub | `src/llm/ARCHITECTURE.md` · `src/llm/ROADMAP.md` · `include/llm/` |
| `llm_streaming` | 0 | 0 | 0 | docs-only module path; no colocated C/C++ implementation files | `src/llm_streaming/ARCHITECTURE.md` · `src/llm_streaming/ROADMAP.md` |
| `llm_wiki` | 9 | 4 | 0 | runtime code-bearing path | `src/llm_wiki/ARCHITECTURE.md` · `src/llm_wiki/ROADMAP.md` · `include/llm_wiki/` |
| `onnx_clip` | 2 | 1 | 0 | thin code-bearing path | `src/onnx_clip/ARCHITECTURE.md` · `src/onnx_clip/ROADMAP.md` · `include/onnx_clip/` |
| `prompt_engineering` | 36 | 6 | 3 | runtime code-bearing path | `src/prompt_engineering/ARCHITECTURE.md` · `src/prompt_engineering/ROADMAP.md` · `include/prompt_engineering/` |
| `rag` | 71 | 16 | 7 | high-integration runtime hub | `src/rag/ARCHITECTURE.md` · `src/rag/ROADMAP.md` · `include/rag/` |
| `stable_diffusion` | 7 | 2 | 0 | runtime code-bearing path | `src/stable_diffusion/ARCHITECTURE.md` · `src/stable_diffusion/ROADMAP.md` · `include/stable_diffusion/` |
| `training` | 16 | 8 | 1 | runtime code-bearing path | `src/training/ARCHITECTURE.md` · `src/training/ROADMAP.md` · `include/training/` |
| `voice` | 24 | 3 | 1 | runtime code-bearing path | `src/voice/ARCHITECTURE.md` · `src/voice/ROADMAP.md` · `include/voice/` |
| `whisper` | 7 | 1 | 0 | runtime code-bearing path | `src/whisper/ARCHITECTURE.md` · `src/whisper/ROADMAP.md` · `include/whisper/` |

## Server & API

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `api` | 11 | 9 | 3 | runtime code-bearing path | `src/api/ARCHITECTURE.md` · `src/api/ROADMAP.md` · `include/api/` |
| `network` | 30 | 9 | 2 | runtime code-bearing path | `src/network/ARCHITECTURE.md` · `src/network/ROADMAP.md` · `include/network/` |
| `rpc_grpc` | 4 | 1 | 0 | runtime code-bearing path | `src/rpc_grpc/ARCHITECTURE.md` · `src/rpc_grpc/ROADMAP.md` · `include/rpc_grpc/` |
| `scraper` | 10 | 1 | 0 | runtime code-bearing path | `src/scraper/ARCHITECTURE.md` · `src/scraper/ROADMAP.md` · `include/scraper/` |
| `server` | 123 | 35 | 4 | high-integration runtime hub | `src/server/ARCHITECTURE.md` · `src/server/ROADMAP.md` · `include/server/` |

## Security & Auth

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `auth` | 37 | 3 | 3 | runtime code-bearing path | `src/auth/ARCHITECTURE.md` · `src/auth/ROADMAP.md` · `include/auth/` |
| `governance` | 35 | 4 | 5 | runtime code-bearing path | `src/governance/ARCHITECTURE.md` · `src/governance/ROADMAP.md` · `include/governance/` |
| `security` | 52 | 7 | 20 | high-integration runtime hub | `src/security/ARCHITECTURE.md` · `src/security/ROADMAP.md` · `include/security/` |
| `user_storage_encrypted` | 5 | 1 | 0 | runtime code-bearing path | `src/user_storage_encrypted/ARCHITECTURE.md` · `src/user_storage_encrypted/ROADMAP.md` · `include/user_storage_encrypted/` |

## Distributed Systems

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `cdc` | 13 | 3 | 9 | runtime code-bearing path | `src/cdc/ARCHITECTURE.md` · `src/cdc/ROADMAP.md` · `include/cdc/` |
| `distributed_knowledge` | 5 | 2 | 8 | runtime code-bearing path | `src/distributed_knowledge/ARCHITECTURE.md` · `src/distributed_knowledge/ROADMAP.md` · `include/distributed_knowledge/` |
| `failover` | 4 | 3 | 0 | runtime code-bearing path | `src/failover/ARCHITECTURE.md` · `src/failover/ROADMAP.md` · `include/failover/` |
| `maintenance` | 3 | 6 | 1 | thin code-bearing path | `src/maintenance/ARCHITECTURE.md` · `src/maintenance/ROADMAP.md` · `include/maintenance/` |
| `projects` | 7 | 2 | 1 | runtime code-bearing path | `src/projects/ARCHITECTURE.md` · `src/projects/ROADMAP.md` · `include/projects/` |
| `replication` | 13 | 2 | 2 | runtime code-bearing path | `src/replication/ARCHITECTURE.md` · `src/replication/ROADMAP.md` · `include/replication/` |
| `sharding` | 95 | 6 | 14 | high-integration runtime hub | `src/sharding/ARCHITECTURE.md` · `src/sharding/ROADMAP.md` · `include/sharding/` |
| `transaction` | 20 | 8 | 6 | runtime code-bearing path | `src/transaction/ARCHITECTURE.md` · `src/transaction/ROADMAP.md` · `include/transaction/` |
| `updates` | 25 | 5 | 1 | runtime code-bearing path | `src/updates/ARCHITECTURE.md` · `src/updates/ROADMAP.md` · `include/updates/` |

## Observability & Hardening

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `chaos` | 1 | 0 | 0 | thin code-bearing path | `src/chaos/ARCHITECTURE.md` · `src/chaos/ROADMAP.md` · `include/chaos/` |
| `observability` | 31 | 5 | 17 | high-integration runtime hub | `src/observability/ARCHITECTURE.md` · `src/observability/ROADMAP.md` · `include/observability/` |
| `performance` | 31 | 3 | 8 | runtime code-bearing path | `src/performance/ARCHITECTURE.md` · `src/performance/ROADMAP.md` · `include/performance/` |

## Data Integration

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `chimera` | 7 | 3 | 0 | runtime code-bearing path | `src/chimera/ARCHITECTURE.md` · `src/chimera/ROADMAP.md` · `include/chimera/` |
| `content` | 47 | 8 | 5 | runtime code-bearing path | `src/content/ARCHITECTURE.md` · `src/content/ROADMAP.md` · `include/content/` |
| `document` | 2 | 3 | 2 | thin code-bearing path | `src/document/ARCHITECTURE.md` · `src/document/ROADMAP.md` · `include/document/` |
| `exporters` | 16 | 6 | 2 | runtime code-bearing path | `src/exporters/ARCHITECTURE.md` · `src/exporters/ROADMAP.md` · `include/exporters/` |
| `importers` | 48 | 3 | 2 | runtime code-bearing path | `src/importers/ARCHITECTURE.md` · `src/importers/ROADMAP.md` · `include/importers/` |
| `ingestion` | 38 | 6 | 6 | runtime code-bearing path | `src/ingestion/ARCHITECTURE.md` · `src/ingestion/ROADMAP.md` · `include/ingestion/` |
| `toolbox` | 11 | 5 | 2 | runtime code-bearing path | `src/toolbox/ARCHITECTURE.md` · `src/toolbox/ROADMAP.md` · `include/toolbox/` |

## Extensibility & Lifecycle

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `plugins` | 10 | 5 | 14 | high-integration runtime hub | `src/plugins/ARCHITECTURE.md` · `src/plugins/ROADMAP.md` · `include/plugins/` |

## Source-root Docs & Analysis

| Module | C/C++ files | Direct module deps | Direct consumers | Current footprint signal | Primary anchors |
|---|---:|---:|---:|---|---|
| `ai_working` | 0 | 0 | 0 | source-root evidence and working-notes path | `src/ai_working/` |
