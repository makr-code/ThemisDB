# ThemisDB Research & Papers

This directory contains research papers, draft manuscripts, architectural analyses, and design documents for ThemisDB development.

## Canonical Structure & Status (2026-08)

| Cluster | Canonical Location | Scope | Canonical? |
|---|---|---|---|
| Manuscripts | [`research/manuscripts/`](manuscripts/README.md) | ThemisDB-authored publication drafts and portfolio clustering | ✅ |
| Papers | [`research/papers/`](papers/README.md) | Curated scientific sources with module/version status | ✅ |
| Drafts / WIP | `research/*_DRAFT.md` (top-level legacy) | Working manuscripts and exploration notes | ⚠️ Working state |
| Experiments | [`research/experiments/`](experiments/README.md) | Reproducible validation runs and benchmark protocols | ✅ |
| Architecture | [`research/architecture_decisions/`](architecture_decisions/README.md) | ADR records for accepted/proposed decisions | ✅ |
| Implementation Influence | [`research/implementation_influence/`](implementation_influence/README.md) | Research → module/version traceability matrix | ✅ |

### Draft Lifecycle Labels

- `ACTIVE_DRAFT`: aktuell bearbeitet, kein kanonischer Ersatz vorhanden.
- `SUPERSEDED_DRAFT`: durch ein neueres/reiferes Dokument ersetzt; nur noch Referenz.
- `ARCHIVE_CANDIDATE`: veraltet, ohne aktive Weiterentwicklung; bei nächster Bereinigung in Archiv verschieben.

### Marked Outdated / Superseded Drafts

| Legacy Draft | Status | Canonical Successor |
|---|---|---|
| [`THEMIS_MULTIMODEL_INDEX_EVALUATION.md`](THEMIS_MULTIMODEL_INDEX_EVALUATION.md) | `SUPERSEDED_DRAFT` | [`THEMIS_MULTIMODEL_INDEX_EVALUATION_V2.md`](THEMIS_MULTIMODEL_INDEX_EVALUATION_V2.md) |
| [`LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS_DRAFT.md`](LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS_DRAFT.md) | `SUPERSEDED_DRAFT` | [`LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md`](LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md) |
| [`GPU_VECTOR_INDEXING_RESEARCH_DRAFT.md`](GPU_VECTOR_INDEXING_RESEARCH_DRAFT.md) | `SUPERSEDED_DRAFT` | [`GPU_VECTOR_INDEXING_RESEARCH.md`](GPU_VECTOR_INDEXING_RESEARCH.md) |
| [`PRODUCT_QUANTIZATION_RESEARCH_DRAFT.md`](PRODUCT_QUANTIZATION_RESEARCH_DRAFT.md) | `SUPERSEDED_DRAFT` | [`PRODUCT_QUANTIZATION_RESEARCH.md`](PRODUCT_QUANTIZATION_RESEARCH.md) |

### Production-Near Documentation Links

- [`research/implementation_influence/by_module.md`](implementation_influence/by_module.md) (source-to-module mapping)
- [`research/implementation_influence/by_version.md`](implementation_influence/by_version.md) (source-to-release mapping)
- [`research/manuscripts/README.md`](manuscripts/README.md) (canonical manuscript portfolio and migration matrix)
- [`src/rag/README.md`](../src/rag/README.md), [`src/prompt_engineering/README.md`](../src/prompt_engineering/README.md), [`src/search/README.md`](../src/search/README.md) (module-level implementation context)

### Review & Documentation Audit Trace

- [`research/DOCUMENTATION_AUDIT_REPORT_2026-05-13.md`](DOCUMENTATION_AUDIT_REPORT_2026-05-13.md)

## Building LaTeX Papers

Papers in `research/*.tex` can be compiled locally with a standard
TeX distribution (TeX Live 2022+ or MiKTeX 22+):

```bash
# From the research/ directory:
cd research/

# Full build with bibliography (run twice for cross-references):
pdflatex boltzmann_flare_rag_monitoring.tex
bibtex   boltzmann_flare_rag_monitoring
pdflatex boltzmann_flare_rag_monitoring.tex
pdflatex boltzmann_flare_rag_monitoring.tex

# Output: boltzmann_flare_rag_monitoring.pdf
```

