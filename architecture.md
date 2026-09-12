# ThemisDB Architecture

> **Auto-generated** — do not edit manually.
> Source: `tools/architecture-generator/generate_architecture.py`
> Generated: `2026-09-12T08:01:12.632725+00:00`

## Statistics

| Metric | Value |
|--------|-------|
| Total modules | 68 |
| Total relationships | 47 |
| Modules with public plugin | 10 |
| Modules with private plugin | 4 |
| Modules with API contracts | 6 |
| Documentation sources scanned | 1170 |
| LLM Wiki source files | 7718 |
| LLM Wiki module/API entries | 80 |
| LLM Wiki generated | `2026-09-07T03:02:22+00:00` |
| ai_working artifacts | 64 |

## Module Architecture Diagram

```mermaid
flowchart TB
    %% Vertical-first layout and GitHub-friendly styling
    subgraph T0["T0: Trusted Core"]
        direction TB
        mod_base["base"]
        mod_core["core"]
        mod_plugins["plugins"]
        mod_themis["themis"]
        mod_utils["utils"]
    end
    subgraph T1["T1: Engine (Query/Storage/Index)"]
        direction TB
        mod_aql["aql"]
        mod_cache["cache"]
        mod_execution["execution"]
        mod_index["index"]
        mod_metadata["metadata"]
        mod_query["query"]
        mod_storage["storage"]
    end
    subgraph T3["T3: Infrastructure & Governance"]
        direction TB
        mod_acceleration["acceleration"]
        mod_access_model["access_model"]
        mod_ai["ai"]
        mod_analytics["analytics"]
        mod_api["api"]
        mod_auth["auth"]
        mod_cdc["cdc"]
        mod_chaos["chaos"]
        mod_chimera["chimera"]
        mod_config["config"]
        mod_content["content"]
        mod_distributed_knowledge["distributed_knowledge"]
        mod_distributed_tensor["distributed_tensor"]
        mod_document["document"]
        mod_ethics_ai["ethics_ai 🔒"]
        mod_evaluation["evaluation"]
        mod_exporters["exporters ✅"]
        mod_failover["failover"]
        mod_geo["geo ✅"]
        mod_governance["governance"]
        mod_gpu["gpu"]
        mod_graph["graph"]
        mod_importers["importers 🔒"]
        mod_ingestion["ingestion"]
        mod_llama_cpp["llama_cpp ✅"]
        mod_llm["llm"]
        mod_llm_wiki["llm_wiki 🔒"]
        mod_maintenance["maintenance"]
        mod_network["network"]
        mod_observability["observability"]
        mod_onnx_clip["onnx_clip"]
        mod_performance["performance"]
        mod_process["process"]
        mod_projects["projects"]
        mod_prompt_engineering["prompt_engineering"]
        mod_rag["rag"]
        mod_replication["replication"]
        mod_retrieval["retrieval"]
        mod_rpc_grpc["rpc_grpc"]
        mod_scheduler["scheduler"]
        mod_scraper["scraper ✅"]
        mod_search["search"]
        mod_security["security"]
        mod_server["server"]
        mod_sharding["sharding"]
        mod_stable_diffusion["stable_diffusion ✅"]
        mod_temporal["temporal"]
        mod_tensor["tensor"]
        mod_timeseries["timeseries ✅"]
        mod_toolbox["toolbox"]
        mod_training["training"]
        mod_transaction["transaction"]
        mod_updates["updates"]
        mod_user_storage_encrypted["user_storage_encrypted 🔒"]
        mod_voice["voice"]
        mod_whisper["whisper ✅"]
    end

    classDef tierT0 fill:#EAF2FF,stroke:#1D4ED8,color:#0F172A,stroke-width:1.2px;
    classDef tierT1 fill:#ECFDF3,stroke:#15803D,color:#0F172A,stroke-width:1.2px;
    classDef tierT3 fill:#FFF7ED,stroke:#C2410C,color:#0F172A,stroke-width:1.2px;
    classDef publicPlugin fill:#E0F2FE,stroke:#0369A1,color:#0F172A,stroke-dasharray: 3 2;
    classDef privatePlugin fill:#FCE7F3,stroke:#9D174D,color:#0F172A,stroke-dasharray: 2 2;
    linkStyle default stroke:#64748B,stroke-width:1.1px,opacity:0.85;
    class mod_base,mod_core,mod_plugins,mod_themis,mod_utils tierT0;
    class mod_aql,mod_cache,mod_execution,mod_index,mod_metadata,mod_query,mod_storage tierT1;
    class mod_acceleration,mod_access_model,mod_ai,mod_analytics,mod_api,mod_auth,mod_cdc,mod_chaos,mod_chimera,mod_config,mod_content,mod_distributed_knowledge,mod_distributed_tensor,mod_document,mod_ethics_ai,mod_evaluation,mod_exporters,mod_failover,mod_geo,mod_governance,mod_gpu,mod_graph,mod_importers,mod_ingestion,mod_llama_cpp,mod_llm,mod_llm_wiki,mod_maintenance,mod_network,mod_observability,mod_onnx_clip,mod_performance,mod_process,mod_projects,mod_prompt_engineering,mod_rag,mod_replication,mod_retrieval,mod_rpc_grpc,mod_scheduler,mod_scraper,mod_search,mod_security,mod_server,mod_sharding,mod_stable_diffusion,mod_temporal,mod_tensor,mod_timeseries,mod_toolbox,mod_training,mod_transaction,mod_updates,mod_user_storage_encrypted,mod_voice,mod_whisper tierT3;
    class mod_exporters,mod_geo,mod_llama_cpp,mod_scraper,mod_stable_diffusion,mod_timeseries,mod_whisper publicPlugin;
    class mod_ethics_ai,mod_importers,mod_llm_wiki,mod_user_storage_encrypted privatePlugin;

    mod_plugins -->|"plugin base"| mod_base
    mod_themis -->|"root aggregation"| mod_core
    mod_themis -->|"plugin loader"| mod_plugins
    mod_utils -->|"utilities"| mod_base
    mod_aql -->|"parses/plans"| mod_core
    mod_cache -->|"caches"| mod_execution
    mod_execution -->|"uses"| mod_index
    mod_execution -->|"executes"| mod_query
    mod_execution -->|"reads/writes"| mod_storage
    mod_index -->|"persists"| mod_storage
    mod_metadata -->|"schema store"| mod_storage
    mod_query -->|"optimizes"| mod_aql
    mod_config -->|"config bootstrap"| mod_core
    mod_observability -->|"metrics hooks"| mod_core
    mod_process -->|"lifecycle"| mod_core
    mod_security -->|"crypto primitives"| mod_core
    mod_transaction -->|"MVCC"| mod_core
    mod_analytics -->|"reads data"| mod_storage
    mod_content -->|"content store"| mod_storage
    mod_rag -->|"knowledge store"| mod_storage
    mod_replication -->|"replica I/O"| mod_storage
    mod_retrieval -->|"vector search"| mod_index
    mod_search -->|"fulltext index"| mod_index
    mod_server -->|"dispatches"| mod_execution
    mod_sharding -->|"partitions"| mod_storage
    mod_training -->|"model storage"| mod_storage
    mod_transaction -->|"data commit"| mod_storage
    mod_acceleration -->|"GPU dispatch"| mod_gpu
    mod_ai -->|"orchestrates"| mod_llm
    mod_ai -->|"semantic search"| mod_retrieval
    mod_api -->|"authenticates"| mod_auth
    mod_api -->|"routes"| mod_server
    mod_auth -->|"primitives"| mod_security
    mod_distributed_knowledge -->|"knowledge graph"| mod_graph
    mod_distributed_knowledge -->|"distributed"| mod_replication
    mod_governance -->|"policy check"| mod_auth
    mod_governance -->|"audit events"| mod_observability
    mod_llm -->|"local inference"| mod_llama_cpp
    mod_llm_wiki -->|"wiki retrieval"| mod_llm
    mod_llm_wiki -->|"provenance"| mod_rag
    mod_network -->|"transport"| mod_server
    mod_rag -->|"generates"| mod_llm
    mod_rag -->|"retrieves"| mod_retrieval
    mod_search -->|"semantic search"| mod_retrieval
    mod_sharding -->|"replicates shards"| mod_replication
    mod_training -->|"fine-tunes"| mod_llm
    mod_transaction -->|"distributes"| mod_replication

    linkStyle 4 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 12 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 13 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 14 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 15 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 16 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 17 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 18 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 19 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 20 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 21 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 22 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 23 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 24 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 25 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;
    linkStyle 26 stroke:#1E293B,stroke-width:1.3px,opacity:0.92;

    click mod_base href "https://github.com/makr-code/ThemisDB/blob/develop/src/base/ROADMAP.md" "themis::resource — module documentation" _blank
    click mod_core href "https://github.com/makr-code/ThemisDB/blob/develop/src/core/ROADMAP.md" "themis::core — module documentation" _blank
    click mod_plugins href "https://github.com/makr-code/ThemisDB/blob/develop/src/plugins/ROADMAP.md" "themis::plugins — module documentation" _blank
    click mod_themis href "https://github.com/makr-code/ThemisDB/blob/develop/src/themis/ROADMAP.md" "themis — module documentation" _blank
    click mod_utils href "https://github.com/makr-code/ThemisDB/blob/develop/src/utils/ROADMAP.md" "themis::utils — module documentation" _blank
    click mod_aql href "https://github.com/makr-code/ThemisDB/blob/develop/src/aql/ROADMAP.md" "themis::aql — module documentation" _blank
    click mod_cache href "https://github.com/makr-code/ThemisDB/blob/develop/src/cache/ROADMAP.md" "themis::cache — module documentation" _blank
    click mod_execution href "https://github.com/makr-code/ThemisDB/blob/develop/src/execution/ROADMAP.md" "themis::execution — module documentation" _blank
    click mod_index href "https://github.com/makr-code/ThemisDB/wiki/Index-Module" "themis::index — module documentation" _blank
    click mod_metadata href "https://github.com/makr-code/ThemisDB/blob/develop/src/metadata/ROADMAP.md" "themis::metadata — module documentation" _blank
    click mod_query href "https://github.com/makr-code/ThemisDB/blob/develop/src/query/ROADMAP.md" "themis::query — module documentation" _blank
    click mod_storage href "https://github.com/makr-code/ThemisDB/wiki/Storage-Module" "themis::storage — module documentation" _blank
    click mod_acceleration href "https://github.com/makr-code/ThemisDB/wiki/Acceleration-Module" "themis::acceleration — module documentation" _blank
    click mod_access_model href "https://github.com/makr-code/ThemisDB/blob/develop/src/access_model/ROADMAP.md" "themis::access_model — module documentation" _blank
    click mod_ai href "https://github.com/makr-code/ThemisDB/blob/develop/src/ai/ROADMAP.md" "themis::ai — module documentation" _blank
    click mod_analytics href "https://github.com/makr-code/ThemisDB/blob/develop/src/analytics/ROADMAP.md" "themis::analytics — module documentation" _blank
    click mod_api href "https://github.com/makr-code/ThemisDB/wiki/API-Module" "themis::api — module documentation" _blank
    click mod_auth href "https://github.com/makr-code/ThemisDB/wiki/Auth-Module" "themis::auth — module documentation" _blank
    click mod_cdc href "https://github.com/makr-code/ThemisDB/blob/develop/src/cdc/ROADMAP.md" "themis::cdc — module documentation" _blank
    click mod_chaos href "https://github.com/makr-code/ThemisDB/blob/develop/src/chaos/ROADMAP.md" "themis::chaos — module documentation" _blank
    click mod_chimera href "https://github.com/makr-code/ThemisDB/blob/develop/src/chimera/ROADMAP.md" "themis::chimera — module documentation" _blank
    click mod_config href "https://github.com/makr-code/ThemisDB/blob/develop/src/config/ROADMAP.md" "themis::config — module documentation" _blank
    click mod_content href "https://github.com/makr-code/ThemisDB/blob/develop/src/content/ROADMAP.md" "themis::content — module documentation" _blank
    click mod_distributed_knowledge href "https://github.com/makr-code/ThemisDB/blob/develop/src/distributed_knowledge/ROADMAP.md" "themis::distributed_knowledge — module documentation" _blank
    click mod_distributed_tensor href "https://github.com/makr-code/ThemisDB/blob/develop/src/distributed_tensor/ROADMAP.md" "themis::distributed_tensor — module documentation" _blank
    click mod_document href "https://github.com/makr-code/ThemisDB/blob/develop/src/document/ROADMAP.md" "themis::document — module documentation" _blank
    click mod_ethics_ai href "https://github.com/makr-code/ThemisDB/blob/develop/src/ethics_ai/ROADMAP.md" "themis::ethics_ai — module documentation" _blank
    click mod_evaluation href "https://github.com/makr-code/ThemisDB/blob/develop/src/evaluation/ROADMAP.md" "themis::evaluation — module documentation" _blank
    click mod_exporters href "https://github.com/makr-code/ThemisDB/blob/develop/src/exporters/ROADMAP.md" "themis::exporters — module documentation" _blank
    click mod_failover href "https://github.com/makr-code/ThemisDB/blob/develop/src/failover/ROADMAP.md" "themis::failover — module documentation" _blank
    click mod_geo href "https://github.com/makr-code/ThemisDB/blob/develop/src/geo/ROADMAP.md" "themis::geo — module documentation" _blank
    click mod_governance href "https://github.com/makr-code/ThemisDB/blob/develop/src/governance/ROADMAP.md" "themis::governance — module documentation" _blank
    click mod_gpu href "https://github.com/makr-code/ThemisDB/blob/develop/src/gpu/ROADMAP.md" "themis::gpu — module documentation" _blank
    click mod_graph href "https://github.com/makr-code/ThemisDB/wiki/Graph-Module" "themis::graph — module documentation" _blank
    click mod_importers href "https://github.com/makr-code/ThemisDB/blob/develop/src/importers/ROADMAP.md" "themis::importers — module documentation" _blank
    click mod_ingestion href "https://github.com/makr-code/ThemisDB/blob/develop/src/ingestion/ROADMAP.md" "themis::ingestion — module documentation" _blank
    click mod_llama_cpp href "https://github.com/makr-code/ThemisDB/blob/develop/src/llama_cpp/ROADMAP.md" "themis::llama_cpp — module documentation" _blank
    click mod_llm href "https://github.com/makr-code/ThemisDB/wiki/LLM-Module" "themis::llm — module documentation" _blank
    click mod_llm_wiki href "https://github.com/makr-code/ThemisDB/wiki/LLM-Wiki" "themis::llm_wiki — module documentation" _blank
    click mod_maintenance href "https://github.com/makr-code/ThemisDB/blob/develop/src/maintenance/ROADMAP.md" "themis::maintenance — module documentation" _blank
    click mod_network href "https://github.com/makr-code/ThemisDB/blob/develop/src/network/ROADMAP.md" "themis::network — module documentation" _blank
    click mod_observability href "https://github.com/makr-code/ThemisDB/wiki/Observability-Module" "themis::observability — module documentation" _blank
    click mod_onnx_clip href "https://github.com/makr-code/ThemisDB/blob/develop/src/onnx_clip/ROADMAP.md" "themis::onnx_clip — module documentation" _blank
    click mod_performance href "https://github.com/makr-code/ThemisDB/blob/develop/src/performance/ROADMAP.md" "themis::performance — module documentation" _blank
    click mod_process href "https://github.com/makr-code/ThemisDB/blob/develop/src/process/ROADMAP.md" "themis::process — module documentation" _blank
    click mod_projects href "https://github.com/makr-code/ThemisDB/blob/develop/src/projects/ROADMAP.md" "themis::projects — module documentation" _blank
    click mod_prompt_engineering href "https://github.com/makr-code/ThemisDB/blob/develop/src/prompt_engineering/ROADMAP.md" "themis::prompt_engineering — module documentation" _blank
    click mod_rag href "https://github.com/makr-code/ThemisDB/wiki/RAG-Module" "themis::rag — module documentation" _blank
    click mod_replication href "https://github.com/makr-code/ThemisDB/wiki/Replication-Module" "themis::replication — module documentation" _blank
    click mod_retrieval href "https://github.com/makr-code/ThemisDB/blob/develop/src/retrieval/ROADMAP.md" "themis::retrieval — module documentation" _blank
    click mod_rpc_grpc href "https://github.com/makr-code/ThemisDB/blob/develop/src/rpc_grpc/ROADMAP.md" "themis::rpc_grpc — module documentation" _blank
    click mod_scheduler href "https://github.com/makr-code/ThemisDB/blob/develop/src/scheduler/ROADMAP.md" "themis::scheduler — module documentation" _blank
    click mod_scraper href "https://github.com/makr-code/ThemisDB/blob/develop/src/scraper/ROADMAP.md" "themis::scraper — module documentation" _blank
    click mod_search href "https://github.com/makr-code/ThemisDB/blob/develop/src/search/ROADMAP.md" "themis::search — module documentation" _blank
    click mod_security href "https://github.com/makr-code/ThemisDB/wiki/Security-Module" "themis::security — module documentation" _blank
    click mod_server href "https://github.com/makr-code/ThemisDB/blob/develop/src/server/ROADMAP.md" "themis::server — module documentation" _blank
    click mod_sharding href "https://github.com/makr-code/ThemisDB/wiki/Sharding-Module" "themis::sharding — module documentation" _blank
    click mod_stable_diffusion href "https://github.com/makr-code/ThemisDB/blob/develop/src/stable_diffusion/ROADMAP.md" "themis::stable_diffusion — module documentation" _blank
    click mod_temporal href "https://github.com/makr-code/ThemisDB/blob/develop/src/temporal/ROADMAP.md" "themis::temporal — module documentation" _blank
    click mod_tensor href "https://github.com/makr-code/ThemisDB/blob/develop/src/tensor/ROADMAP.md" "themis::tensor — module documentation" _blank
    click mod_timeseries href "https://github.com/makr-code/ThemisDB/blob/develop/src/timeseries/ROADMAP.md" "themis::timeseries — module documentation" _blank
    click mod_toolbox href "https://github.com/makr-code/ThemisDB/blob/develop/src/toolbox/ROADMAP.md" "themis::toolbox — module documentation" _blank
    click mod_training href "https://github.com/makr-code/ThemisDB/blob/develop/src/training/ROADMAP.md" "themis::training — module documentation" _blank
    click mod_transaction href "https://github.com/makr-code/ThemisDB/wiki/Transaction-Module" "themis::transaction — module documentation" _blank
    click mod_updates href "https://github.com/makr-code/ThemisDB/blob/develop/src/updates/ROADMAP.md" "themis::updates — module documentation" _blank
    click mod_user_storage_encrypted href "https://github.com/makr-code/ThemisDB/blob/develop/src/user_storage_encrypted/ROADMAP.md" "themis::user_storage_encrypted — module documentation" _blank
    click mod_voice href "https://github.com/makr-code/ThemisDB/blob/develop/src/voice/ROADMAP.md" "themis::voice — module documentation" _blank
    click mod_whisper href "https://github.com/makr-code/ThemisDB/blob/develop/src/whisper/ROADMAP.md" "themis::whisper — module documentation" _blank
```

