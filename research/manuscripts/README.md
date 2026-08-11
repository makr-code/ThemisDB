# ThemisDB Manuscript Portfolio

**Status**: Active  
**Last Updated**: 2026-08-10  
**Scope**: Canonical portfolio structure for ThemisDB-authored research manuscripts, publication drafts, and migration of legacy top-level draft files.  
**Primary (Quelle der Wahrheit)**: `research/README.md`, `research/ARXIV_PAPER_TEMPLATE.md`, `research/implementation_influence/by_module.md`, module-level `README.md` / `ROADMAP.md` files under `src/`, `tests/`, and `benchmarks/`.

---

## Purpose

This directory is the canonical home for **ThemisDB-authored manuscripts**.
It separates internal publication drafts from:

- `research/papers/` — external scientific sources and paper summaries
- `research/architecture_decisions/` — ADR records
- `research/experiments/` — reproducible experiment artefacts
- top-level `research/*_DRAFT.md` — legacy manuscript locations under migration

## Current State

- The repository already contains a large body of arXiv-style ThemisDB draft papers.
- Most active drafts still live at the top level of `research/`.
- Several themes overlap and require explicit clustering to avoid duplicate submissions and fragmented evidence.
- High-value gaps remain underrepresented in manuscript form, especially failover/recovery, cross-modal cost models, and observability.

## Portfolio Structure

| Cluster | Path | Scope |
|---|---|---|
| Flagship | `research/manuscripts/flagship/` | System-wide ThemisDB papers and umbrella narratives |
| Systems | `research/manuscripts/systems/` | Query processing, optimization, cost models, and cross-cutting systems work |
| Retrieval & RAG | `research/manuscripts/retrieval_rag/` | RAG, hybrid retrieval, evaluation, and contention-aware retrieval |
| LLM Runtime & Training | `research/manuscripts/llm_runtime_training/` | Serving, adapters, scheduling, training, and runtime lifecycle |
| Distributed Consistency & Resilience | `research/manuscripts/distributed_consistency_resilience/` | Sharding, replication, failover, recovery, CDC, transaction recovery |
| Geo / Temporal / Streaming | `research/manuscripts/geo_temporal_streaming/` | Geo, time-series, streaming, bitemporal, GPU-fusion topics |
| Security / Governance / Ethics | `research/manuscripts/security_governance_ethics/` | PQC, ethics AI, policy/compliance, safety and governance |
| Verticals | `research/manuscripts/verticals/` | Domain-specific solution papers (for example finance/HFT) |

## Manuscript Lifecycle

Use these lifecycle labels consistently in manuscript headers and indexes:

- `ACTIVE_DRAFT` — active working manuscript, still evolving
- `REVIEW_CANDIDATE` — structurally coherent, evidence mostly assembled
- `SUBMISSION_CANDIDATE` — only final experiments/editorial work remain
- `SUPERSEDED_DRAFT` — replaced by a better-scoped successor
- `ARCHIVE_CANDIDATE` — keep only for historical traceability

## Required Metadata

Every manuscript in this portfolio should state:

- Title
- Status
- Version
- Last Updated
- Target Venue
- Scientific Delta
- Canonical repository evidence
- Required experiments
- Open risks / claim boundaries
- Overlap / successor / predecessor relation

## Migration Matrix for Existing Manuscripts

