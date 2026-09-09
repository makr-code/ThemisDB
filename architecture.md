# ThemisDB Architecture

> **Auto-generated** — do not edit manually.
> Source: `tools/architecture-generator/generate_architecture.py`
> Generated: `2026-09-09T07:09:22.569994+00:00`

## Statistics

| Metric | Value |
|--------|-------|
| Total modules | 68 |
| Total relationships | 47 |
| Modules with public plugin | 10 |
| Modules with private plugin | 4 |
| Modules with API contracts | 6 |
| Documentation sources scanned | 1166 |
| LLM Wiki source files | 7718 |
| LLM Wiki module/API entries | 80 |
| LLM Wiki generated | `2026-09-07T03:02:22+00:00` |
| ai_working artifacts | 64 |

## Module Architecture Diagram

```mermaid
flowchart TD
    subgraph T0["T0: Trusted Core"]
        base["base\nthemis::resource"]
        core["core\nthemis::core"]
        plugins["plugins\nthemis::plugins"]
        themis["themis\nthemis"]
        utils["utils\nthemis::utils"]
    end
    subgraph T1["T1: Engine (Query/Storage/Index)"]
        aql["aql\nthemis::aql"]
        cache["cache\nthemis::cache"]
        execution["execution\nthemis::execution"]
        index["index\nthemis::index"]
        metadata["metadata\nthemis::metadata"]
        query["query\nthemis::query"]
        storage["storage\nthemis::storage"]
    end
    subgraph T3["T3: Infrastructure & Governance"]
        acceleration["acceleration\nthemis::acceleration"]
        access_model["access_model\nthemis::access_model"]
        ai["ai\nthemis::ai"]
        analytics["analytics\nthemis::analytics"]
        api["api\nthemis::api"]
        auth["auth\nthemis::auth"]
        cdc["cdc\nthemis::cdc"]
        chaos["chaos\nthemis::chaos"]
        chimera["chimera\nthemis::chimera"]
        config["config\nthemis::config"]
        content["content\nthemis::content"]
        distributed_knowledge["distributed_knowledge\nthemis::distributed_knowledge"]
        distributed_tensor["distributed_tensor\nthemis::distributed_tensor"]
        document["document\nthemis::document"]
        ethics_ai["ethics_ai\nthemis::ethics_ai\n🔒plugin"]
        evaluation["evaluation\nthemis::evaluation"]
        exporters["exporters\nthemis::exporters\n✅plugin"]
        failover["failover\nthemis::failover"]
        geo["geo\nthemis::geo\n✅plugin"]
        governance["governance\nthemis::governance"]
        gpu["gpu\nthemis::gpu"]
        graph["graph\nthemis::graph"]
        importers["importers\nthemis::importers\n🔒plugin"]
        ingestion["ingestion\nthemis::ingestion"]
        llama_cpp["llama_cpp\nthemis::llama_cpp\n✅plugin"]
        llm["llm\nthemis::llm"]
        llm_wiki["llm_wiki\nthemis::llm_wiki\n🔒plugin"]
        maintenance["maintenance\nthemis::maintenance"]
        network["network\nthemis::network"]
        observability["observability\nthemis::observability"]
        onnx_clip["onnx_clip\nthemis::onnx_clip"]
        performance["performance\nthemis::performance"]
        process["process\nthemis::process"]
        projects["projects\nthemis::projects"]
        prompt_engineering["prompt_engineering\nthemis::prompt_engineering"]
        rag["rag\nthemis::rag"]
        replication["replication\nthemis::replication"]
        retrieval["retrieval\nthemis::retrieval"]
        rpc_grpc["rpc_grpc\nthemis::rpc_grpc"]
        scheduler["scheduler\nthemis::scheduler"]
        scraper["scraper\nthemis::scraper\n✅plugin"]
        search["search\nthemis::search"]
        security["security\nthemis::security"]
        server["server\nthemis::server"]
        sharding["sharding\nthemis::sharding"]
        stable_diffusion["stable_diffusion\nthemis::stable_diffusion\n✅plugin"]
        temporal["temporal\nthemis::temporal"]
        tensor["tensor\nthemis::tensor"]
        timeseries["timeseries\nthemis::timeseries\n✅plugin"]
        toolbox["toolbox\nthemis::toolbox"]
        training["training\nthemis::training"]
        transaction["transaction\nthemis::transaction"]
        updates["updates\nthemis::updates"]
        user_storage_encrypted["user_storage_encrypted\nthemis::user_storage_encrypted\n🔒plugin"]
        voice["voice\nthemis::voice"]
        whisper["whisper\nthemis::whisper\n✅plugin"]
    end

    aql -->|"parses/plans"| core
    query -->|"optimizes"| aql
    execution -->|"executes"| query
    execution -->|"reads/writes"| storage
    execution -->|"uses"| index
    index -->|"persists"| storage
    cache -->|"caches"| execution
    metadata -->|"schema store"| storage
    transaction -->|"MVCC"| core
    transaction -->|"data commit"| storage
    transaction -->|"distributes"| replication
    replication -->|"replica I/O"| storage
    sharding -->|"partitions"| storage
    sharding -->|"replicates shards"| replication
    api -->|"routes"| server
    api -->|"authenticates"| auth
    server -->|"dispatches"| execution
    auth -->|"primitives"| security
    governance -->|"policy check"| auth
    governance -->|"audit events"| observability
    rag -->|"retrieves"| retrieval
    rag -->|"generates"| llm
    rag -->|"knowledge store"| storage
    llm -->|"local inference"| llama_cpp
    llm_wiki -->|"wiki retrieval"| llm
    llm_wiki -->|"provenance"| rag
    retrieval -->|"vector search"| index
    ai -->|"orchestrates"| llm
    ai -->|"semantic search"| retrieval
    acceleration -->|"GPU dispatch"| gpu
    observability -->|"metrics hooks"| core
    network -->|"transport"| server
    distributed_knowledge -->|"knowledge graph"| graph
    distributed_knowledge -->|"distributed"| replication
    analytics -->|"reads data"| storage
    training -->|"fine-tunes"| llm
    training -->|"model storage"| storage
    content -->|"content store"| storage
    search -->|"fulltext index"| index
    search -->|"semantic search"| retrieval
    security -->|"crypto primitives"| core
    config -->|"config bootstrap"| core
    process -->|"lifecycle"| core
    utils -->|"utilities"| base
    themis -->|"root aggregation"| core
    themis -->|"plugin loader"| plugins
    plugins -->|"plugin base"| base
```

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
| `transaction` | 3 | `core`, `replication`, `storage` |
| `rag` | 3 | `llm`, `retrieval`, `storage` |
| `sharding` | 2 | `replication`, `storage` |
| `api` | 2 | `auth`, `server` |
| `governance` | 2 | `auth`, `observability` |
| `llm_wiki` | 2 | `llm`, `rag` |
| `ai` | 2 | `llm`, `retrieval` |
| `distributed_knowledge` | 2 | `graph`, `replication` |
| `training` | 2 | `llm`, `storage` |
| `search` | 2 | `index`, `retrieval` |
| `themis` | 2 | `core`, `plugins` |
| `aql` | 1 | `core` |
| `query` | 1 | `aql` |
| `index` | 1 | `storage` |
| `cache` | 1 | `execution` |
| `metadata` | 1 | `storage` |
| `replication` | 1 | `storage` |
| `server` | 1 | `execution` |
| `auth` | 1 | `security` |

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
1166 documentation files scanned from `docs/` and repository root governance files.

## Source Hashes

| Source | SHA-256 prefix |
|--------|----------------|
| `ARCHITECTURE.md` | `4e17d46197ca1d0f` |
| `FUTURE_ENHANCEMENTS.md` | `27719b9a0d59d926` |
| `ROADMAP.md` | `84e7325175bb2229` |
| `ai_context/ARCHITECTURE_CLASSIFICATION.md` | `57a7f2182a1193c8` |
| `ai_context/MODULES_AND_NAMESPACES.md` | `bf42dba44a39f8cb` |
| `ai_context/api_contracts/` | `327db5382688c4b9` |
| `ai_context/developer_llm_wiki/` | `a1c14f1daa494f60` |
| `ai_working/` | `23010ee516eb01da` |
| `docs/` | `4e41caa1db36a619` |
