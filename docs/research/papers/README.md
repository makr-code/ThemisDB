# Scientific Papers Index

This directory contains documentation of all scientific papers that have influenced ThemisDB's implementation.

## Purpose

Each paper that serves as a foundation for a ThemisDB algorithm, data structure, or design decision should be documented here using the [template](_template_paper.md).

## Index

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Attention Is All You Need — Vaswani et al. (2017)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#2-transformer-architecture--foundation-models) | `src/llm/` | v1.3.0+ | ✅ Implemented |
| [BERT — Devlin et al. (2019)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#3-embeddings--semantic-representations) | `src/vector/`, `src/rag/` | v1.0.0+ | ✅ Implemented |
| [Sentence-BERT — Reimers & Gurevych (2019)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#32-sentence-embeddings-sbert) | `src/vector/`, `src/rag/` | v1.0.0+ | ✅ Implemented |
| [GPT-3 Few-Shot Learning — Brown et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#41-few-shot-in-context-learning) | `src/prompt_engineering/` | v1.2.0+ | ✅ Implemented |
| [Chain-of-Thought Prompting — Wei et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#42-chain-of-thought-cot-prompting) | `src/prompt_engineering/` | v1.2.0+ | ✅ Implemented |
| [LoRA — Hu et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#71-low-rank-adaptation-lora) | `src/llm/lora/` | v1.3.0+ | ✅ Implemented |
| [QLoRA — Dettmers et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#72-qlora) | `src/llm/lora/` | v1.3.0+ | ✅ Implemented |
| [FlashAttention — Dao et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#83-flash-attention) | `src/llm/` (CUDA) | v1.4.0-alpha+ | ✅ Implemented |
| [Speculative Decoding — Chen et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#84-speculative-decoding) | `src/llm/` | v1.4.0-alpha+ | ✅ Implemented |
| [PagedAttention — Kwon et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#85-pagedattention--continuous-batching) | `src/llm/` | v1.4.0-alpha+ | ✅ Implemented |
| [RAG — Lewis et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#61-original-rag-framework) | `src/rag/` | v1.2.0+ | ✅ Implemented |
| [RAGAS — Es et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#63-rag-evaluation-ragas) | `src/llm/monitoring/` | v1.3.0+ | ✅ Implemented |
| [APE — Zhou et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape) | `src/prompt_engineering/` | v1.4.0-alpha+ | ✅ Implemented |
| [Matryoshka Representation Learning — Kusupati et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#34-matryoshka-representation-learning) | `src/vector/` | v1.4.1+ | ⏳ Planned |
| [Scaling Laws — Kaplan et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#22-scaling-laws) | `src/llm/` (model selection) | v1.3.0+ | ✅ Implemented |
| [Graph-Process Schema and Hybrid Vector+Graph Retrieval — ThemisDB (2026)](process_graph_vector_ai_2026.md) | `src/graph/`, `src/vector/`, `src/rag/`, `src/llm/` | v1.9.0+ | 🔄 Partially Implemented |

**Relational & Query:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [DuckDB — Raasveldt & Mühleisen (2019)](duckdb_olap_2019.md) | `src/query/`, `src/exporters/` | planned v2.x | ⏳ Planned |

**AI-Driven Query Optimization:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Bao: Learned Query Optimization — Marcus et al. (2021)](marcus_bao_learned_query_opt_2021.md) | `src/query/adaptive_optimizer.cpp`, `src/query/runtime_reoptimizer.cpp` | planned v2.0.0 | 🔄 In Progress |
| [AI Meets Database (AI4DB) — Zhou et al. (2022)](zhou_ai4db_survey_2022.md) | `src/query/`, `src/storage/index_analyzer.cpp`, `src/cache/` | v2.0.0+ framework | 🔄 In Progress |

**Streaming & Continuous Queries:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [CQL — Arasu, Babu & Widom (2006)](arasu_cql_2006.md) | `src/query/continuous_query/`, `src/timeseries/`, `src/analytics/` | planned v2.0.0 | ⏳ Planned |

**Vector Search:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [HNSW — Malkov & Yashunin (2020)](hnsw_efficient_ann_2020.md) | `src/index/`, `src/vector/`, `src/rag/` | v1.0.0+ | ✅ Implemented |

**Graph Databases:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Graph Databases — Robinson, Webber & Eifrem (2015)](graph_databases_oreilly_2015.md) | `src/graph/`, `src/aql/` | v1.0.0+ | 🔄 Partially Implemented |

**Temporal / Timeline:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [SQL:2011 Temporal Features — Kulkarni & Michels (2012)](temporal_sql2011_2012.md) | `src/temporal/`, `src/query/` | v1.x+ | 🔄 Partially Implemented |

**Process Mining & BPMN:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Process Mining — van der Aalst (2016)](process_mining_van_der_aalst_2012.md) | `src/process/`, `src/analytics/` | v1.9.0+ | 🔄 Partially Implemented |
| [ProcessGPT — Busch et al. (2023)](processgpt_busch_2023.md) | `src/process/`, `src/llm/` | planned Q2 2026 | ⏳ Planned |
| [ProcessTransformer — Bukhsh et al. (2021)](processtransformer_bukhsh_2021.md) | `src/process/`, `src/training/` | planned Q1 2027 | ⏳ Planned |

**Graph RAG & Retrieval:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md) | `src/process/`, `src/rag/`, `src/graph/` | planned Q3 2026 | ⏳ Planned |
| [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md) | `src/process/`, `src/rag/` | planned Q2 2026 | ⏳ Planned |

**Near-Realtime LLM Inferencing & RAG:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [S-LoRA — Sheng et al. (2023)](sheng_slora_concurrent_adapters_2023.md) | `src/llm/lora/`, `src/rag/` | planned Q2/Q3 2026 | 🔄 In Progress |
| [Speculative RAG — Wang et al. (2024)](wang_speculative_rag_2024.md) | `src/rag/streaming_retriever.cpp`, `src/llm/` | planned Q1 2027 | ⏳ Planned |

**LoRA / PEFT:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [LoRA — Hu et al. (2022)](lora_low_rank_adaptation_2022.md) | `src/llm/lora/`, `src/training/` | v1.3.0+ | ✅ Implemented |

**Prompt Engineering:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [Prompt Pattern Catalog — White et al. (2023)](prompt_patterns_catalog_2023.md) | `src/prompt_engineering/` | v1.2.0+ | ✅ Implemented |
| [LMQL — Beurer-Kellner et al. (2023)](lmql_beurer_kellner_2023.md) | `src/prompt_engineering/`, `src/llm/` | planned v2.x | ⏳ Planned |

**Verwaltungs-IT / Administrative IT:**

| Paper | Module(s) | Version | Status |
|-------|-----------|---------|--------|
| [OZG / FIM / XÖV Standards](verwaltungs_it_ozg_sources.md) | `src/process/`, `src/importers/`, `src/auth/` | v1.9.0+ | 🔄 Partially Implemented |

> For full IEEE citations and implementation details, see [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md).

## Adding a New Paper

1. Copy [_template_paper.md](_template_paper.md) to a new file named `<short_key>_<year>.md`  
   Example: `hnsw_efficient_ann_2018.md`
2. Fill in all required fields (see [TEMPLATES.md](TEMPLATES.md))
3. Link the paper in the relevant module README under *Wissenschaftliche Grundlagen & Einflüsse*
4. Register it in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
<topic>_<year>.md
```

Examples:
- `hnsw_efficient_ann_2018.md`
- `lsm_tree_rocksdb_2016.md`
- `raft_consensus_2014.md`

## See Also

- [TEMPLATES.md](TEMPLATES.md) — required fields and formatting rules
- [_template_paper.md](_template_paper.md) — copy-paste starter template
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — end-to-end contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master cross-reference index