| Current file | Target cluster | Portfolio decision |
|---|---|---|
| `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` | `flagship/` | Keep as flagship umbrella paper |
| `DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md` | `flagship/` or `distributed_consistency_resilience/` | Keep separate only if the distributed contribution stays distinct from the flagship system paper |
| `THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md` | `distributed_consistency_resilience/` | Keep as sharding/fault model paper |
| `DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md` | `retrieval_rag/` | Primary database-native RAG paper |
| `SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md` | `retrieval_rag/` | Separate contention/isolation paper |
| `ACID_CONSTRAINED_RAG_DRAFT.md` | `retrieval_rag/` | Treat as predecessor / appendix candidate unless re-expanded with unique experiments |
| `HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md` | `retrieval_rag/` | Retrieval system comparison paper |
| `COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md` | `systems/` | Optimizer / planning paper |
| `QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md` | `systems/` | Shared IR / query-engine paper |
| `LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md` | `llm_runtime_training/` | Adapter lifecycle paper |
| `DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md` | `llm_runtime_training/` | Primary serving systems paper |
| `CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md` | `llm_runtime_training/` | Retain only as scheduler-centric companion paper |
| `GOSSIP_AWARE_LORA_ROUTING_DRAFT.md` | `llm_runtime_training/` | Routing/evaluation companion paper |
| `GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md` | `llm_runtime_training/` | Domain-routing paper |
| `ADALORA_FEDERATED_TRAINING_KNOWLEDGE_GRAPH_DRAFT.md` | `llm_runtime_training/` | Specialized training paper |
| `ADALORA_TT_BRIDGE_ARXIV_DRAFT.md` | `llm_runtime_training/` | Compression / adapter representation paper |
| `GPU_GEOSPATIAL_FAISS_DBSCAN_TEMPORAL_FUSION_DRAFT.md` | `geo_temporal_streaming/` | Keep as GPU geo-temporal fusion paper |
| `BITEMPORAL_ENGINE_HLC_CONFLICT_RESOLUTION_PAPER_DRAFT.md` | `geo_temporal_streaming/` | Keep as temporal systems paper |
| `BITEMPORAL_CDC_EXACTLY_ONCE_SCHEMA_EVOLUTION_DRAFT.md` | `distributed_consistency_resilience/` | Keep as CDC / lineage / recovery paper |
| `SIMD_TIMESERIES_COMPRESSION_GORILLA_CONTINUOUS_AGG_DRAFT.md` | `geo_temporal_streaming/` | Keep as time-series systems paper |
| `PROCESS_MINING_OCEL2_LIGHTRAG_GDPR_BPMN_DRAFT.md` | `verticals/` | Vertical process-intelligence paper |
| `POST_QUANTUM_CRYPTOGRAPHY_HTAP_DATABASE_DRAFT.md` | `security_governance_ethics/` | Security / compliance paper |
| `ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md` | `security_governance_ethics/` | Ethics/governance paper |
| `HFT_RAG_LLM_THEMISDB_TRADING_ORCHESTRATION_ARXIV_2026.md` | `verticals/` | Finance vertical paper |
| `boltzmann_flare_rag_monitoring.tex` | `systems/` | Observability paper seed / companion artefact |

## High-Priority New Manuscripts (2026-08)

1. ✅ `distributed_consistency_resilience/FAILOVER_SPLIT_BRAIN_DISASTER_RECOVERY_PAPER_DRAFT.md` — REVIEW_CANDIDATE
2. ✅ `distributed_consistency_resilience/UNIFIED_RECOVERY_SEMANTICS_AI_WORKLOADS_PAPER_DRAFT.md` — REVIEW_CANDIDATE
3. ✅ `systems/CROSS_MODAL_CARDINALITY_COST_MODELS_PAPER_DRAFT.md` — REVIEW_CANDIDATE
4. ✅ `systems/DB_NATIVE_OBSERVABILITY_RAG_AI_INFERENCE_PAPER_DRAFT.md` — REVIEW_CANDIDATE

## Migrated Manuscripts (2026-08-10)

| Original legacy file | Canonical path | Status |
|---|---|---|
| `research/DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md` | `retrieval_rag/DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md` | SUBMISSION_CANDIDATE |
| `research/PROCESS_MINING_OCEL2_LIGHTRAG_GDPR_BPMN_DRAFT.md` | `verticals/PROCESS_MINING_OCEL2_LIGHTRAG_GDPR_BPMN_DRAFT.md` | SUBMISSION_CANDIDATE |
| `research/DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md` | `llm_runtime_training/DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md` | SUBMISSION_CANDIDATE |
| `research/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md` | `llm_runtime_training/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md` | SUBMISSION_CANDIDATE |
| `research/BITEMPORAL_ENGINE_HLC_CONFLICT_RESOLUTION_PAPER_DRAFT.md` | `geo_temporal_streaming/BITEMPORAL_ENGINE_HLC_CONFLICT_RESOLUTION_PAPER_DRAFT.md` | ACTIVE_DRAFT |
| `research/POST_QUANTUM_CRYPTOGRAPHY_HTAP_DATABASE_DRAFT.md` | `security_governance_ethics/POST_QUANTUM_CRYPTOGRAPHY_HTAP_DATABASE_DRAFT.md` | REVIEW_CANDIDATE |

All legacy top-level files have been marked SUPERSEDED_DRAFT with canonical pointer.

## New Manuscripts (Stufe 3, Q4 2026)

1. ✅ `llm_runtime_training/LORA_WIKI_TRANSACTIONAL_KNOWLEDGE_INGESTION_PAPER_DRAFT.md` — ACTIVE_DRAFT
2. ✅ `security_governance_ethics/ETHICS_AI_YAML_POLICY_ENFORCEMENT_PAPER_DRAFT.md` — ACTIVE_DRAFT
3. ✅ `verticals/PROCESS_GRAPH_VECTOR_AI_HYBRID_PAPER_DRAFT.md` — ACTIVE_DRAFT