Legend: `✅` = public plugin available, `🔒` = private plugin integration.

## Tier Classification

| Tier | Description | Count | Modules (sample) |
|------|-------------|-------|-------------------|
| T0 | Trusted Core | 5 | base, core, plugins, themis, utils |
| T1 | Engine (Query/Storage/Index) | 7 | aql, cache, execution, index, metadata, query … |
| T3 | Infrastructure & Governance | 56 | acceleration, access_model, ai, analytics, api, auth … |

## Consumer / Provider Dependencies

Top modules by outgoing dependency count:

| Module | Provides to (count) | Consumes from |
|--------|---------------------|---------------|
| `execution` | 3 | `index`, `query`, `storage` |
| `rag` | 3 | `llm`, `retrieval`, `storage` |
| `transaction` | 3 | `core`, `replication`, `storage` |
| `ai` | 2 | `llm`, `retrieval` |
| `api` | 2 | `auth`, `server` |
| `distributed_knowledge` | 2 | `graph`, `replication` |
| `governance` | 2 | `auth`, `observability` |
| `llm_wiki` | 2 | `llm`, `rag` |
| `search` | 2 | `index`, `retrieval` |
| `sharding` | 2 | `replication`, `storage` |
| `themis` | 2 | `core`, `plugins` |
| `training` | 2 | `llm`, `storage` |
| `acceleration` | 1 | `gpu` |
| `analytics` | 1 | `storage` |
| `aql` | 1 | `core` |
| `auth` | 1 | `security` |
| `cache` | 1 | `execution` |
| `config` | 1 | `core` |
| `content` | 1 | `storage` |
| `index` | 1 | `storage` |

