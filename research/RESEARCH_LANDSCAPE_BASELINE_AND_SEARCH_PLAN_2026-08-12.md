# Research Landscape Baseline + Search Plan (2026-08-12)

Scope: inventory, canonicalization, gap analysis, search strategy, and maintenance checks for research assets in:
- `/home/runner/work/ThemisDB/ThemisDB/docs/research`
- `/home/runner/work/ThemisDB/ThemisDB/research/papers`

## 1) Baseline the current research landscape

### 1.1 Inventory summary

| Scope | Markdown Files | Notes |
|---|---:|---|
| `/home/runner/work/ThemisDB/ThemisDB/docs/research` | 22 | Mixed legacy mirror + support docs + generated influence views |
| `/home/runner/work/ThemisDB/ThemisDB/research/papers` | 36 | Canonical paper entries (including template/index files) |

### 1.2 Indexed paper maturity summary (from `research/papers/README.md`)

| Status | Count |
|---|---:|
| Implemented | 25 |
| In Progress | 9 |
| Planned | 9 |

### 1.3 Domain classification summary (indexed papers)

| Domain | Count |
|---|---:|
| Graph | 1 |
| LLM/RAG | 34 |
| Process Mining | 3 |
| Query Optimization | 4 |
| Vector Index | 1 |

### 1.4 Full inventory — `docs/research`

| File | Domain | Maturity/Status | Source |
|---|---|---|---|
| `docs/research/AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md` | General Research | Supporting Artifact | N/A |
| `docs/research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md` | General Research | Supporting Artifact | N/A |
| `docs/research/DYNAMIC_SCHEMA_RECONFIGURATION_RESEARCH.md` | General Research | Supporting Artifact | N/A |
| `docs/research/GNN_BASED_INDEXING_AND_EMBEDDINGS.md` | Vector Index | Supporting Artifact | N/A |
| `docs/research/GPU_VECTOR_INDEXING_RESEARCH.md` | Vector Index | Supporting Artifact | N/A |
| `docs/research/HYBRID_SEARCH_OPTIMIZATION.md` | General Research | Supporting Artifact | N/A |
| `docs/research/KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md` | Vector Index | Supporting Artifact | N/A |
| `docs/research/LEARNED_INDEX_STRUCTURES_RESEARCH.md` | General Research | Supporting Artifact | N/A |
| `docs/research/LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md` | LLM/RAG | Supporting Artifact | N/A |
| `docs/research/PRODUCT_QUANTIZATION_RESEARCH.md` | Vector Index | Supporting Artifact | N/A |
| `docs/research/PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md` | LLM/RAG | Supporting Artifact | N/A |
| `docs/research/PROMPT_OPTIMIZATION_IMPLEMENTATION_STRATEGY.md` | LLM/RAG | Supporting Artifact | N/A |
| `docs/research/ethics_discourse_process_equality.md` | Process Mining | Supporting Artifact | N/A |
| `docs/research/git_gitops_themis_vergleich.md` | General Research | Supporting Artifact | N/A |
| `docs/research/implementation_influence/README.md` | General Research | Generated Index | implementation_influence views |
| `docs/research/implementation_influence/by_module.md` | General Research | Generated Index | implementation_influence views |
| `docs/research/implementation_influence/by_paper.md` | General Research | Generated Index | implementation_influence views |
| `docs/research/ml_enhancements_bibliography.md` | Graph | Planned/Research Input | bibliography artifacts |
| `docs/research/papers/README.md` | General Research | Unknown | research/papers metadata + index tables |
| `docs/research/papers/arasu_cql_2006.md` | Query Optimization | Planned | research/papers metadata + index tables |
| `docs/research/schema/README.md` | General Research | Supporting Artifact | N/A |
| `docs/research/thermodynamic_anomaly_detection_bibliography.md` | Graph | Planned/Research Input | bibliography artifacts |

### 1.5 Full inventory — `research/papers`