Required LaTeX packages (all included in TeX Live full):
`amsmath`, `amssymb`, `amsthm`, `booktabs`, `hyperref`,
`natbib`, `algorithm`, `algpseudocode`, `geometry`, `microtype`,
`enumitem`, `array`, `multirow`, `xcolor`.

Quick check (missing packages):
```bash
pdflatex -interaction=nonstopmode boltzmann_flare_rag_monitoring.tex \
  | grep "! LaTeX Error"
```

## Contents

### Manuscript Portfolio (2026-08-10)
- [`manuscripts/README.md`](manuscripts/README.md)
  — canonical ThemisDB manuscript portfolio structure, lifecycle labels, migration matrix,
  and high-priority next-paper list.
- Cluster indexes:
  - [`manuscripts/flagship/README.md`](manuscripts/flagship/README.md)
  - [`manuscripts/systems/README.md`](manuscripts/systems/README.md)
  - [`manuscripts/retrieval_rag/README.md`](manuscripts/retrieval_rag/README.md)
  - [`manuscripts/llm_runtime_training/README.md`](manuscripts/llm_runtime_training/README.md)
  - [`manuscripts/distributed_consistency_resilience/README.md`](manuscripts/distributed_consistency_resilience/README.md)
  - [`manuscripts/geo_temporal_streaming/README.md`](manuscripts/geo_temporal_streaming/README.md)
  - [`manuscripts/security_governance_ethics/README.md`](manuscripts/security_governance_ethics/README.md)
  - [`manuscripts/verticals/README.md`](manuscripts/verticals/README.md)

### High-Priority New Manuscript Seeds (2026-08-10)
- [`manuscripts/distributed_consistency_resilience/FAILOVER_SPLIT_BRAIN_DISASTER_RECOVERY_PAPER_DRAFT.md`](manuscripts/distributed_consistency_resilience/FAILOVER_SPLIT_BRAIN_DISASTER_RECOVERY_PAPER_DRAFT.md)
  — failover state machine, fail-closed split-brain prevention, and DR execution semantics.
- [`manuscripts/distributed_consistency_resilience/UNIFIED_RECOVERY_SEMANTICS_AI_WORKLOADS_PAPER_DRAFT.md`](manuscripts/distributed_consistency_resilience/UNIFIED_RECOVERY_SEMANTICS_AI_WORKLOADS_PAPER_DRAFT.md)
  — cross-module recovery contract spanning sharding, replication, failover, and AI workloads.
- [`manuscripts/systems/CROSS_MODAL_CARDINALITY_COST_MODELS_PAPER_DRAFT.md`](manuscripts/systems/CROSS_MODAL_CARDINALITY_COST_MODELS_PAPER_DRAFT.md)
  — cross-modal selectivity and cost-model research line for AQL/graph/vector/geo.
- [`manuscripts/systems/DB_NATIVE_OBSERVABILITY_RAG_AI_INFERENCE_PAPER_DRAFT.md`](manuscripts/systems/DB_NATIVE_OBSERVABILITY_RAG_AI_INFERENCE_PAPER_DRAFT.md)
  — observability for RAG and AI inference pipelines, linked to the Boltzmann monitoring draft.

### Boltzmann Observability for RAG (2026-06, ACTIVE_DRAFT v0.1)
- [`boltzmann_flare_rag_monitoring.tex`](boltzmann_flare_rag_monitoring.tex)
  — **Boltzmann-Inspired Observability for AI Inference and RAG Pipelines:
  Energy-Entropy Monitoring, FLARE Parallels, and Correlation Analysis.**
  Proposes a statistical-mechanical observability layer deriving entropy $H$,
  effective candidate count $N_\text{eff}$, energy gap, and free-energy proxy
  $\Phi$ from retrieval score distributions.
  Structural comparison table FLARE vs. Boltzmann layer.
  ThemisDB AQL integration examples, performance SLO hypotheses, evaluation
  protocol.
  Bibliography: [`bib/boltzmann_flare_rag_monitoring.bib`](bib/boltzmann_flare_rag_monitoring.bib).
  Related architecture draft: [`docs/architecture/boltzmann_observability_draft.md`](../docs/architecture/boltzmann_observability_draft.md).
  Performance expectations: [`docs/performance/boltzmann_observability_expectations.md`](../docs/performance/boltzmann_observability_expectations.md).