Top modules by incoming dependency count:

| Module | Consumed by (count) | Consumer modules |
|--------|----------------------|------------------|
| `storage` | 10 | `analytics`, `content`, `execution`, `index`, `metadata`, `rag`, `replication`, `sharding`, `training`, `transaction` |
| `core` | 7 | `aql`, `config`, `observability`, `process`, `security`, `themis`, `transaction` |
| `llm` | 4 | `ai`, `llm_wiki`, `rag`, `training` |
| `index` | 3 | `execution`, `retrieval`, `search` |
| `replication` | 3 | `distributed_knowledge`, `sharding`, `transaction` |
| `retrieval` | 3 | `ai`, `rag`, `search` |
| `auth` | 2 | `api`, `governance` |
| `base` | 2 | `plugins`, `utils` |
| `execution` | 2 | `cache`, `server` |
| `server` | 2 | `api`, `network` |
| `aql` | 1 | `query` |
| `gpu` | 1 | `acceleration` |
| `graph` | 1 | `distributed_knowledge` |
| `llama_cpp` | 1 | `llm` |
| `observability` | 1 | `governance` |
| `plugins` | 1 | `themis` |
| `query` | 1 | `execution` |
| `rag` | 1 | `llm_wiki` |
| `security` | 1 | `auth` |