| File | Domain | Maturity/Status | Indexed in `README.md` |
|---|---|---|---|
| `research/papers/README.md` | General Research | Support/Template | No |
| `research/papers/TEMPLATES.md` | General Research | Support/Template | No |
| `research/papers/_template_paper.md` | General Research | Support/Template | No |
| `research/papers/arasu_cql_2006.md` | Query Optimization | Planned | Yes |
| `research/papers/bai_constitutional_ai_rlaif_2022.md` | LLM/RAG | Implemented | Yes |
| `research/papers/bordes_transe_2013.md` | Graph | Unknown | No |
| `research/papers/duckdb_olap_2019.md` | Query Optimization | Planned | Yes |
| `research/papers/forgy_rete_algorithm_1982.md` | General Research | In Progress | No |
| `research/papers/graph_databases_oreilly_2015.md` | Graph | In Progress | Yes |
| `research/papers/graphrag_edge_2024.md` | LLM/RAG | Planned | Yes |
| `research/papers/hipporag_gutierrez_2024.md` | LLM/RAG | Planned | Yes |
| `research/papers/hnsw_efficient_ann_2020.md` | Vector Index | Implemented | Yes |
| `research/papers/khattab_dspy_2023.md` | LLM/RAG | In Progress | Yes |
| `research/papers/liu_geval_2023.md` | General Research | Implemented | Yes |
| `research/papers/llm_index_advisor_integrated_2024.md` | LLM/RAG | Implemented | Yes |
| `research/papers/lmql_beurer_kellner_2023.md` | LLM/RAG | Planned | Yes |
| `research/papers/lora_low_rank_adaptation_2022.md` | LLM/RAG | Implemented | Yes |
| `research/papers/madaan_self_refine_2023.md` | General Research | Implemented | Yes |
| `research/papers/marcus_bao_learned_query_opt_2021.md` | Query Optimization | In Progress | Yes |
| `research/papers/owl2_description_logics_2012.md` | Graph | Unknown | No |
| `research/papers/process_graph_vector_ai_2026.md` | Vector Index | In Progress | Yes |
| `research/papers/process_mining_van_der_aalst_2012.md` | Process Mining | In Progress | Yes |
| `research/papers/processgpt_busch_2023.md` | LLM/RAG | Planned | Yes |
| `research/papers/processtransformer_bukhsh_2021.md` | Process Mining | Planned | Yes |
| `research/papers/prompt_patterns_catalog_2023.md` | LLM/RAG | Implemented | Yes |
| `research/papers/pryzant_protegi_prompt_optimization_2023.md` | LLM/RAG | Implemented | Yes |
| `research/papers/sheng_slora_concurrent_adapters_2023.md` | LLM/RAG | In Progress | Yes |
| `research/papers/tarjan_scc_1972.md` | General Research | Implemented | No |
| `research/papers/temporal_sql2011_2012.md` | Query Optimization | In Progress | Yes |
| `research/papers/tensor_networks_themisdb.md` | General Research | In Progress | No |
| `research/papers/verwaltungs_it_ozg_sources.md` | Security | In Progress | Yes |
| `research/papers/wang_speculative_rag_2024.md` | LLM/RAG | Planned | Yes |
| `research/papers/yao_react_2022.md` | LLM/RAG | Implemented | Yes |
| `research/papers/yao_tree_of_thoughts_2023.md` | General Research | Implemented | Yes |
| `research/papers/zheng_llm_judge_2023.md` | LLM/RAG | Implemented | Yes |
| `research/papers/zhou_ai4db_survey_2022.md` | General Research | In Progress | Yes |

## 2) Source-of-truth policy and duplication risk

### 2.1 Canonical path policy (adopted)

- Canonical research root: `/home/runner/work/ThemisDB/ThemisDB/research/`.
- Canonical paper records: `/home/runner/work/ThemisDB/ThemisDB/research/papers/`.
- Canonical generated influence views: `/home/runner/work/ThemisDB/ThemisDB/research/implementation_influence/`.
- Validation/index scripts must resolve against `research/*` (not `docs/research/*`).

### 2.2 Migration map (`docs/research/papers` → `research/papers`)

| Legacy path | Canonical path | Duplicate state | Action |
|---|---|---|---|
| `/home/runner/work/ThemisDB/ThemisDB/docs/research/papers/README.md` | `/home/runner/work/ThemisDB/ThemisDB/research/papers/README.md` | Content diverged | Keep canonical under research/papers; stop updating legacy mirror |
| `/home/runner/work/ThemisDB/ThemisDB/docs/research/papers/arasu_cql_2006.md` | `/home/runner/work/ThemisDB/ThemisDB/research/papers/arasu_cql_2006.md` | Content diverged | Keep canonical under research/papers; stop updating legacy mirror |