### Ethics AI Module Research (2026-04-29, v0.4)
- [ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md](ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md) — **Declarative Multi-Philosophy Ethical Reasoning in Database-Native AI Systems: YAML-Configured Ethics Schools and Structured Discourse in ThemisDB** (v0.4). Target: arXiv cs.AI / cs.CY. **Central pattern: Inference Trifecta** — RAG (7 AQL patterns) + **Ethical Monocle** (YAML→PromptScaffold, formal construction function `M(P,T_budget)`) + **LoRA Judge** (school-aware faithfulness evaluator, RLAIF loop). All principle citations grounded in actual `plugins/ethics_ai/philosophies/` YAML files: `kant:kategorischer_imperativ`, `kant:selbstzweck`, `utilitarianism:greatest_happiness`, `contractualism:original_position` / `reasonable_rejection`, `lebensphilosophie_nietzsche:will_to_power`, `socratic:socratic_method`. Appendix B: direct YAML excerpts from 5 profiles. Schema note: `nietzsche.yaml` uses `school:` (not `school_id:`) and map-style theses — both documented. 34 references, 5 RQs, 3 hypotheses, W1–W6 workloads, Tables R1–R4, 4-stage production path.

### 🏆 Flagship System Paper (2026-04-27)
- [THEMISDB_SYSTEM_PAPER_ARXIV_2026.md](THEMISDB_SYSTEM_PAPER_ARXIV_2026.md) — **ThemisDB: An ACID-Compliant Multi-Model Database with Native AI/LLM Integration** (v0.2). Target: arXiv cs.DB · VLDB 2027. Covers all four architectural tiers, ACID-constrained RAG pipeline, autonomous LoRA lifecycle (4-loop RLAIF), prompt engineering, 18 evidence IDs, 8 workloads (W1–W8), 23 references (IEEE+DOI), Figures 1–5 (ASCII schematics). Measured baselines: Graph 1.177 M ops/s ✅, TS 61.0 M pts/s ✅, AQL P99 9.67 ms ✅. Open: W5 isolation×faithfulness empirical run, GPU benchmarks, multi-node distributed benchmark.

### Full Draft v1.0 (Core, 2026-04-20)
- [THEMIS_MULTIMODEL_INDEX_EVALUATION_V2.md](THEMIS_MULTIMODEL_INDEX_EVALUATION_V2.md) — **v1.0 full scientific paper**: ThemisDB multi-model database — nine individualized index families (HNSW, B-tree, IVF+PQ, Graph, R-tree+Z-order, Inverted, RMI, MRL, Adaptive Advisor), formal system evaluation protocol (W1/W2/W3 workloads, 6 RQs, 12 hypotheses with acceptance criteria), operational risk model (10 failure modes, security threat model, concurrency table), threats to validity, 24 references with DOIs, full traceability appendix. arXiv cs.DB / VLDB-ready.

### Submission-Ready Draft Set (Core, 2026-04-20)
- [THEMIS_MULTIMODEL_INDEX_EVALUATION.md](THEMIS_MULTIMODEL_INDEX_EVALUATION.md) — ThemisDB multi-model database: individual index methods (HNSW, B-tree, RMI, R-tree, MRL, IVF+PQ, Graph, Inverted, Adaptive), system evaluation (latency SLA, ANN recall, QPS), and operational risk model — repository-grounded, arXiv-structured draft (v0.1)
### Submission-Ready Draft Set (Core, 2026-04-20)
- [THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md](THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md) — Systematic evaluation and risk analysis of the ThemisDB RAID-sharding system: 20-item risk taxonomy (5 dimensions), 9 Related Work subsections, full topology reference (7 RAID modes, 3 EC algorithms, consistent-hash ring, quorum model, geo-distribution), CAP/PACELC positioning table, quorum availability model, 6 fault-injection workloads, 40 references (v0.3)

### Submission-Ready Draft Set (Core, 2026-04-19)
- [DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md](DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md) — Includes RQ/Hypotheses, reporting-table plan, Threats to Validity, and claim-to-evidence appendix
- [HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md](HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md) — Includes RQ/Hypotheses, planner evaluation matrix, Threats to Validity, and claim-to-evidence appendix
- [LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md](LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md) — Includes RQ/Hypotheses, lifecycle evaluation matrix, Threats to Validity, and claim-to-evidence appendix
- [DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md](DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md) — Includes RQ/Hypotheses, serving policy evaluation matrix, Threats to Validity, and claim-to-evidence appendix
- [DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md](DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md) — Includes RQ/Hypotheses, distributed fault-evaluation matrix, Threats to Validity, and claim-to-evidence appendix
- [RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md](RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md) — Includes RQ/Hypotheses, formal validity threats, claim-to-evidence appendix, and submission readiness checklist
- [QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md](QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md) — Includes codebase finding that AQL/GraphQL are already inherently embedded, plus consolidation design (shared IR/cost model), validity, traceability, and readiness checklist