## Dependency Evaluation

Inter-tier dependency flow counts:

| Consumer Tier | Provider Tier | Relationship Count |
|---------------|---------------|--------------------|
| T3 | T3 | 20 |
| T3 | T1 | 10 |
| T1 | T1 | 7 |
| T3 | T0 | 5 |
| T0 | T0 | 4 |
| T1 | T0 | 1 |

Dependency label distribution:

| Relationship Label | Count |
|--------------------|-------|
| `semantic search` | 2 |
| `GPU dispatch` | 1 |
| `MVCC` | 1 |
| `audit events` | 1 |
| `authenticates` | 1 |
| `caches` | 1 |
| `config bootstrap` | 1 |
| `content store` | 1 |
| `crypto primitives` | 1 |
| `data commit` | 1 |
| `dispatches` | 1 |
| `distributed` | 1 |
| `distributes` | 1 |
| `executes` | 1 |
| `fine-tunes` | 1 |
| `fulltext index` | 1 |
| `generates` | 1 |
| `knowledge graph` | 1 |
| `knowledge store` | 1 |
| `lifecycle` | 1 |

Isolated modules (no incoming/outgoing dependency): 30
- `access_model`, `cdc`, `chaos`, `chimera`, `distributed_tensor`, `document`, `ethics_ai`, `evaluation`, `exporters`, `failover`, `geo`, `importers`, `ingestion`, `maintenance`, `onnx_clip`, `performance`, `projects`, `prompt_engineering`, `rpc_grpc`, `scheduler`, `scraper`, `stable_diffusion`, `temporal`, `tensor`, `timeseries`, `toolbox`, `updates`, `user_storage_encrypted`, `voice`, `whisper`


## Knowledge Sources

### Developer LLM Wiki
- Generated at: `2026-09-07T03:02:22+00:00`
- Source hash: `407f4e67669f74c9943811d098f5937155a55ca4220a2c0499e0acb56102706d`
- Source file count: 7718
- Module/API entries indexed: 80

### ai_working Artifacts
Wave/batch reports: 42
JSON analysis artifacts: 7

### Documentation Sources
1170 documentation files scanned from `docs/` and repository root governance files.

## Source Hashes

| Source | SHA-256 prefix |
|--------|----------------|
| `ARCHITECTURE.md` | `3983a562f05de940` |
| `FUTURE_ENHANCEMENTS.md` | `2474f927bbbb8189` |
| `ROADMAP.md` | `e49e49cd67bf2920` |
| `ai_context/ARCHITECTURE_CLASSIFICATION.md` | `57a7f2182a1193c8` |
| `ai_context/MODULES_AND_NAMESPACES.md` | `bf42dba44a39f8cb` |
| `ai_context/api_contracts/` | `327db5382688c4b9` |
| `ai_context/developer_llm_wiki/` | `a1c14f1daa494f60` |
| `ai_working/` | `23010ee516eb01da` |
| `docs/` | `719621e53454009e` |