## 3) Structured gap analysis vs roadmap priorities

| Theme | Coverage in `research/papers/README.md` | Gap | Priority |
|---|---|---|---|
| Query AI | Bao (in progress), AI4DB (in progress), DuckDB (planned), CQL (planned) | Need stronger benchmark-linked follow-up references for runtime reoptimizer and index-advisor integration | Wave A/B |
| GraphRAG/HippoRAG | GraphRAG (planned), HippoRAG (planned), OWL2 + TransE (planned) | Need explicit benchmark-suite references and implementation-ready experiment protocols in paper docs | Wave A/B |
| Process intelligence | Process Mining (in progress), ProcessGPT (planned), ProcessTransformer (planned) | Need actionable references tied to current process module evidence and next issue links | Wave B |
| Temporal SQL | SQL:2011 temporal (in progress), CQL (planned) | Need explicit conformance matrix references and measurable gate criteria per temporal feature family | Wave B |
| Index optimization | HNSW (implemented), LLM Index Advisor (implemented), Product Quantization/Learned Index docs outside canonical papers index | Need canonical paper entries in `research/papers/` for PQ/learned-index docs now living as standalone research notes | Wave A/B |

### 3.1 Missing paper docs candidates for roadmap-linked scientific claims

- `/home/runner/work/ThemisDB/ThemisDB/research/PRODUCT_QUANTIZATION_RESEARCH.md` lacks a canonical linked paper entry in `/home/runner/work/ThemisDB/ThemisDB/research/papers/README.md`.
- `/home/runner/work/ThemisDB/ThemisDB/research/LEARNED_INDEX_STRUCTURES_RESEARCH.md` lacks a canonical linked paper entry in `/home/runner/work/ThemisDB/ThemisDB/research/papers/README.md`.
- `/home/runner/work/ThemisDB/ThemisDB/research/GPU_VECTOR_INDEXING_RESEARCH.md` should be decomposed into canonical per-paper entries if used for roadmap status claims.
- `Matryoshka Representation Learning` is indexed via anchor only; no dedicated `/home/runner/work/ThemisDB/ThemisDB/research/papers/<file>.md` record exists.

### 3.2 Stale-record flags

- Legacy mirror divergence: `/home/runner/work/ThemisDB/ThemisDB/docs/research/papers/arasu_cql_2006.md` still marks `Not Started`, while canonical `/home/runner/work/ThemisDB/ThemisDB/research/papers/arasu_cql_2006.md` marks `Fully Implemented`.
- Legacy index drift: `/home/runner/work/ThemisDB/ThemisDB/docs/research/papers/README.md` is missing newer categories and entries present in canonical `/home/runner/work/ThemisDB/ThemisDB/research/papers/README.md`.
- Planned entries without strong follow-up anchors (issue/milestone/benchmark details still sparse): `duckdb_olap_2019.md`, `graphrag_edge_2024.md`, `hipporag_gutierrez_2024.md`, `processgpt_busch_2023.md`, `processtransformer_bukhsh_2021.md`, `lmql_beurer_kellner_2023.md`, `wang_speculative_rag_2024.md`.

## 4) Scientific search strategy by domain

| Domain | Search Axes | Priority Venues/Sources | Query Families |
|---|---|---|---|
| LLM/RAG | retrieval policies, judge reliability, latency, adapter routing, hallucination controls | NeurIPS, ICLR, ICML, ACL/EMNLP, arXiv cs.CL/cs.AI | `database rag retrieval policy`, `llm judge calibration benchmark`, `speculative rag latency`, `multi-lora routing serving` |
| Query Optimization | learned cost models, adaptive re-optimization, cardinality estimation, HTAP planners | VLDB, SIGMOD, CIDR, TODS, arXiv cs.DB | `learned query optimizer database`, `runtime reoptimizer benchmark`, `ai4db cardinality estimation` |
| Vector Index | ANN graph variants, PQ/IVF tradeoffs, filtered ANN, update-heavy ANN | VLDB, SIGMOD, NeurIPS Datasets/Benchmarks, ann-benchmarks, arXiv cs.LG | `product quantization ann database`, `learned index vector retrieval`, `dynamic hnsw update benchmark` |
| Graph | GraphRAG pipelines, KGE, ontology-constrained retrieval, graph summarization | KDD, WWW, NeurIPS, ISWC, VLDB | `graphrag benchmark knowledge graph`, `ontology constrained retrieval llm`, `kge completion database retrieval` |
| Process Mining | predictive process analytics, LLM process reasoning, conformance checking | BPM, ICPM, CAiSE, arXiv cs.AI | `process mining llm`, `process transformer benchmark`, `conformance checking streaming` |
| Temporal | bi/tri-temporal semantics, stream-time correctness, late-data compensation | VLDB, SIGMOD, IEEE ICDE | `sql2011 temporal conformance`, `stream watermark correctness`, `late data temporal query` |
| Security/Governance | RAG threat models, policy enforcement, compliance-by-design, auditability | IEEE S&P, USENIX Security, NDSS, NIST/ENISA docs | `rag threat model benchmark`, `database policy conflict detection`, `ai governance compliance controls` |