## Experiment Protocols Created

| Cluster | Protocol file |
|---|---|
| `distributed_consistency_resilience/` | `research/experiments/distributed_consistency_resilience/failover_split_brain_protocol.md` |
| `systems/` | `research/experiments/systems/cross_modal_cost_model_protocol.md` |
| `systems/` | `research/experiments/systems/observability_rag_ai_protocol.md` |
| `llm_runtime_training/` | `research/experiments/llm_runtime_training/wiki_ingestion_protocol.md` |
| `security_governance_ethics/` | `research/experiments/security_governance_ethics/ethics_ai_policy_protocol.md` |
| `verticals/` | `research/experiments/verticals/process_graph_vector_hybrid_protocol.md` |

## Submission Calendar

| Manuscript | Cluster | Status | Submission window |
|---|---|---|---|
| `FAILOVER_SPLIT_BRAIN_DISASTER_RECOVERY_PAPER_DRAFT.md` | distributed_consistency_resilience/ | REVIEW_CANDIDATE | VLDB 2027 / ICDE 2027 (Q4 2026 DDL) |
| `DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md` | retrieval_rag/ | SUBMISSION_CANDIDATE | VLDB 2027 (Q4 2026 DDL) |
| `PROCESS_MINING_OCEL2_LIGHTRAG_GDPR_BPMN_DRAFT.md` | verticals/ | SUBMISSION_CANDIDATE | BPM/ICPM 2026 (Q3 2026) |
| `CROSS_MODAL_CARDINALITY_COST_MODELS_PAPER_DRAFT.md` | systems/ | REVIEW_CANDIDATE | SIGMOD/VLDB 2027 (Q4 2026) |
| `DB_NATIVE_OBSERVABILITY_RAG_AI_INFERENCE_PAPER_DRAFT.md` | systems/ | REVIEW_CANDIDATE | EuroSys Workshop (Q4 2026) |
| `UNIFIED_RECOVERY_SEMANTICS_AI_WORKLOADS_PAPER_DRAFT.md` | distributed_consistency_resilience/ | REVIEW_CANDIDATE | SIGMOD 2027 (Q1 2027) |
| `ADALORA_TT_BRIDGE_ARXIV_DRAFT.md` | llm_runtime_training/ | SUBMISSION_CANDIDATE | arXiv Q3 2026 |
| `DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md` | llm_runtime_training/ | SUBMISSION_CANDIDATE | arXiv Q3/Q4 2026 |
| `POST_QUANTUM_CRYPTOGRAPHY_HTAP_DATABASE_DRAFT.md` | security_governance_ethics/ | REVIEW_CANDIDATE | ACM CCS 2026 |
| `LORA_WIKI_TRANSACTIONAL_KNOWLEDGE_INGESTION_PAPER_DRAFT.md` | llm_runtime_training/ | ACTIVE_DRAFT | VLDB 2027 (Q4 2026) |
| `ETHICS_AI_YAML_POLICY_ENFORCEMENT_PAPER_DRAFT.md` | security_governance_ethics/ | ACTIVE_DRAFT | arXiv Q4 2026 |
| `PROCESS_GRAPH_VECTOR_AI_HYBRID_PAPER_DRAFT.md` | verticals/ | ACTIVE_DRAFT | SIGMOD/BPM 2027 |
| `BITEMPORAL_ENGINE_HLC_CONFLICT_RESOLUTION_PAPER_DRAFT.md` | geo_temporal_streaming/ | ACTIVE_DRAFT | VLDB/SIGMOD 2027 |

- Do not move legacy drafts in bulk without updating all incoming links.
- Prefer adding canonical portfolio indexes first, then migrate individual manuscripts cluster by cluster.
- When a manuscript gains a canonical portfolio path, mark the legacy top-level file as `SUPERSEDED_DRAFT` in `research/README.md`.
- Keep claim scope tighter than repository scope; each paper must have a distinct novelty delta and evidence boundary.

## Next Actions

- Cluster existing legacy drafts by target submission line.
- Reduce overlap in RAG, serving, and retrieval topics.
- Expand manuscript-specific experiment plans under `research/experiments/` as evidence matures.
- Add missing manuscript lines for failover/recovery, observability, and LLM Wiki / transactional knowledge ingestion.