### Published & Finalized Papers
- (To be added)

### Research Drafts & Work-in-Progress
- [THEMIS_MULTIMODEL_INDEX_EVALUATION.md](THEMIS_MULTIMODEL_INDEX_EVALUATION.md) — Multi-model database index methods (nine families: HNSW, B-tree, RMI, R-tree, MRL, IVF+PQ, Graph, Inverted, Adaptive), system evaluation, and risk model
- [DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md](DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md) — ACID-constrained, database-native RAG with integrated quality evaluation
- [HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md](HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md) — Cost-aware hybrid ANN retrieval with HNSW/FAISS and planner integration
- [LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md](LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md) — Operational lifecycle and SLO-focused study for LoRA/QLoRA in DB runtime
- [DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md](DB_NATIVE_LLM_SERVING_OPTIMIZATION_PAPER_DRAFT.md) — Paged KV-cache, continuous batching, and speculative decoding in DB-native serving
- [DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md](DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md) — Distributed ACID multi-model AI database architecture and trade-off evaluation plan
- [RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md](RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md) — RAID sharding strategy and LLM distributed inference integration
- [ACID_CONSTRAINED_RAG_DRAFT.md](ACID_CONSTRAINED_RAG_DRAFT.md) — ACID transaction semantics + RAG integration with measured benchmarks
- [SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md](SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md) — Isolation-aware RAG quality/latency trade-offs under contention
- [QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md](QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md) — Unified query engine for AQL and GraphQL (submission-structured: RQ/Hypotheses, methodology, validity, traceability, readiness checklist)
- [GOSSIP_AWARE_LORA_ROUTING_DRAFT.md](GOSSIP_AWARE_LORA_ROUTING_DRAFT.md) — Federated LoRA routing via epidemic gossip protocols (submission-structured: RQ/Hypotheses, validity, traceability, readiness checklist)
- [GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md](GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md) — Domain-aware LoRA routing with capability gossip and failover (submission-structured: RQ/Hypotheses, validity, traceability, readiness checklist)
- [THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md](THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md) — Systematic evaluation and risk analysis of the Themis RAID-sharding system (20-item risk taxonomy, full topology reference [7 RAID modes, 3 EC algorithms, consistent-hash ring, quorum model, geo-distribution], CAP/PACELC positioning, 6 fault-injection workloads, 40 references — v0.3)
- [THEMIS_IT_IS_OKAY_TO_FAIL.md](THEMIS_IT_IS_OKAY_TO_FAIL.md) — Retrospektive zu Fehlentwicklungen, Fehlannahmen und Fehlentscheidungen in ThemisDB mit Korrekturprogramm und Vollständigkeitsregister. v0.7: ~98 Befunde (Kategorien A–J), Abschnitt XI mit wissenschaftlichem Kontext (24 externe Primärquellen: Lu et al. 2008, Cook 1998, Lamport 2001, Dong/RocksDB 2021 u.a.) und Literaturverzeichnis.
- [LLM_PROCESSING_OPTIMIZATION_PATTERNS.md](LLM_PROCESSING_OPTIMIZATION_PATTERNS.md) — Inference optimization patterns from llama.cpp (batching, speculative decoding, KV-cache)
- [CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md](CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md) — Scheduler/KV cache trade-offs for DB-native LLM serving
- [COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md](COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md) — Cost-based plan selection for lexical+vector+graph retrieval
- [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS_DRAFT.md](LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS_DRAFT.md) — ArXiv-structured migration of LLM integration foundations from research/
- [GPU_VECTOR_INDEXING_RESEARCH_DRAFT.md](GPU_VECTOR_INDEXING_RESEARCH_DRAFT.md) — ArXiv-structured migration of GPU vector indexing research from research/
- [PRODUCT_QUANTIZATION_RESEARCH_DRAFT.md](PRODUCT_QUANTIZATION_RESEARCH_DRAFT.md) — ArXiv-structured migration of PQ research from research/