## 5) Wave-based acquisition and review prioritization

| Wave | Selection Rule | Candidate Set | Ranking Criteria |
|---|---|---|---|
| Wave A | Direct blockers for `In Progress` roadmap items | Bao, AI4DB, S-LoRA, DSPy, SQL:2011 Temporal, Process Mining | Implementation impact > evidence quality > recency |
| Wave B | Enables near-term `Planned` delivery windows | DuckDB, CQL extensions, GraphRAG, HippoRAG, ProcessGPT, ProcessTransformer, LMQL, Speculative RAG | Roadmap dependency proximity > benchmark quality > interoperability risk |
| Wave C | High-upside exploration for future enhancements | Matryoshka, advanced learned indexes, robust RAG evaluation frameworks, governance/safety frontier work | Strategic upside > novelty > cost to productionize |

## 6) Standardized extraction and traceability schema

For each selected paper entry in `/home/runner/work/ThemisDB/ThemisDB/research/papers/` capture:

1. Key claims (what the paper proves).
2. Preconditions (hardware, dataset, assumptions).
3. Measurable claims (latency/throughput/accuracy/memory).
4. Failure modes and edge cases.
5. Concrete module mapping (`src/<module>/...`).
6. Version target + roadmap linkage.
7. Validation anchors (tests/benchmarks/docs).

Traceability requirements:
- Link each paper row in `/home/runner/work/ThemisDB/ThemisDB/research/papers/README.md`.
- Link each module impact in `/home/runner/work/ThemisDB/ThemisDB/research/implementation_influence/README.md`.
- Regenerate `/home/runner/work/ThemisDB/ThemisDB/research/implementation_influence/by_module.md`, `by_paper.md`, `by_version.md`.

## 7) Verification and maintenance loop

After each paper batch:

1. `python3 /home/runner/work/ThemisDB/ThemisDB/scripts/validate_research_metadata.py`
2. `python3 /home/runner/work/ThemisDB/ThemisDB/scripts/validate_research_links.py`
3. `python3 /home/runner/work/ThemisDB/ThemisDB/scripts/generate_research_index.py --dry-run`
4. `python3 /home/runner/work/ThemisDB/ThemisDB/scripts/generate_research_index.py` (when committing index updates)
5. Review orphan/empty module mappings in generated influence views.

Cadence:
- Bi-weekly refresh for LLM/RAG/query-AI domains.
- Monthly refresh for graph/process/temporal/index domains.
- Quarterly governance alignment sweep against `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md` and `/home/runner/work/ThemisDB/ThemisDB/FUTURE_ENHANCEMENTS.md`.

## 8) Deliverables completed in this implementation

- Canonical inventory + deduplication report (this document).
- Canonical path policy and migration map for duplicated paper files.
- Roadmap-priority gap matrix and stale-record flags.
- Domain search playbook with query families and venue priorities.
- Validation-ready maintenance checklist and command sequence.

## Open decisions requiring maintainer confirmation

1. Confirm whether `/home/runner/work/ThemisDB/ThemisDB/docs/research/` should remain as read-only legacy mirror or be removed after migration.
2. Confirm whether future wave prioritization should remain balanced (A/B delivery + C exploration) or shift to strict near-term delivery focus.
3. Confirm whether best-practice/ADR artifacts should stay in the same acquisition wave board when tightly coupled to paper-backed features.