### Financial AI & HFT Domain Research (2026-04-27)
- [HFT_RAG_LLM_THEMISDB_TRADING_ORCHESTRATION_ARXIV_2026.md](HFT_RAG_LLM_THEMISDB_TRADING_ORCHESTRATION_ARXIV_2026.md) — RAG-LLM-orchestrated high-frequency trading on ThemisDB: multi-modal financial signal processing (news, central bank comms, geopolitical risk, social media), latency budget analysis (sub-200 ms RAG loop), regulatory feasibility (MiFID II, SEC 15c3-5, EU AI Act), 31-paper related work survey, 6 open research questions (cs.AI / q-fin.TR / cs.DB)

### Planned Research Topics
- (See drafts above for current work-in-progress topics)

### Research Tooling & Method Notes
- [ARXIV_QUERY_STRATEGY_TOP4_2026-04-19.md](ARXIV_QUERY_STRATEGY_TOP4_2026-04-19.md) — Pre-search strategy and query protocol for four prioritized paper drafts
- [ARXIV_PAPER_TEMPLATE.md](ARXIV_PAPER_TEMPLATE.md) — Canonical paper template (derived from RAID paper structure) for future arXiv-ready drafts
- [`templates/MANUSCRIPT_TEMPLATE.md`](templates/MANUSCRIPT_TEMPLATE.md) — portfolio-level template for ThemisDB-authored manuscripts
- [`templates/EXPERIMENT_TEMPLATE.md`](templates/EXPERIMENT_TEMPLATE.md) — companion experiment protocol template
- [`templates/ARTIFACT_CHECKLIST.md`](templates/ARTIFACT_CHECKLIST.md) — minimum evidence checklist before promoting a manuscript

## Guidelines for Contributors

1. **Naming Convention**: Use descriptive, topic-focused filenames (e.g., `TOPIC_ARCHITECTURE_ANALYSIS.md`).
2. **Structure**: Begin with abstract, include implementation evidence from repo, add measured benchmarks where applicable.
	- Use [ARXIV_PAPER_TEMPLATE.md](ARXIV_PAPER_TEMPLATE.md) as the default structure for all new papers.
	- Use [`manuscripts/README.md`](manuscripts/README.md) to decide the target cluster for ThemisDB-authored manuscripts.
3. **Evidence Anchors**: Reference actual code files, test cases, and benchmark harnesses with line numbers.
4. **Versioning**: Track paper version/status in frontmatter or first section.
5. **Commit Early**: Papers must be committed to git to persist across sessions. Do not rely on working-tree-only edits.

### Recommended Structure for Submission Candidates

For manuscripts targeting submission readiness, include these sections explicitly:
- Research Questions and Hypotheses (3 RQs + 2 testable hypotheses)
- Reporting Tables and Figure Plan in Results
- Threats to Validity (internal, construct, external)
- Claim-to-Evidence Traceability appendix
- Submission Readiness Checklist with open/closed items

## Status Tracking

Papers should include version and status information:
- **Status**: Draft, In Review, Submitted, Published
- **Last Updated**: Date of most recent meaningful update
- **Target Venue**: (if applicable) VLDB, SIGMOD, ICDE, etc.

---

*Last Updated: 2026-08-10 (manuscript portfolio structure, templates, and high-priority manuscript seeds added)*

---

## 🗂️ Structured Research System

| Directory | Purpose |
|-----------|---------|
| [`papers/`](papers/README.md) | External scientific papers influencing ThemisDB algorithms |
| [`manuscripts/`](manuscripts/README.md) | Canonical portfolio for ThemisDB-authored manuscripts and publication drafts |
| [`best_practices/`](best_practices/README.md) | Engineering patterns from open-source & industry |
| [`architecture_decisions/`](architecture_decisions/README.md) | ADR-style records of design choices |
| [`implementation_influence/`](implementation_influence/README.md) | Cross-reference: source → module → version |
| [`stand_der_technik/`](stand_der_technik/README.md) | Quarterly state-of-the-art landscape reviews |
| [`experiments/`](experiments/README.md) | Experimental results and benchmarks |
| [`templates/`](templates/) | Templates and evidence checklists for ThemisDB-authored manuscripts |
| [`schema/`](schema/README.md) | Schema examples and YAML references |

📖 New contributor? Start with [RESEARCH_GUIDE.md](RESEARCH_GUIDE.md).
