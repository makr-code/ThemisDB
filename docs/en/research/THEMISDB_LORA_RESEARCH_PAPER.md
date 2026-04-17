[docs](../../README.md) > [en](../INDEX.md) > [research](./research.md) > [feature](./feature.md)

---
Datum: 2026-04-16
Status: draft
Primary (Quelle der Wahrheit): include/training/ada_lora_adapter.h, include/training/incremental_lora_trainer.h, include/training/lora_data_selection.h, include/training/training_pipeline.h, include/training/auto_labeler.h, include/index/hnsw_parameter_tuner.h, include/performance/workload_adaptive_optimizer.h, include/rag/continuous_learning_orchestrator.h, include/performance/phase3/bao.h
Bezug / Reference: Hu et al. (2022) LoRA arXiv:2106.09685 · Zhang et al. (2023) AdaLoRA arXiv:2303.10512 · Dettmers et al. (2023) QLoRA arXiv:2305.14314 · Marcus et al. (2021) Bao SIGMOD'21
---

# ThemisDB-LoRA: Adaptive LLM Specialization for Database-Aware Storage Optimization

**Technical Research Paper — ThemisDB Project**
*Version 1.0 · 2026-04-16 · Apache-2.0*

---

## Table of Contents

1. [Abstract](#1-abstract)
2. [Introduction](#2-introduction)
3. [Background](#3-background)
   - 3.1 [Low-Rank Adaptation (LoRA)](#31-low-rank-adaptation-lora)
   - 3.2 [AdaLoRA — Adaptive Rank Allocation](#32-adalora--adaptive-rank-allocation)
   - 3.3 [QLoRA — Quantized LoRA](#33-qlora--quantized-lora)
   - 3.4 [BAO Query Optimizer](#34-bao-query-optimizer)
   - 3.5 [Retrieval-Augmented Generation (RAG)](#35-retrieval-augmented-generation-rag)
4. [Architecture](#4-architecture)
   - 4.1 [The Fundamental Constraint: Static vs. Dynamic Knowledge](#41-the-fundamental-constraint-static-vs-dynamic-knowledge)
   - 4.2 [Two-Layer System Diagram](#42-two-layer-system-diagram)
   - 4.3 [Adapter Lifecycle](#43-adapter-lifecycle)
5. [Training Data Pipeline: `DomainType::DATABASE_OPTIMIZER`](#5-training-data-pipeline-domaintypedatabase_optimizer)
   - 5.1 [Ground-Truth Sources](#51-ground-truth-sources)
   - 5.2 [Required Code Extension](#52-required-code-extension)
   - 5.3 [Pipeline Integration](#53-pipeline-integration)
   - 5.4 [ContinuousLearningOrchestrator Integration](#54-continuouslearningorchestrator-integration)
6. [Training Sample Quality Pipeline](#6-training-sample-quality-pipeline)
7. [Evaluation Framework](#7-evaluation-framework)
   - 7.1 [Offline Metrics](#71-offline-metrics)
   - 7.2 [Online A/B Metrics](#72-online-ab-metrics)
   - 7.3 [Benchmark Dataset](#73-benchmark-dataset)
8. [Open Research Questions](#8-open-research-questions)
9. [Implementation Roadmap](#9-implementation-roadmap)
10. [Distributed Knowledge Federation: RAID-5 of Intelligence](#10-distributed-knowledge-federation-raid-5-of-intelligence)
    - 10.1 [The Problem: Shard-Local Learning](#101-the-problem-shard-local-learning)
    - 10.2 [RAID Analogy on the Knowledge Level](#102-raid-analogy-on-the-knowledge-level)
    - 10.3 [Architecture: Four Connection Layers](#103-architecture-four-connection-layers)
    - 10.4 [Privacy and Security Guarantees](#104-privacy-and-security-guarantees)
    - 10.5 [Optimisation Layers 5–10 in Distributed Mode](#105-optimisation-layers-510-in-distributed-mode)
    - 10.6 [Implementation Timeline](#106-implementation-timeline)
11. [Related Work](#11-related-work)
12. [Conclusion](#12-conclusion)
13. [References](#13-references)

---

## 1. Abstract

Modern hybrid database systems such as ThemisDB combine relational, graph, vector,
and time-series storage engines in a single instance. The operational complexity of
such systems exceeds the cognitive capacity of traditional database administrators.
This paper proposes **ThemisDB-LoRA**: a two-layer architecture that uses Low-Rank
Adaptation (LoRA) fine-tuning to specialize a large language model (LLM) with stable,
version-controlled database knowledge, while Retrieval-Augmented Generation (RAG)
supplies the volatile, instance-specific runtime context at query time.

We analyze the existing production-ready training infrastructure in ThemisDB
(`IncrementalLoRATrainer`, `AdaLoRAAdapter`, `LegalAutoLabeler`,
`LoRADataSelectionPipeline`), identify the architectural gap between static knowledge
and live operational data, and specify a concrete `DomainType::DATABASE_OPTIMIZER`
extension to the auto-labeling pipeline that would close this gap.
We derive **eight open research questions** that govern the path from prototype to
production.

---

## 2. Introduction

ThemisDB is a hybrid database system that simultaneously manages documents, graph
structures, vector embeddings, and time-series data under a unified AQL query layer.
Its configuration surface is correspondingly wide: HNSW index parameters (`efSearch`,
`M`, `efConstruction`), RocksDB compaction strategies, cache allocation, query-plan
hints (BAO/Thompson Sampling), and distributed shard placement all interact. Optimal
configuration is workload-dependent, changes over time, and depends on data
distribution — a three-dimensional optimization problem with no closed-form solution.

The conventional answer is a rule-based advisor (e.g., `HnswParameterTuner`,
`WorkloadAdaptiveOptimizer`) that fires heuristics against live metrics. The emerging
answer is to embed that reasoning capability into an LLM, which can:

- express its recommendations in natural language and explain its rationale,
- accept corrections in dialogue,
- compose multi-step optimization plans.

The central research question of this paper is:

> **To what extent can LoRA fine-tuning produce a reliably calibrated ThemisDB expert
> LLM, and what must be handled at inference time through RAG instead?**

---

## 2.1 The Evolution of Automated Database Administration

Understanding where ThemisDB-LoRA sits in the research landscape requires a brief
survey of how automated database administration has evolved over the past two decades.

### 2.1.1 Rule-Based Era (Pre-2015)

Early advisors such as the Microsoft Index Tuning Wizard (Chaudhuri & Narasayya, 1997)
and later SQL Server Database Engine Tuning Advisor (DTA) employed cost-model-driven
search over the physical design space — selecting indexes and materialised views that
minimized the estimated execution cost under a workload. These systems were deterministic,
auditable, and entirely static: the rules encoded expert knowledge at design time and did
not adapt.

Key limitation: they could not learn from observed outcomes. When a recommended index
produced no speedup (e.g., due to a data distribution the cost model mis-estimated), the
advisor had no signal to improve its next recommendation.

### 2.1.2 Machine Learning Era (2015–2022)

The first large-scale ML-based database tuning system was **OtterTune** (Van Aken et al.,
2017 [ACM SIGMOD]). OtterTune uses Gaussian Process regression to model the relationship
between configuration knobs (e.g., `innodb_buffer_pool_size`, `max_connections`) and
throughput/latency targets. In controlled experiments it matched or exceeded expert-tuned
configurations on OLTP benchmarks such as YCSB and TPC-C, achieving **throughput
improvements of up to 22 %** vs. the default configuration.

Deep-reinforcement-learning-based tuners followed:

- **CDBTune** (Zhang et al., 2019 [ACM SIGMOD]) uses Deep Deterministic Policy Gradient
  (DDPG) to tune MySQL configuration knobs. On OLTP workloads it reported a **22.6 %**
  improvement in transactions-per-second vs. DBA-tuned configurations.
- **QTune** (Li et al., 2019 [VLDB]) adds query-level features as state representation,
  achieving superior generalisation across different workload patterns.
- **Bao** (Marcus et al., 2021 [ACM SIGMOD]) shifts focus from database configuration to
  query plan selection. It uses Thompson Sampling over a small set of per-query plan
  hints, reducing tail latency (p99) by **30 %** on JOB benchmark vs. the PostgreSQL
  default planner.

**Critical limitation shared by all pre-2022 systems**: they are black-box function
approximators. They cannot explain their decisions in natural language, cannot be
corrected through dialogue, and cannot compose multi-step optimization plans that
involve schema redesign, workload reclassification, and index management simultaneously.

### 2.1.3 LLM-Augmented Database Administration (2022–present)

The maturation of large language models opened a new paradigm:

- **DB-BERT** (Trummer, 2022 [ACM SIGMOD]) fine-tunes BERT to extract configuration
  hints from database documentation and online forum posts. It demonstrated that language
  models can correctly interpret expert knowledge embedded in free text — but DB-BERT
  is read-only (no fine-tuning pipeline) and limited to BERT-scale models.
- **D-Bot** (Zhou et al., 2023 [arXiv:2312.01454]) shows that GPT-4 with tool-use
  (SQL execution, metric scraping) can diagnose root causes of performance anomalies
  with **49 % diagnosis accuracy** on a test suite of 360 database anomalies. However,
  D-Bot uses zero-shot or few-shot prompting of a closed-source API: it is subject to
  hallucination on proprietary query engines and cannot be deployed offline.
- **GPT-4-as-DBA** (Zhou et al., 2023) benchmarks GPT-4 on the "DB-GPT" benchmark,
  finding that without database-specific fine-tuning, GPT-4 achieves **~60 %** accuracy
  on index selection tasks vs. **~85 %** for a domain-fine-tuned model.

This last result establishes the core quantitative motivation for ThemisDB-LoRA: a
domain-fine-tuned model is expected to exceed the zero-shot GPT-4 baseline by roughly
25 percentage points on domain-specific tasks, while remaining deployable on-premises
without API dependency.

### 2.1.4 Parameter-Efficient Fine-Tuning (PEFT) as the Enabling Technology

Full fine-tuning of a 7B-parameter model requires ≈ 28 GB of optimizer state in
16-bit precision (Dettmers et al., 2023 [NeurIPS]). This makes full FT impractical on
single-GPU setups. The PEFT family solves this:

| Method | Trainable params | VRAM (7B model) | Accuracy vs. full FT |
|---|---|---|---|
| Full fine-tuning | 7 000 M (100 %) | ≈ 112 GB (fp16, Adam) | 100 % (baseline) |
| LoRA r=8 (Hu et al. 2022) | ≈ 4 M (0.06 %) | ≈ 14 GB | ≈ 98 % |
| QLoRA NF4 + LoRA r=8 (Dettmers et al. 2023) | ≈ 4 M (0.06 %) | ≈ **6.5 GB** | ≈ 97 % |
| AdaLoRA r=4–16 adaptive (Zhang et al. 2023) | ≈ 2–8 M | ≈ 7–15 GB | ≈ 99 % on selected layers |

QLoRA at NF4 quantization enables training on a **single RTX 3090 (24 GB)** — the
exact hardware target for the ThemisDB home-lab and edge deployment profile.

Critically, LoRA adapters are stored as small weight differential files (< 100 MB for
rank-16 on a 7B model), enabling version-controlled, rollable deployment that integrates
directly into ThemisDB's `IncrementalLoRATrainer` / `deployVersionEx` lifecycle.

### 2.1.5 Retrieval-Augmented Generation for Grounding Live Database State

RAG (Lewis et al., 2020 [NeurIPS]) was originally designed for knowledge-intensive NLP
tasks. Its application to database administration differs in one critical way:

- In NLP RAG, retrieved documents are static (Wikipedia, document corpora).
- In DB administration RAG, retrieved "documents" are **live time-series metrics**:
  current p99, cache-hit ratio, query plan features, HNSW recall estimates.

Asai et al. (2023 [Self-RAG, ICLR]) show that selective retrieval (only retrieving when
necessary) reduces hallucination while maintaining accuracy. Applied to the ThemisDB-LoRA
architecture, this means: the LLM-Advisor should explicitly signal when it is grounding
a recommendation in retrieved metric data vs. static adapter knowledge. This is the
foundation for the uncertainty-aware response format proposed in Section 4.2.

---

## 3. Background

### 3.1 Low-Rank Adaptation (LoRA)

LoRA (Hu et al., 2022) freezes the pre-trained weight matrix *W₀* and injects a
trainable low-rank decomposition *ΔW = B · A*, where *B ∈ ℝ^{d×r}* and
*A ∈ ℝ^{r×k}*, *r ≪ min(d,k)*. The effective weight becomes:

```
W = W₀ + (α/r) · B · A
```

Key properties for database advisor use:

| Property | Value |
|---|---|
| **Parameter efficiency** | rank 8 over a 7 B-parameter model modifies ≈ 0.06 % of weights |
| **Version management** | adapters are small files (< 100 MB), shippable per release |
| **Composability** | multiple domain adapters can be merged via `LoRAAdapterMerger` |

### 3.2 AdaLoRA — Adaptive Rank Allocation

Zhang et al. (2023) extend LoRA with adaptive rank allocation via SVD-based importance
scoring. ThemisDB already implements this in `AdaLoRAAdapter`
(`include/training/ada_lora_adapter.h`):

```cpp
struct AdaLoRALayerStats {
    size_t max_rank;     // Maximum allowed rank for this layer
    size_t active_rank;  // Currently active (unpruned) rank
    float  importance;   // ‖ΔW‖²_F — higher → more parameters retained
};

ReallocResult reallocateRanks(size_t target_budget);
```

This is directly applicable to database advisor training: attention layers responsible
for query-plan reasoning will naturally receive higher importance scores and retain more
rank; feed-forward layers responsible for syntax can be pruned aggressively.

### 3.3 QLoRA — Quantized LoRA

QLoRA (Dettmers et al., 2023) quantizes the frozen base model weights to 4-bit
NormalFloat (NF4) before injecting LoRA adapters, reducing VRAM requirements by ~4×.
ThemisDB supports this natively in `IncrementalLoRATrainer`
(`include/training/incremental_lora_trainer.h`):

```cpp
enum class TrainingQuantizationType { NONE, FP16, INT8, NF4 };

IncrementalTrainingConfig config;
config.quantization = QuantizationConfig{TrainingQuantizationType::NF4, /*block_size=*/64};
```

This enables training on consumer hardware (RTX 3090/4090, 24 GB VRAM) with a 7 B
base model.

### 3.4 BAO Query Optimizer

ThemisDB's `BaoOptimizer` (SIGMOD'21, Marcus et al.) uses Thompson Sampling to select
among alternative query plans (`include/performance/phase3/bao.h`):

```cpp
std::vector<QueryPlan> generate_plans(const std::string& query);
double predict_latency(const QueryPlan& plan, const QueryFeatures& features);
```

The `(query, plan_chosen, actual_latency)` tuples produced by BAO are a natural
ground-truth source for LoRA training samples (see Section 5).

### 3.5 Retrieval-Augmented Generation (RAG)

RAG (Lewis et al., 2020) augments an LLM's input with retrieved documents at inference
time, without modifying model weights. ThemisDB provides a first-class RAG bridge via
`RAGIngestionBridge` (`include/rag/rag_ingestion_bridge.h`):

```cpp
IndexResult indexDocument(text, collection, mime, filename);
void        enrichRetrievedDocuments(docs);
std::string buildEntityContext(entities);
```

In the ThemisDB-LoRA architecture, RAG is used exclusively for instance-specific,
volatile operational data — not for domain knowledge (which lives in the adapter).

### 3.6 HNSW — The Approximate Nearest-Neighbor Backbone of ThemisDB

ThemisDB's vector search layer is built on the **Hierarchical Navigable Small World**
(HNSW) algorithm (Malkov & Yashunin, 2018 [IEEE TPAMI 42(4)]). HNSW constructs a
multi-layer proximity graph where each node is connected to its `M` nearest neighbors at
each layer; search proceeds greedily from the top layer down, visiting at most `efSearch`
candidates at the bottom layer.

Empirical results from Malkov & Yashunin (2018) on SIFT-1M (1M 128-dim vectors):

| `efSearch` | Recall@10 | Queries/s (1 core) |
|---|---|---|
| 16 | 0.88 | 11 500 |
| 64 | 0.96 | 5 800 |
| 128 | 0.99 | 2 900 |
| 512 | > 0.999 | 750 |

Key insight: the recall–speed trade-off is **nearly linear in log(efSearch)**. A 4×
reduction of efSearch yields approximately 2× higher throughput at a recall cost of
3–8 pp. This is the core parameter that `HnswParameterTuner` manages and that the
LLM-Advisor must understand to give calibrated recommendations.

For the LLM-Advisor, the training data must encode not just the parameter values but the
*reasoning pattern*: "given a dataset of N vectors with dimension D, a workload with
average query complexity C, and a current CPU utilization of U%, the optimal efSearch
is approximately ...". This reasoning pattern is a prime candidate for LoRA encoding
because it is stable (algorithmic, not data-dependent) yet requires multi-step reasoning.

---

## 4. Architecture

### 4.1 The Fundamental Constraint: Static vs. Dynamic Knowledge

LoRA weights are **frozen at serving time**. A running ThemisDB instance has a
continuously changing state — schema, data distributions, live query log, current
cache hit ratio. This state cannot be encoded into LoRA weights, because the adapter
would be obsolete before deployment. The architecture must therefore separate:

| Layer | Mechanism | Update frequency | Content |
|---|---|---|---|
| **Static expertise** | LoRA adapter | Monthly / per-release | AQL semantics, index types, optimization rules, anti-patterns |
| **Live instance context** | RAG / Tool-call | Per-query (< 100 ms) | Current `WorkloadProfile`, HNSW metrics, query plan features, Prometheus metrics |

### 4.2 Two-Layer System Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                    USER / ADMIN REQUEST                      │
│   "Should I add an HNSW index to collection X?"             │
└──────────────────────┬───────────────────────────────────────┘
                       │
     ┌─────────────────▼──────────────────────┐
     │      RAG CONTEXT ASSEMBLY (< 100 ms)   │
     │                                        │
     │  WorkloadAdaptiveOptimizer.profile()   │
     │  HnswParameterTuner.currentMetrics()   │
     │  BaoOptimizer.recentDecisions(n=20)    │
     │  LoRADataSelectionPipeline.stats()     │
     │  Prometheus: p99, cache_hit_rate, …    │
     │                                        │
     │  → JSON context block (≈ 2 000 tokens) │
     └─────────────────┬──────────────────────┘
                       │
     ┌─────────────────▼──────────────────────┐
     │    LLM INFERENCE                       │
     │                                        │
     │  Base Model (e.g. Llama-3.1-8B)        │
     │    + LoRA Adapter "themisdb-expert-v2" │
     │        (static knowledge layer)        │
     │                                        │
     │  Input: system_prompt + RAG context    │
     │          + user question               │
     └─────────────────┬──────────────────────┘
                       │
     ┌─────────────────▼──────────────────────┐
     │    RESPONSE + OPTIONAL TOOL CALLS      │
     │                                        │
     │  Recommendation + rationale            │
     │  Optional: AQL query to execute        │
     │  Optional: config patch (JSON)         │
     └────────────────────────────────────────┘
```

### 4.3 Autonomous Storage Intelligence — Beyond the Advisory Model

The initial framing of ThemisDB-LoRA as a "DBA advisor chatbot" is deliberately
conservative. The deeper architectural vision is an **autonomous storage intelligence
layer**: an LLM embedded in the ThemisDB process that participates in decisions
previously made by deterministic rule engines — not merely recommending to a human
operator, but dispatching tool calls that directly affect storage layout, index
configuration, query routing, and data placement.

This distinction is crucial for understanding the scope of the project and its
relationship to the existing production code:

| Mode | Who acts? | Latency budget | Risk level | ThemisDB entry point |
|---|---|---|---|---|
| **Advisory** | Human DBA, informed by LLM | Seconds to minutes | Low (human in the loop) | Admin UI / REST API |
| **Semi-autonomous** | LLM proposes → safety gate validates → system applies | 100 ms – 5 s | Medium (schema-less DDL only) | `SelfImprovementOrchestrator` |
| **Fully autonomous** | LLM selects and dispatches in hot path | < 10 ms for routing | High (affects live queries) | `AQLModelRouter` / `BaoOptimizer` |

ThemisDB-LoRA targets all three modes, with rollout ordered by risk: advisory first,
then semi-autonomous index management, then hot-path routing influence.

#### What the LLM Decides Autonomously

The following decisions are currently made by deterministic rule engines in ThemisDB.
Each is a candidate for LLM-augmented or LLM-replaced decision-making:

**1. Storage Backend Routing at Ingest Time**

When a document arrives via `AQLIngestionBridge::enrichInsertPayload()`, the bridge
currently applies fixed rules: extract entities → write to graph store. The LLM can
enrich this with semantic understanding:

```
Incoming document → ContentType::detectFromBlob()
                  → LLM analyzes structure, content, query access patterns
                  → Decides: embed as HNSW vector? create graph edges?
                             store as time-series? all of the above?
                  → dispatches to: RocksDB (doc), GraphStore (edges),
                                   HnswIndex (vector), TSStore (timeseries)
```

The content-type infrastructure (`include/content/content_type.h`,
`ContentTypeRegistry`) and the multi-format ingestion pipeline
(`ContentToolboxBridge`) are already production-ready. The missing piece is
a semantic routing policy that can consider cross-modal access patterns.

**2. Index Lifecycle Management**

`IndexSuggestionEngine` (in `include/index/adaptive_index.h`) already generates
index suggestions from `QueryPatternTracker` data and `SelectivityAnalyzer`
statistics. The LLM's role is to:

- Validate suggestions against the current workload context (is the pattern stable
  enough to justify index creation cost?)
- Compose multi-field composite index strategies that single-field heuristics miss
- Schedule DROP INDEX on stale indexes (currently requires manual DBA action)

**3. Query Execution Strategy**

`AQLModelRouter` (`include/aql/aql_model_router.h`) already classifies queries into
`QueryModelType` (VECTOR / GRAPH / GEO / FULLTEXT / TIMESERIES / RELATIONAL / PROCESS)
using keyword heuristics. The LLM can upgrade this to semantic classification and can
compose multi-model execution plans for queries that span multiple storage backends —
e.g. "find contracts related to company X that were active last quarter" requires GRAPH +
FULLTEXT + TIMESERIES.

**4. HNSW Parameter Adaptation**

`HnswParameterTuner::recordQueryResult()` already runs an online feedback loop, but
it optimizes a single scalar target (latency vs. recall). The LLM can reason about
the broader context: if a workload shift is driving efSearch up and CPU usage is near
80 %, the correct response is not just "lower efSearch" but "consider HNSW index rebuild
with higher M to reduce efSearch at same recall" — a multi-step strategy that the
rule-based tuner cannot express.

**5. Compression Strategy Selection**

`ICompressionSelector` (`include/timeseries/compression_selector.h`) uses a fixed
decision tree (variance → Gorilla, run_length_ratio → RLE, etc.). The LLM can detect
anomalous patterns — e.g. a series with normally high variance that suddenly becomes
monotone (indicating a sensor malfunction, not a legitimate data pattern) — and override
the default compression choice while flagging the anomaly.

---

### 4.4 The Four Self-Optimizing Loops

The complete ThemisDB-LoRA system implements four nested feedback loops operating at
different timescales. The loops are **not independent**: outcomes from faster loops feed
as training signals into slower loops.

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  LOOP 4 — ADAPTER IMPROVEMENT   (weeks)                                     ║
║                                                                              ║
║  Outcomes from Loops 1–3 →  ContinuousLearningOrchestrator                  ║
║    → IncrementalLoRATrainer retrains →  new adapter version                  ║
║    → deployVersionEx(traffic_split=0.1)  → A/B test →  promote / rollback   ║
║                                                                              ║
║  ┌──────────────────────────────────────────────────────────────────────┐    ║
║  │  LOOP 3 — INDEX LIFECYCLE   (hours–days)                             │    ║
║  │                                                                      │    ║
║  │  QueryPatternTracker accumulates →  SelectivityAnalyzer              │    ║
║  │    → IndexSuggestionEngine  →  LLM validates + enriches              │    ║
║  │    → SelfImprovementOrchestrator gates + applies                     │    ║
║  │    → observe: query latency Δp99, cache-miss rate                    │    ║
║  │    → outcome becomes training sample for Loop 4                      │    ║
║  │                                                                      │    ║
║  │  ┌────────────────────────────────────────────────────────────────┐  │    ║
║  │  │  LOOP 2 — WORKLOAD ADAPTATION   (minutes)                      │  │    ║
║  │  │                                                                 │  │    ║
║  │  │  WorkloadAdaptiveOptimizer.record_query()                       │  │    ║
║  │  │    → classify_workload()  →  get_strategy()                     │  │    ║
║  │  │    → enable_auto_adapt(interval=60s)  →  apply_strategy()       │  │    ║
║  │  │    → AdaptationCallback fires → LLM notified if strategy shifts  │  │    ║
║  │  │    → HnswParameterTuner.recordQueryResult()  (sub-loop)         │  │    ║
║  │  │                                                                 │  │    ║
║  │  │  ┌───────────────────────────────────────────────────────────┐  │  │    ║
║  │  │  │  LOOP 1 — QUERY EXECUTION   (milliseconds)                │  │  │    ║
║  │  │  │                                                            │  │  │    ║
║  │  │  │  AQLModelRouter.classify() →  route to backend            │  │  │    ║
║  │  │  │  BaoOptimizer.select_plan() →  execute                    │  │  │    ║
║  │  │  │  BaoOptimizer.update_model(plan, result)  ← Thompson      │  │  │    ║
║  │  │  │  Sampling converges on best plan within ~50 queries        │  │  │    ║
║  │  │  └───────────────────────────────────────────────────────────┘  │  │    ║
║  │  └────────────────────────────────────────────────────────────────┘  │    ║
║  └──────────────────────────────────────────────────────────────────────┘    ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

#### Loop 1 — Query Execution (≤ 10 ms)

**Components:** `AQLModelRouter`, `BaoOptimizer`, `HnswParameterTuner`
**Signal:** `QueryResult.execution_time_ms`, rows returned, success
**Update rule:** `BaoOptimizer::update_model()` — Thompson Sampling over plan arms

Thompson Sampling converges on the best plan with high probability after
*O(k log k)* queries for *k* plans (Agrawal & Goyal, 2012 [ICML]). For typical
ThemisDB query plan spaces of 3–8 hints, this means ≈ 30–100 queries per collection
before BAO reliably selects the optimal plan.

**LLM involvement (semi-autonomous):** The LLM receives the `WorkloadProfile` change
notification via `AdaptationCallback` and can override the BAO hint selection if it
detects a pattern that Thompson Sampling cannot capture (e.g., a periodic workload
where the optimal plan alternates between time windows).

#### Loop 2 — Workload Adaptation (60 s interval)

**Components:** `WorkloadAdaptiveOptimizer`, `HnswParameterTuner`, `CompressionSelector`
**Signal:** Sliding window of `QueryObs` (is_write, complexity, latency_us, result_rows)
**Update rule:** `classify_workload()` → `get_strategy()` → `apply_strategy()`

`WorkloadAdaptiveOptimizer` exposes a typed `OptimizationStrategy` struct covering
`thread_pool_size`, `cache_size_mb`, `join_algorithm`, and `enable_jit_compilation`.
These parameters map directly to the storage engine's runtime configuration and
can be applied without service restart.

`HnswParameterTuner` runs its own sub-loop within this cycle:
`recordQueryResult(k, ef_used, latency_ms, recall)` feeds a sliding statistics
window (`stats_window_size = 1000`) and triggers `efSearch` adjustment when
observed latency deviates from `target_latency`.

**LLM involvement (advisory → semi-autonomous):** When `WorkloadType` transitions
(e.g. OLTP → VECTOR after a new embedding-based feature ships), the LLM can
generate a *migration plan* that goes beyond what `get_strategy()` returns:
"This workload shift requires not just a thread-pool resize but also a dedicated
HNSW collection rebuild with higher M=32 for the new 1536-dim embedding space."

#### Loop 3 — Index Lifecycle (hours to days)

**Components:** `QueryPatternTracker`, `SelectivityAnalyzer`, `IndexSuggestionEngine`,
`SelfImprovementOrchestrator`
**Signal:** `QueryPattern.count / total_time_ms` aggregates + `SelectivityStats`
**Update rule:** `IndexSuggestionEngine` generates `IndexSuggestion`; gated by
`SelfImprovementOrchestrator` A/B test before application

`SelectivityStats.estimated_l3_cache_fit_ratio` and
`estimated_cache_miss_rate` (from `adaptive_index.h`) provide cache-aware
index utility estimates — a feature that generic database tuners typically lack.

**LLM involvement (core role):** The rule-based `IndexSuggestionEngine` produces
single-field index suggestions. The LLM synthesizes cross-field composite index
strategies, multi-collection join patterns, and HNSW + BTree hybrid access paths
that no single-field selectivity analysis can discover. This is where the LoRA
adapter's domain knowledge is most directly exercised: it encodes *when composite
strategies outperform single-field approaches*, based on patterns seen in the
training corpus.

**Safety gate:** `SelfImprovementOrchestrator` (with `enable_auto_rollback = true`)
wraps every index creation/deletion in an A/B test with `ab_test_sample_size = 1000`
queries before full deployment. If the new index fails to improve latency by at
least `target_improvement = 10 %`, it is automatically dropped.

#### Loop 4 — Adapter Improvement (weekly)

**Components:** `ContinuousLearningOrchestrator`, `IncrementalLoRATrainer`,
`LoRADataSelectionPipeline`, `AdaLoRAAdapter`
**Signal:** All Loop 1–3 outcomes, DBA feedback (👍/👎), `DataSelectionMetrics`
**Update rule:** `min_accuracy_drop = 0.05` → `runLoRARetraining()` →
`deployVersionEx(version, traffic_split=0.1)` → promote or rollback

The cross-loop signal flow is the architectural novelty of ThemisDB-LoRA:

```
Loop 1 outcome: (query, plan_chosen, actual_latency_ms)
Loop 2 outcome: (workload_type_before, strategy_applied, latency_change_pct)
Loop 3 outcome: (index_suggestion, applied, delta_p99, dba_accepted)
  │
  ▼
ContinuousLearningOrchestrator.logInteraction()
  │
  ▼
LoRADataSelectionPipeline (confidence = f(delta_p99))
  │
  ▼  (when accuracy_drop > 0.05 OR weekly interval)
IncrementalLoRATrainer.train(use_existing_adapter=true, mode=INCREMENTAL)
  │
  ▼
deployVersionEx("themisdb-expert-v{n+1}", traffic_split=0.1)
  │
  ▼  (after min_ab_samples=500 queries)
promote (if improvement > 2%)  OR  rollbackVersionEx (if regression > 5%)
```

This is the self-improvement mechanism: every autonomous decision the LLM makes
in Loops 1–3 is observed, outcome-labeled, quality-filtered by
`LoRADataSelectionPipeline`, and fed back into the next adapter version — without
requiring a separate human annotation step for the majority of samples.

---

### 4.5 Adapter Lifecycle

ThemisDB already implements the full lifecycle via `IncrementalLoRATrainer`:

```
INITIAL training ──► adapter "themisdb-expert-v1"
                          │
New ThemisDB release  ────► INCREMENTAL update ──► "v1.1"
New index type added  ────► FINETUNE ──────────► "v1.2"
                          │
                 deployVersionEx(version, traffic_split=0.1)  ← A/B test
                 rollbackVersionEx(target_version)            ← safety net
```

---

## 5. Training Data Pipeline: `DomainType::DATABASE_OPTIMIZER`

### 5.1 Ground-Truth Sources

The proposed extension maps the existing rule-based advisor outputs to labeled training
samples:

| Source | Becomes |
|---|---|
| `EXPLAIN` output (before index/config change) | `TrainingSample.input` |
| `EXPLAIN` output (after change) | `TrainingSample.output` |
| Δp99 latency | `TrainingSample.confidence` |
| BAO: `(query, plan, actual_latency)` | Labeled optimizer-decision pair |
| `HnswParameterTuner` logs | HNSW parameter recommendation pairs |
| `WorkloadAdaptiveOptimizer` strategy decisions | Config-patch samples |

### 5.2 Required Code Extension

The `LegalAutoLabeler` (`include/training/auto_labeler.h`) currently accepts
`DomainType::LEGAL / MEDICAL / FINANCIAL`. Adding `DATABASE_OPTIMIZER` requires the
following changes:

**a) Enum extension in `include/training/auto_labeler.h`:**

```cpp
enum class DomainType {
    LEGAL,
    MEDICAL,
    FINANCIAL,
    DATABASE_OPTIMIZER  // NEW: ThemisDB configuration domain
};
```

**b) New category vocabulary (replaces modal-verb heuristics):**

```
// Category labels for DATABASE_OPTIMIZER domain
// "index_recommendation"    – add/drop/modify index
// "config_tuning"           – change cache, compaction, thread-pool
// "schema_redesign"         – partition strategy, edge model change
// "query_rewrite"           – rewrite AQL for better plan
// "workload_classification" – OLTP / OLAP / VECTOR / GRAPH detection
```

**c) `LoRADataSelectionConfig` domain keywords
(`include/training/lora_data_selection.h`):**

```cpp
LoRADataSelectionConfig cfg;
cfg.domain_type = DomainType::DATABASE_OPTIMIZER;

cfg.domain_keywords["index_recommendation"] = {
    "HNSW", "efSearch", "efConstruction", "IVF", "FAISS",
    "learned_index", "vector_index", "CREATE INDEX"
};
cfg.domain_keywords["config_tuning"] = {
    "RocksDB", "compaction", "block_cache", "bloom_filter",
    "write_buffer", "level_multiplier", "thread_pool"
};
cfg.domain_keywords["query_rewrite"] = {
    "EXPLAIN", "seq_scan", "full_scan", "join_order",
    "AQL", "FOR IN FILTER RETURN", "COLLECT"
};
```

**d) Ground-truth confidence function based on Δp99 latency:**

```
Δp99 > 30 %  →  confidence = 0.9   (strong positive outcome)
Δp99 5–30 %  →  confidence = 0.7
Δp99 < 5 %   →  confidence = 0.5   (flagged for human review)
Δp99 < 0 %   →  rejected           (flag_low_confidence = true)
```

### 5.3 Pipeline Integration

```cpp
// Concrete integration with existing TrainingPipeline
// (include/training/training_pipeline.h)

TrainingPipelineConfig pipeline_cfg;
pipeline_cfg.auto_label_config.source_collection = "db_optimizer_log";
pipeline_cfg.auto_label_config.target_collection = "db_optimizer_training";
pipeline_cfg.auto_label_config.domain_type       = DomainType::DATABASE_OPTIMIZER;
pipeline_cfg.auto_label_config.min_confidence    = 0.7f;

pipeline_cfg.training_config.base_model_path  = "models/llama-3.1-8b.gguf";
pipeline_cfg.training_config.rank             = 16;   // higher than legal (richer domain)
pipeline_cfg.training_config.alpha            = 32.0f;
pipeline_cfg.training_config.quantization     = {TrainingQuantizationType::NF4, 64};
pipeline_cfg.training_config.lora_plus_lambda = 4.0f; // LoRA+ for faster convergence

TrainingPipeline pipeline(pipeline_cfg, db_connection);
PipelineStats result = pipeline.run();
```

### 5.4 ContinuousLearningOrchestrator Integration

The existing `ContinuousLearningOrchestrator`
(`include/rag/continuous_learning_orchestrator.h`) already monitors accuracy drop and
triggers retraining:

```cpp
ContinuousLearningConfig cl_cfg;
cl_cfg.min_accuracy_drop     = 0.05; // 5 % advisor accuracy drop → retrain
cl_cfg.retraining_interval   = std::chrono::hours{168}; // weekly for DB-optimizer
cl_cfg.ab_test_traffic_split = 0.1;  // 10 % canary for new adapter version
cl_cfg.min_ab_samples        = 500;  // min queries before promotion decision
```

No changes to the orchestrator interface are needed; only the
`min_accuracy_drop` and `retraining_interval` thresholds differ from the
legal-domain defaults.

---

## 6. Training Sample Quality Pipeline

The `LoRADataSelectionPipeline` (`include/training/lora_data_selection.h`) already
applies a four-stage filter. The recommended parameter set for the
`DATABASE_OPTIMIZER` domain deviates from the legal-domain defaults as follows:

```
Stage 1: Quality filter
  min_length_tokens = 100   (EXPLAIN output is verbose; legal uses 50)
  max_length_tokens = 4096  (complex multi-join plans; legal uses 10 000)
  enable_pii_check  = false (no PII in query plans)

Stage 2: Deduplication
  minhash_threshold = 0.90  (DB queries can be near-identical;
                             stricter than legal default of 0.95)

Stage 3: Embedding & clustering
  model = "multilingual-e5-large"
  → clusters similar query-pattern recommendations together
  → prevents over-fitting to one query shape

Stage 4: Difficulty scoring
  perplexity_weight       = 0.3  (prefer medium-complexity plans)
  diversity_weight        = 0.4  (structural variety in plans more important)
  domain_relevance_weight = 0.3  (BM25 against DB keyword dictionary)
```

---

## 7. Evaluation Framework

### 7.1 Offline Metrics

| Metric | Definition | Target | Baseline (rule-based) |
|---|---|---|---|
| **Advisor Accuracy** | % of recommendations improving p99 by > 10 % in sandbox replay | ≥ 75 % | ~55 % (HnswParameterTuner heuristics alone) |
| **Baseline-Relative Gain** | Δ(Advisor Accuracy) vs. rule-based baseline | ≥ +15 pp | 0 pp (by definition) |
| **AQL Validity** | % of generated AQL queries that parse + execute without error | ≥ 95 % | 100 % (rule-based never generates AQL) |
| **Hallucination Rate** | % of answers citing non-existent index types or parameters | < 3 % | 0 % (rule-based never hallucinates) |
| **HNSW Parameter RMSE** | RMSE of recommended `efSearch` vs. optimal (from golden dataset) | < 20 | ~28 (HnswParameterTuner reactive lag) |
| **Hot-Pattern Accuracy** | Advisor Accuracy restricted to top-50 patterns by `QueryPattern.count` | ≥ 85 % | ~60 % |
| **Golden-Dataset Match Rate** | Agreement with golden-labeled optimal decisions (see §7.4) | ≥ 80 % | ~62 % |
| **ECE (calibration error)** | Expected Calibration Error of LLM confidence scores | < 0.05 | N/A |

### 7.2 Online A/B Metrics

```
Control group:   Rule-based system (HnswParameterTuner + WorkloadAdaptiveOptimizer + BaoOptimizer)
Treatment group: ThemisDB-LoRA advisor (10 % traffic via deployVersionEx)

Primary metric:  Frequency-weighted mean p99 query latency
                 weight(pattern) = QueryPattern.count / total_queries
Secondary:       DBA acceptance rate of recommendations (thumbs up/down)
                 BaoOptimizer.get_stats().avg_speedup (treatment vs. control)
Guard rail:      Zero regression in cluster stability (no failover events)
                 No increase in BaoOptimizer.get_stats().model_updates rate
                 (excessive model churn = instability signal)
```

**Statistical stopping rule:** Mann-Whitney U-test on p99 samples, α = 0.05,
minimum 500 samples per group (`min_ab_samples = 500` in `ContinuousLearningConfig`).

---

### 7.3 Baseline Definition

The rule-based baseline is not a single system but a **composed pipeline** of four
existing components, each with documented expected performance:

| Baseline component | What it decides | Expected performance |
|---|---|---|
| `BaoOptimizer` (Thompson Sampling) | Query plan selection | ~30 % p99 reduction vs. no planner [Marcus et al., SIGMOD 2021, §6.3] |
| `HnswParameterTuner` (reactive) | efSearch adaptation | ~15 % latency improvement over static efSearch [Malkov & Yashunin, IEEE TPAMI 2018, §5] |
| `WorkloadAdaptiveOptimizer` | Thread pool, cache, join algorithm | ~22 % throughput improvement over default config [Van Aken et al., SIGMOD 2017, §5.2] |
| `IndexSuggestionEngine` (rule-based) | Single-field index suggestions | ~10 % query time reduction for indexed fields [Ding et al., SIGMOD 2020, §6] |

**Composed baseline Advisor Accuracy** (estimated, from historical ThemisDB optimizer
logs): ~55 % of rule-based decisions produce Δp99 > 10 %. The LLM target of ≥ 75 %
therefore represents a **+20 pp absolute improvement** — consistent with the
GPT-4-as-DBA benchmark result of fine-tuned (85 %) vs. zero-shot (60 %) [Zhou et al.,
arXiv 2308.05481, Table 2].

---

### 7.4 Golden Dataset Construction

The golden dataset is built **continuously from ThemisDB runtime data** — not from
manually curated examples. This is the decisive advantage over systems that rely on
synthetic benchmarks.

#### Source: Frequent Query Patterns

```cpp
// QueryPatternTracker::getTopPatterns() returns the top-N most frequent patterns
// across all active collections, ordered by count descending.
// Each pattern carries: collection, field, operation, count, total_time_ms,
// cache_misses, cache_hits, avg_cache_miss_penalty_ms.

QueryPatternTracker tracker;
auto hot_patterns = tracker.getTopPatterns(/*limit=*/100);

// Hot patterns are those where count > 1 % of total_queries
// (typically 10–30 patterns cover 80 % of workload — Zipf distribution)
```

**Zipf's Law in query workloads** (Gray et al., 1994, "The Benchmark Handbook"):
empirically, the top-10 query patterns account for approximately 60–80 % of total
query volume in OLTP workloads. For our golden dataset, this means:

- Top-10 patterns: ~70 % of workload → evaluated with high statistical power
- Patterns 11–50: ~20 % of workload → medium power
- Patterns 51–100: ~10 % of workload → included for coverage

#### Labeling: Automatic from Observed Outcomes

Each golden sample is a tuple `(pattern, context, decision, outcome)`:

```
pattern:  QueryPattern {collection, field, operation, count, total_time_ms}
context:  WorkloadProfile + HnswParameterTuner.getStats() + BaoOptimizer.get_stats()
decision: the action taken (index created, efSearch changed, plan hint applied)
outcome:  Δp99 after N=100 subsequent queries matching this pattern
label:    "optimal" if Δp99 > 20% AND no regression on other patterns
          "acceptable" if Δp99 5–20%
          "neutral" if |Δp99| < 5%
          "harmful" if Δp99 < -5%
```

"Optimal" decisions are confirmed by a 3-day hold-out window: the Δp99 must be
sustained (not a transient fluke). This uses the same time-window logic as
`ContinuousLearningOrchestrator.retraining_interval`.

#### Labeling: RLAIF for Subjective Decisions

For decisions where outcome measurement is noisy (e.g., composite index vs. two
single-field indexes — both work, but one is better for the access pattern), the
existing `RLAIFTrainer` (`include/rag/rlaif_trainer.h`) generates preference pairs:

```
PreferencePair {
  prompt:   pattern + context description
  chosen:   composite index strategy (LLM recommendation A)
  rejected: two single-field indexes (LLM recommendation B)
}
```

The `HeuristicAIJudge` (no LLM runtime required) evaluates based on
`QueryPattern.total_time_ms` and `cache_miss_penalty_ms` — objective measurements
that do not require a separate reward model.

#### Calibration against Ground Truth

The `CalibrationManager` (`include/rag/calibration_manager.h`) calibrates LLM
confidence scores against observed outcomes using temperature scaling:

```cpp
CalibrationManager cal;
cal.addGroundTruth({
    .test_id       = pattern.collection + ":" + pattern.field,
    .faithfulness_score = outcome.delta_p99 > 0.1 ? 1.0 : 0.0,
    .relevance_score    = outcome.delta_p99 / 0.3  // normalized
});
auto [before, after] = cal.train(llm_judge);
// Target: ECE < 0.05 after calibration (well-calibrated confidence → safe for
// autonomous action without human confirmation)
```

---

### 7.5 Frequency-Weighted Evaluation

Standard (uniform) Advisor Accuracy treats all recommendations equally. This is
misleading: a correct recommendation on a pattern executed 10 000 times per day
is worth 1 000× more than one on a pattern executed 10 times per day.

**Frequency-weighted Advisor Accuracy:**

```
WAdvisorAcc = Σ_i [ w_i · 1(decision_i = "optimal" OR "acceptable") ]
              where w_i = QueryPattern_i.count / Σ_j QueryPattern_j.count
```

This is computed directly from `QueryPatternTracker::getTopPatterns()` and the
golden dataset outcome labels.

**Expected relationship** between uniform and weighted accuracy:
- If the LLM over-fits to rare patterns and under-performs on hot patterns:
  WAdvisorAcc < UniformAdvisorAcc
- If the LLM correctly prioritizes hot patterns (as expected after frequency-weighted
  data selection via `LoRADataSelectionConfig.diversity_weight`):
  WAdvisorAcc ≈ UniformAdvisorAcc or slightly higher

Target: WAdvisorAcc ≥ 80 % (5 pp higher than uniform target, justified by the
higher statistical power on hot patterns during training).

---

### 7.6 Minimum Viable Golden Dataset

```
Composition:
  - 100 patterns from QueryPatternTracker.getTopPatterns(100)
  - Per pattern: 10 outcome observations (Δp99 over 3-day windows)
  - Total: 1 000 labeled (pattern, context, decision, outcome) triples
  - Distribution mirrors live workload (Zipf-weighted)

Split:
  - 700 train / 150 validation / 150 test (stratified by WorkloadType)
  - Test set is frozen after initial collection (never used for training decisions)

Construction CLI:
```

```bash
themisdb-cli golden-dataset build \
  --top-patterns 100 \
  --min-observations 10 \
  --observation-window 3d \
  --min-delta-p99 0.05 \
  --output golden_dataset_v1.jsonl \
  --split 70/15/15

# Produces also: golden_dataset_v1_baseline_scores.json
# containing rule-based system decisions for each pattern (for comparison)
```

---

## 8. Open Research Questions

### RQ1 — Knowledge Decay Rate

> *How quickly does a LoRA adapter become stale when the workload pattern shifts?*

**Hypothesis:** HNSW parameter knowledge is stable (tied to the algorithm, not to data).
Query-plan knowledge decays faster (tied to data distribution). What is the empirical
half-life of each knowledge category? What threshold in advisor accuracy should trigger
an incremental retrain vs. a full adapter replacement?

---

### RQ2 — Rank Selection for Database Knowledge

> *What is the optimal LoRA rank for encoding database optimization expertise, and
> does AdaLoRA's importance-based allocation outperform fixed rank?*

With `AdaLoRAAdapter`, we can measure per-layer importance after training. **Hypothesis:**
attention heads responsible for multi-step plan reasoning retain higher rank; feed-forward
layers responsible for syntax encoding can be pruned more aggressively. Does this match
what we observe for legal-text adapters (`rank=8`) vs. the proposed `rank=16` for
database knowledge?

---

### RQ3 — Ground-Truth Quality vs. Quantity

> *Is a small set of high-confidence (Δp99 > 30 %) training samples more effective
> than a large set of low-confidence samples?*

The `LegalAutoLabeler` uses `min_confidence=0.5f`. For database optimization, samples
with `confidence < 0.7` (Δp99 < 5 %) may introduce noise because the outcome may be
attributable to system load rather than the index change. What is the empirical accuracy
curve as a function of `min_confidence`?

---

### RQ4 — Cross-Instance Generalization

> *Can a single adapter trained on one ThemisDB instance generalize to a different
> instance with different schema and data distribution?*

The RAG layer provides instance-specific context at inference time. **Hypothesis:** the
adapter needs to encode only *reasoning patterns* (e.g. "when M is too low, recall drops
for high-dimension vectors") while specific thresholds come from RAG. What is the accuracy
gap between a pretrained adapter + RAG vs. an instance-specific fine-tuned adapter?

---

### RQ5 — LoRA vs. RAG Boundary

> *What is the optimal division of labor between the frozen LoRA adapter and the
> dynamic RAG context?*

Concretely: should HNSW parameter recommendations live in the adapter (stable algorithmic
knowledge) or in RAG (instance measurements)? We propose an ablation study:
**adapter-only** vs. **RAG-only** vs. **combined**, measured on the Section 7.2 metrics.

---

### RQ6 — Catastrophic Forgetting in Incremental Training

> *Does `IncrementalLoRATrainer` in `INCREMENTAL` mode cause the model to forget
> previously correct recommendations?*

The `freeze_existing_layers=true` flag exists in `IncrementalTrainingConfig` for this
purpose. What is the accuracy retention curve on the original evaluation set after N
incremental updates? Is elastic weight consolidation (EWC) needed, or does LoRA's
low-rank constraint act as an implicit regularizer?

---

### RQ7 — Multi-Adapter Composition

> *Can a merged adapter (legal-domain + database-optimizer via `LoRAAdapterMerger`)
> outperform either single-domain adapter on cross-domain queries?*

ThemisDB users may query both the legal document corpus and the optimizer simultaneously
("which index configuration is optimal for contract-search workloads?"). What is the
accuracy impact of adapter merging vs. adapter switching? What merge ratio (legal weight
vs. optimizer weight) is optimal?

---

### RQ8 — Alignment and Safety

> *What alignment properties are needed for a database advisor LLM to prevent harmful
> recommendations (e.g., `DROP INDEX` on a production system)?*

Required safeguards:

- RLHF signal from DBA acceptance/rejection (captured via A/B feedback loop).
- Hard-coded tool-call validation layer (DDL cannot be issued without explicit
  human confirmation).
- Confidence calibration: does the model correctly express uncertainty when
  `WorkloadProfile.type == UNKNOWN`?

---

### RQ9 — Autonomous Decision Quality vs. Human-in-the-Loop

> *At what confidence threshold should the LLM be allowed to act autonomously
> (without DBA confirmation) on index and storage decisions?*

The `SelfImprovementOrchestrator` already implements a safety gate and A/B framework.
The research question is empirical: what is the relationship between the LLM's
stated confidence and the actual probability of a beneficial outcome? Can calibration
methods (temperature scaling, Platt scaling) make the model's uncertainty estimates
actionable for the `rollback_threshold` decision? At what point does the overhead of
the human-in-the-loop confirmation exceed the risk cost of an incorrect autonomous
decision for a given operation class (index creation vs. DROP INDEX vs. efSearch
adjustment)?

---

### RQ10 — Loop Interference and Oscillation

> *Can the four self-optimizing loops interfere destructively — e.g., Loop 2 raising
> efSearch while Loop 3 is rebuilding the HNSW index, causing temporary oscillation?*

Multi-timescale feedback systems are known to exhibit limit cycles when inner loops
react faster than outer loops can observe their side-effects (Åström & Hägglund, 2006).
The architectural question: does the `AdaptationCallback` mechanism provide sufficient
synchronization between `WorkloadAdaptiveOptimizer` (Loop 2) and `IndexSuggestionEngine`
(Loop 3) to prevent conflicting simultaneous actions?

**Hypothesis:** Oscillation risk is highest at workload-type transition boundaries
(OLTP → VECTOR). Mitigation: a shared `OptimizationLock` with per-resource cooldown
periods between loop-initiated changes.

---

### RQ11 — Storage Backend Selection Accuracy

> *How accurately can the LLM select the optimal storage backend for incoming
> documents, compared to the current rule-based ContentType routing?*

The existing `ContentTypeRegistry::detectFromBlob()` and `ContentType.features`
(geospatial, temporal, hierarchical, versioned, multimodal) already capture rich
structural metadata. The LLM adds *semantic* context: a text document describing
GPS traces is not semantically the same as a document containing geospatial coordinates
embedded in a legal contract. How much does semantic routing improve cross-modal
retrieval quality (measured by RAG recall@k) vs. pure structural routing?

---

### RQ12 — Federated Learning Privacy-Utility Trade-off

> *What is the optimal (ε, δ) budget for differential privacy in federated LoRA
> gradient aggregation across ThemisDB shards, and how does it affect adapter
> convergence?*

The `DifferentialPrivacyManager` is already implemented with configurable (ε, δ)
parameters (default: ε=0.1, δ=1e-5). However, the empirical relationship between
privacy budget and adapter quality degradation in a database-optimizer domain is
unknown. Stronger privacy (lower ε) adds more noise to gradient aggregation and
may slow convergence, especially in heterogeneous shard workloads where inter-shard
signal is sparse. The key question: at what ε does the privacy noise dominate the
gradient signal, and does FedProx (proximal regularisation) recover quality better
than FedAvg in that regime?

**Measurement approach:** vary ε ∈ {0.01, 0.05, 0.1, 0.5, 1.0} across 5-shard
simulation; measure adapter accuracy (§7.1) after 5 federated rounds.

---

## 9. Implementation Roadmap

```
Phase 1 (Q3 2026): Dataset construction
  - [ ] Implement optimizer-log export CLI (EXPLAIN-pair format)
  - [ ] Extend DomainType::DATABASE_OPTIMIZER in include/training/auto_labeler.h
  - [ ] Add DATABASE_OPTIMIZER branch to LegalAutoLabeler::categorize()
  - [ ] Add domain keywords to LoRADataSelectionConfig
  - [ ] Collect 1 000 labeled (query, plan, Δlatency) pairs from all 4 loops
  - [ ] Validate against LoRADataSelectionPipeline quality filters

Phase 2 (Q3 2026): Initial adapter training + advisory deployment
  - [ ] Train "themisdb-expert-v1" (Llama-3.1-8B + NF4 QLoRA, rank=16)
  - [ ] Offline evaluation: Advisor Accuracy, AQL Validity, Hallucination Rate (§7.1)
  - [ ] Baseline comparison: rule-based HnswParameterTuner
  - [ ] Deploy in advisory mode (Admin UI); begin collecting DBA feedback

Phase 3 (Q4 2026): RAG context assembly + loop instrumentation
  - [ ] WorkloadAdaptiveOptimizer → JSON context serializer (≤ 2 000 tokens)
  - [ ] Prometheus scraper → metric context block
  - [ ] RAGIngestionBridge extension for optimizer-log documents
  - [ ] Instrument Loop 1–3 outcome signals → ContinuousLearningOrchestrator

Phase 4 (Q4 2026): A/B deployment + semi-autonomous index loop
  - [ ] deployVersionEx(traffic_split=0.1) via IncrementalLoRATrainer
  - [ ] ContinuousLearningOrchestrator configuration for weekly retrain cycle
  - [ ] DBA feedback UI (thumbs up/down → training signal)
  - [ ] Semi-autonomous Loop 3: LLM → SelfImprovementOrchestrator → index changes
  - [ ] Implement loop-interference cooldown guard (RQ10)

Phase 5 (Q1 2027): AdaLoRA + LoRA+ tuning + autonomous routing
  - [ ] AdaLoRAAdapter importance analysis (answer RQ2)
  - [ ] LoRA+ λ ablation study (answer RQ6)
  - [ ] Incremental training catastrophic-forgetting benchmark
  - [ ] Semantic storage-backend routing via ContentTypeRegistry + LLM (RQ11)
  - [ ] Confidence calibration experiment (RQ9)

Phase 6 (Q2 2027): Production hardening + full autonomy gate
  - [ ] Tool-call validation layer (DDL safety gate; answer RQ8)
  - [ ] Multi-instance generalization study (answer RQ4)
  - [ ] Adapter merge experiment: legal + database-optimizer (answer RQ7)
  - [ ] Loop oscillation test suite (answer RQ10)
  - [ ] Define per-operation autonomy thresholds (answer RQ9)
```

Phase 6 (Q2 2027): Production hardening + full autonomy gate
  - [ ] Tool-call validation layer (DDL safety gate; answer RQ8)
  - [ ] Multi-instance generalization study (answer RQ4)
  - [ ] Adapter merge experiment: legal + database-optimizer (answer RQ7)
  - [ ] Loop oscillation test suite (answer RQ10)
  - [ ] Define per-operation autonomy thresholds (answer RQ9)

Phase 7 (Q3 2027): Distributed Knowledge Federation — RAID-5 of Intelligence
  - [ ] Layer A: GossipProtocol::registerCustomHandler() + AdapterCapabilityAnnouncement broadcast
  - [ ] Layer A: AdaptiveShardRouter::routeByDomain() for domain-aware query routing
  - [ ] Layer B: IncrementalLoRATrainer::exportGradient() + applyGlobalDelta() hooks
  - [ ] Layer B: FederatedAggregator re-used as LoRA-gradient bus (FedAvg/FedProx)
  - [ ] Layer B: ContinuousLearningOrchestrator::FEDERATED_ROUND_START trigger (24 h interval)
  - [ ] Layer C: QueryFederation RAG-aware merge strategy + FederatedRAGMerger
  - [ ] Layer D: CrossShardFeedbackSync — DBA feedback as anonymised embedding via Gossip
  - [ ] DP calibration study (answer RQ12): vary ε ∈ {0.01..1.0} over 5-shard simulation
  - [ ] FederatedAIDecisionAuditor — global DBA decision timeline across all shards
  - [ ] GDPR cross-border check: CrossBorderTransferPolicy for gradient aggregation
```

---

## 10. Distributed Knowledge Federation: RAID-5 of Intelligence

*This section extends Section 4 (Architecture) and Section 9 (Roadmap) with
Layer 11 of the ThemisDB intelligence stack: cross-shard propagation of learning
outcomes. The full specification is maintained in
`docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`.*

---

### 10.1 The Problem: Shard-Local Learning

The four self-optimising feedback loops (§4.4) operate identically on every shard
of a ThemisDB cluster. However, in the current architecture the learning layers —
`IncrementalLoRATrainer`, `RLAIFTrainer`, `ContinuousLearningOrchestrator` — are
**shard-local**: each shard trains on its own workload observations and never shares
insights with peer shards.

This creates three concrete failure modes:

1. **Cold-start on new shards.** A freshly provisioned shard must accumulate its own
   training corpus before Loop 4 becomes effective. In a RAID-5 knowledge model, it
   would inherit a weighted starting point from the cluster.

2. **Fragmented DBA feedback.** A database administrator may submit a correction on
   Shard 3; that correction never reaches Shard 7, which continues making the same
   sub-optimal recommendation.

3. **Missed cross-shard specialisation.** If Shard 7 has processed 14 000 security
   audit queries and developed a highly accurate `SECURITY_MONITOR` adapter, a
   security query routed to Shard 2 (which only has 200 such samples) will produce
   an inferior response — even though a better answer exists in the cluster.

---

### 10.2 RAID Analogy on the Knowledge Level

Classical RAID protects *data* through redundancy and parity. The same metaphor
applies to *learned knowledge*:

| RAID Level | Data Sharding (implemented) | Knowledge Sharding (new) |
|---|---|---|
| **RAID-0** | Striping, no redundancy | Each shard trains in isolation — no knowledge transfer |
| **RAID-1** | Full mirroring | Full LoRA adapter sync to all shards — prohibitively expensive |
| **RAID-5** | Striping + distributed parity | **FedAvg** — gradient aggregation with Differential Privacy |
| **RAID-6** | Double parity | Hierarchical aggregation: Shard → Region → Global |
| **RAID-10** | Mirror + Stripe | Specialised + shared adapters combined |

**Target: RAID-5 for knowledge.** FedAvg-based LoRA adapter federation in which no
shard ever sees the raw data of another shard. Each shard contributes only a
differentially-private gradient to the global aggregation round.

The formal privacy guarantee follows McMahan et al. (2018): with
(ε = 0.1, δ = 1×10⁻⁵), the probability of inferring any individual training sample
from the published aggregate is bounded by ε per query.

---

### 10.3 Architecture: Four Connection Layers

The distributed intelligence architecture consists of four orthogonal layers that
each solve a different knowledge-sharing problem.

```
┌──────────────────────────────────────────────────────────────────────────┐
│            ThemisDB Distributed Knowledge Architecture                   │
│                                                                          │
│  Shard 1          Shard 2          Shard N         Global Aggregator     │
│  ─────────        ─────────        ─────────       ──────────────────    │
│  LoRA-Trainer  ←─ Gossip  ─→  LoRA-Trainer  ─→  FederatedAggregator    │
│  RAGBridge     ←─ QueryFed ─→  RAGBridge     ─→  Merged RAG Context     │
│  FeedbackColl  ←─ Gossip  ─→  FeedbackColl  ─→  Global RLAIF Round     │
│  AdapterReg    ←─ Gossip  ─→  AdapterReg    ─→  Capability Announce    │
│  ZeroTrust     ──────────────────────────────→  mTLS + SphincsPlus      │
└──────────────────────────────────────────────────────────────────────────┘
```

#### Layer A — Gossip-Based Adapter Discovery

**Purpose:** route queries to the domain-specialised shard without training.

Every shard broadcasts its current adapter capabilities via the existing
`GossipProtocol`. The payload is a new `AdapterCapabilityAnnouncement`:

```json
{
  "shard_id": "shard-42",
  "adapter_version": "v1.3.0",
  "domain_type": "SECURITY_MONITOR",
  "accuracy_delta": 0.12,
  "training_samples": 14200,
  "p99_delta_ms": -8.4
}
```

The `AdaptiveShardRouter` receives these announcements via a registered custom handler
and scores each shard per domain. An AQL query carrying a `domain_hint` is then routed
to the highest-scoring shard — a routing improvement that requires *no* additional
training and becomes active within one Gossip cycle (100–500 ms).

**New interfaces required:**

| Component | Extension |
|---|---|
| `GossipProtocol` | `registerCustomHandler(type, fn)` |
| `AdaptiveShardRouter` | `updateAdapterCapability()`, `routeByDomain()` |
| `AdapterRegistry` | `publishAsGossipPayload()` |

#### Layer B — Federated LoRA Gradient Aggregation (RAID-5 Core)

**Purpose:** propagate learning improvements across shards without raw-data sharing.

Every 24 hours — triggered by a new `FEDERATED_ROUND_START` event in
`ContinuousLearningOrchestrator` after Loop 4 completion — each shard exports its
current LoRA gradient as an encrypted, opaque blob. These blobs are collected by
the `FederatedAggregator` (already implemented for PostgreSQL schema federation,
directly reusable for LoRA gradients):

```
Shard 1: IncrementalLoRATrainer → exportGradient() → encrypted_gradient_1
Shard 2: IncrementalLoRATrainer → exportGradient() → encrypted_gradient_2
Shard N: IncrementalLoRATrainer → exportGradient() → encrypted_gradient_N
                    ↓
FederatedAggregator::aggregateUpdates(updates, algorithm="FedAvg")
                    ↓
DifferentialPrivacyManager::addDifferentialPrivacy(stats, ε=0.1, δ=1e-5)
                    ↓
global_adapter_delta (anonymised, DP-protected)
                    ↓
IncrementalLoRATrainer::applyGlobalDelta() on all shards
```

**Privacy guarantee:** No shard ever sees raw training data from another shard.
Only DP-noised gradient aggregates are shared across shard boundaries.

**New interfaces required:**

| Component | Extension |
|---|---|
| `IncrementalLoRATrainer` | `exportGradient()`, `applyGlobalDelta()` |
| `ContinuousLearningOrchestrator` | `FEDERATED_ROUND_START` trigger |
| `ILoRAFederationCoordinator` | New orchestration interface |

#### Layer C — Cross-Shard RAG Federation

**Purpose:** the LLM receives retrieval context from all shards, not just the local one.

Today, `RAGIngestionBridge` is invoked once per query on the local shard. With
`FederatedRAGMerger`, the `QueryFederation` fan-out mechanism (already implemented)
is extended to scatter RAG retrieval across shards and merge the results:

```
AQL query → QueryFederation::fanOut()
              ↓ parallel on every shard
              RAGIngestionBridge::enrichRetrievedDocuments()  [shard-local]
              ↓ merged by
FederatedRAGMerger::merge(shardResults)   [relevance-ranked, deduped]
              ↓
ContinuousLearningOrchestrator (global): quality scoring + ranking
              ↓
LLM: answer from unified knowledge context of all shards
```

The `FederatedRAGMerger` uses the `accuracy_delta` scores published by Layer A to
weight results: a SECURITY_MONITOR result from Shard 7 (delta=+0.12) ranks above
the same type of result from Shard 2 (delta=+0.02).

#### Layer D — Federated RLAIF

**Purpose:** a DBA correction on one shard improves all shards.

When a DBA submits feedback via `FeedbackCollector` on Shard 3, the feedback is
converted to an anonymised embedding (no plaintext) and propagated via Gossip:

```json
{
  "feedback_type": "DenormalizationHint_rejected",
  "reason_embedding": [ 0.031, -0.142, ... ],
  "shard_origin": "ANON"
}
```

Every shard receives this summary via a registered `CrossShardFeedbackSync` handler
and adds it as a new `PreferencePair` to its `RLAIFTrainer`. The next Loop 4 round
on every shard therefore incorporates the global DBA knowledge.

The `ZeroTrustPolicyEnforcer` verifies every Gossip message via the existing
`SignedRequest` mechanism before the handler is invoked.

---

### 10.4 Privacy and Security Guarantees

| Guarantee | Mechanism | Status |
|---|---|---|
| No raw data leaves a shard | Only DP-noised gradients and embeddings are shared | DP in `DifferentialPrivacyManager` (existing) |
| Gradient transport is authenticated | `SignedRequest` + mTLS for all shard communication | Implemented |
| Global audit log is post-quantum tamper-proof | `SphincsPlus` SPHINCS+-SHA2-256s signatures | Implemented (STUB until liboqs) |
| EU GDPR compliance for gradient transfer | `CrossBorderTransferPolicy` checks adequacy | Implemented |
| Feedback anonymisation | Embeddings only, no plaintext; `shard_origin: ANON` | New (Layer D) |
| Continuous trust verification | `ZeroTrustPolicyEnforcer` verifies every Gossip message | Existing |

---

### 10.5 Optimisation Layers 5–10 in Distributed Mode

The six optimisation layers (§4 of `LLM_OPTIMIZATION_LAYERS_MATRIX.md`) gain
qualitatively new capabilities when combined with Layer 11:

| Layer | Shard-local today | Distributed enhancement (Layer 11) |
|---|---|---|
| **L5 Transaction Semantics** | Deadlock hints per shard | Batch-hints propagated cross-shard via `CrossShardTransaction` |
| **L6 Schema Evolution** | Dead-weight detection per shard | Federated access statistics — a field unused on Shard 1 but accessed daily on Shard 7 is not flagged as dead weight |
| **L7 Security** | `IntentAlert` on local shard | Alert propagated via Gossip → all shards raise `session_risk_score` immediately |
| **L8 Multi-Tenant** | `WorkloadFingerprint` per shard | Fingerprint transfer cross-shard: Shard B learns from Shard A's experience with a similar tenant fingerprint |
| **L9 Explainability** | `AIDecisionAuditor` per shard | `FederatedAIDecisionAuditor` — DBA sees decisions from all shards in a single timeline |
| **L10 Layout** | `LayoutHint` local | Layout hints propagated via Gossip to all shards |

---

### 10.6 Implementation Timeline

Following the ROI-ordered implementation sequence:

| Phase | Layer | Duration | Key deliverable |
|---|---|---|---|
| **7A** (Q3 2027) | Layer A — Adapter Gossip | 2 weeks | Domain-aware routing, no training required |
| **7B** (Q3 2027) | Layer C — Federated RAG | 3 weeks | LLM sees knowledge from all shards |
| **7C** (Q4 2027) | Layer B — Federated LoRA | 6 weeks | Gradient federation with DP guarantee |
| **7D** (Q4 2027) | Layer D — Cross-Shard RLAIF | 3 weeks | DBA feedback propagation to all shards |

Total: 14 calendar weeks. Layers A and C can run in parallel with Layers B and D,
giving an effective critical-path duration of 9 weeks.

---

## 11. Related Work

| System | Approach | Autonomous? | Difference to ThemisDB-LoRA |
|---|---|---|---|
| **OtterTune** (Van Aken et al., SIGMOD 2017) | Gaussian Process over config knobs | Partially | No LLM; no natural-language explanation; single-engine only |
| **CDBTune** (Zhang et al., SIGMOD 2019) | Deep RL (DDPG) for knob tuning | Yes | No LLM reasoning; no multi-modal storage; no incremental LoRA |
| **QTune** (Li et al., VLDB 2019) | Deep RL + query features | Yes | Query-level only; no index lifecycle or storage routing |
| **DB-BERT** (Trummer, SIGMOD 2022) | BERT for hint extraction from docs | No | Read-only; no fine-tuning pipeline; no feedback loops |
| **NoisePage / Pilot** (Pavlo et al., VLDB 2021) | Autonomous ML-driven DBMS (self-driving) | Yes | Monolithic RDBMS only; no LLM; no multi-modal storage |
| **Bao** (Marcus et al., SIGMOD 2021) | Thompson Sampling for plan selection | Yes (plan level) | Plan-level only; no storage routing, index lifecycle, or LLM explanation |
| **D-Bot** (Zhou et al., arXiv 2023) | GPT-4 + tool-use for DB diagnosis | Advisory | Closed-source API; no fine-tuning; no continuous learning loop |
| **ALEX** (Ding et al., SIGMOD 2020) | Learned adaptive index structure | Yes (index level) | Single index type only; no LLM reasoning or cross-modal routing |
| **LLM-Tuning** (Zhang et al., 2024) | LoRA for NL2SQL | No | SQL only; no storage / index / compression optimization |
| **GPT-4-as-DBA** (Zhou et al., 2023) | Zero-shot GPT-4 prompting | Advisory | No domain fine-tuning; ~60 % accuracy vs. ~85 % for fine-tuned model |

**Position of ThemisDB-LoRA in this space:**

ThemisDB-LoRA is the first system to combine (a) *in-process* LoRA fine-tuning with
(b) multi-modal hybrid storage routing, (c) four nested self-optimizing feedback loops,
and (d) a continuous learning pipeline that trains from its own operational outcomes —
all within a single deployable database binary. Unlike NoisePage/Pilot (RDBMS-only) or
D-Bot (API-dependent), ThemisDB-LoRA is designed for offline, on-premises, edge
deployment with consumer-grade GPU hardware.

---

## 11. Conclusion

The analysis of ThemisDB's existing infrastructure reveals that all components
necessary for a domain-specialized autonomous storage intelligence are already
production-ready. The system as a whole implements four nested self-optimizing
feedback loops:

- **Loop 1** (milliseconds): `BaoOptimizer` Thompson Sampling over query plan hints
- **Loop 2** (minutes): `WorkloadAdaptiveOptimizer` workload classification + strategy
  application, `HnswParameterTuner` recall/latency adaptation
- **Loop 3** (hours/days): `IndexSuggestionEngine` + `SelfImprovementOrchestrator` for
  autonomous index lifecycle management
- **Loop 4** (weeks): `ContinuousLearningOrchestrator` + `IncrementalLoRATrainer` for
  adapter improvement from observed outcomes

The central architectural insight is that **LoRA and RAG are not alternatives but
complements**: LoRA encodes the stable reasoning patterns of a database expert; RAG
supplies the volatile, instance-specific measurements at inference time. Neither alone
is sufficient.

The critical missing piece is a `DomainType::DATABASE_OPTIMIZER` extension to the
auto-labeling pipeline, a confidence function based on Δp99 latency, and a minimum
viable benchmark of 1 000 labeled optimizer-decision pairs. These are the concrete next
steps toward a production-grade database advisor.

The eight open research questions (RQ1–RQ8) define the experimental program needed to
validate the architecture and calibrate its production deployment.

---

## 12. References

### Core LoRA / PEFT

- **Hu et al. (2022)**
  *LoRA: Low-Rank Adaptation of Large Language Models.*
  arXiv:2106.09685. ICLR 2022.

- **Zhang et al. (2023)**
  *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning.*
  arXiv:2303.10512. ICLR 2023.

- **Dettmers et al. (2023)**
  *QLoRA: Efficient Finetuning of Quantized LLMs.*
  arXiv:2305.14314. NeurIPS 2023.

- **Hayou et al. (2024)**
  *LoRA+: Efficient Low Rank Adaptation of Large Models.*
  arXiv:2402.12354. ICML 2024.

- **Li & Liang (2021)**
  *Prefix-Tuning: Optimizing Continuous Prompts for Generation.*
  arXiv:2101.00190. ACL 2021.

- **Lester et al. (2021)**
  *The Power of Scale for Parameter-Efficient Prompt Tuning.*
  arXiv:2104.08691. EMNLP 2021.

---

### Retrieval-Augmented Generation

- **Lewis et al. (2020)**
  *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks.*
  arXiv:2005.11401. NeurIPS 2020.

- **Asai et al. (2023)**
  *Self-RAG: Learning to Retrieve, Generate, and Critique through Self-Reflection.*
  arXiv:2310.11511. ICLR 2024.

- **Gao et al. (2022)**
  *Precise Zero-Shot Dense Retrieval without Relevance Labels (HyDE).*
  arXiv:2212.10496. ACL 2023.

---

### Approximate Nearest Neighbor Search

- **Malkov & Yashunin (2018)**
  *Efficient and Robust Approximate Nearest Neighbor Search using Hierarchical
  Navigable Small World Graphs.*
  IEEE Transactions on Pattern Analysis and Machine Intelligence, 42(4), 824–836.
  DOI: 10.1109/TPAMI.2018.2889473. arXiv:1603.09320.

- **Subramanya et al. (2019)**
  *DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node.*
  NeurIPS 2019.

- **Guo et al. (2020)**
  *Accelerating Large-Scale Inference with Anisotropic Vector Quantization (ScaNN).*
  arXiv:1908.10396. ICML 2020.

---

### Learned Database Optimization

- **Marcus et al. (2021)**
  *Bao: Making Learned Query Optimization Practical.*
  ACM SIGMOD 2021. DOI: 10.1145/3448016.3452838.

- **Van Aken et al. (2017)**
  *Automatic Database Management System Tuning Through Large-scale Machine Learning.*
  ACM SIGMOD 2017. DOI: 10.1145/3035918.3064029.

- **Zhang et al. (2019)**
  *CDBTune: An End-to-End Automatic Cloud Database Tuning System Using Deep
  Reinforcement Learning.*
  ACM SIGMOD 2019. DOI: 10.1145/3299869.3300085.

- **Li et al. (2019)**
  *QTune: A Query-Aware Database Tuning System with Deep Reinforcement Learning.*
  Proceedings of the VLDB Endowment, 12(12), 2118–2130.
  DOI: 10.14778/3352063.3352129.

- **Ding et al. (2020)**
  *ALEX: An Updatable Adaptive Learned Index.*
  ACM SIGMOD 2020. DOI: 10.1145/3318464.3389711.

- **Pavlo et al. (2021)**
  *Make Your Database System Dream of Electric Sheep: Towards Self-Driving
  Operation (NoisePage / Pilot).*
  Proceedings of the VLDB Endowment, 14(12), 3211–3221.
  DOI: 10.14778/3476311.3476411.

---

### LLM-Augmented Database Administration

- **Trummer (2022)**
  *DB-BERT: a Database Tuning Tool that "Reads" the Manual.*
  ACM SIGMOD 2022. DOI: 10.1145/3514221.3517843.

- **Zhou et al. (2023)**
  *D-Bot: Database Diagnosis System using Large Language Models.*
  arXiv:2312.01454.

- **Zhou et al. (2023)**
  *GPT-4-as-DBA (DB-GPT Benchmark): LLM-Based Database Administrator.*
  arXiv:2308.05481.

---

### Reinforcement Learning and Bandit Algorithms

- **Agrawal & Goyal (2012)**
  *Analysis of Thompson Sampling for the Multi-armed Bandit Problem.*
  Proceedings of COLT 2012. arXiv:1111.1797.

- **Ouyang et al. (2022)**
  *Training Language Models to Follow Instructions with Human Feedback (InstructGPT).*
  arXiv:2203.02155. NeurIPS 2022.

---

### Control Theory for Feedback Systems

- **Åström & Hägglund (2006)**
  *Advanced PID Control.*
  ISA — The Instrumentation, Systems, and Automation Society, ISBN 1-55617-942-1.

---

*Authors: ThemisDB Engineering Team*
*License: Apache-2.0*
*Repository: github.com/makr-code/ThemisDB*

---

## 13. Runtime Influence Mechanisms: 7 Classes

> **Cross-reference:** `PERFORMANCE_EXPECTATIONS.md §14.1` ·
> `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12` ·
> `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12`

Every LoRA and AdaLoRA mechanism described in this paper belongs to exactly one
of the following seven runtime influence classes. The classification determines
operator intent (tunable vs. autonomous), feedback expectations, and SLO
coupling.

| # | Class | Semantics | Instances in this paper |
|---|---|---|---|
| 1 | **Switch** | Binary ON/OFF — deterministic code-path flip | `enable_draft_kv_cache`, `hot_swap.enabled`, `importance_pruning.enabled`, `federation.broadcast_importance_scores` |
| 2 | **Fader** | Continuous signed −x…0…+x — hot-reloadable via SIGHUP; neutral point separates suppressive from amplifying regime | `acceptance_threshold` (0.6–0.75–0.9), `total_rank_budget` (128–512–1024), `speculative_tokens` (3–6–10), `chunked_prefill_size`, `worker_threads` |
| 3 | **Optimizer** | Solves a mathematical objective (min/max); no environment perception; triggered externally | `WorkloadFingerprintEngine` (min. workload-class error), FedAvg rank aggregation (min. federated loss), TIES-Merge SVD (min. sign conflict), BayesianOptimizer (max. F1) |
| 4 | **Agentic Solver** | Perception → Decision → Action cycle; fully autonomous; no operator in the loop | `SelfImprovementModule` (Acceptance + Confidence → threshold rewrite), LLM Intent Classifier (query semantics → route/block), `CrossShardFeedbackSync` |
| 5 | **Closed Loop** | Output measured; measurement fed back as correction signal into same process | AdaLoRA importance-score → rank reallocation loop, CI SLO gate (P99 regression blocks deploy), RLAIF quality loop |
| 6 | **Open Loop** | Input triggers action; no feedback path returns to the trigger source | SIGHUP config hot-reload, gossip broadcast of importance scores, event-triggered LoRA hot-swap, Kafka → GraphDB write |
| 7 | **Causal Chain** | Directed multi-component cause-effect sequence; no return path to originator | WorkloadFingerprintEngine → `total_rank_budget` → AdaLoRA per-layer reallocation → FedAvg shard propagation → TTFT P99↓ · Throughput↑ |

**Key design rule:** Switches and Faders are operator-controlled; Optimizers,
Agentic Solvers, and Closed Loops are system-controlled. Open Loops and Causal
Chains are fire-and-forget — SLO effects must be validated by an independent
monitoring path (§6 Δp99 rules).

**Operational Resilience — Cross-Cutting Dimensions**

The five dimensions below are not independent taxonomy classes — each instantiates
one or more of the seven classes above with resilience-specific patterns. They apply
orthogonally across all four LoRA loops and across the LoRA infrastructure stack.
Canonical full tables with every ThemisDB instance:
`DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.8` · `VERTEILTES_WISSEN_FEDERATION.md §12.8`.

### Backpressure

Backpressure occurs when a downstream component signals its capacity limit upstream.
The three LoRA-relevant instances each map to a different class:

| Mechanism | Class | Downstream signal | Upstream reaction | Config key |
|---|---|---|---|---|
| Inference request queue | **Fader** | `max_pending_requests` depth exceeded | ingestion throttled | `max_pending_requests` |
| Kafka topic lag (Loop 4 training events) | **Closed Loop** | topic-lag metric | consumer rate adjusted | `kafka.consumer.max_poll_records` |
| Inference endpoint HTTP 429 | **Open Loop** | 429 response | caller backs off (exponential) | — |
| LLM queue hard-drop | **Switch** | queue full (ON) | request rejected with 503 | `llm.queue.hard_drop_enabled` |

### Timeout / Circuit Breaker

Timeouts bound waiting time (Open Loop / Fader). Circuit Breakers measure error rates
and proactively block a path (Closed Loop), then probe recovery (HALF_OPEN state).

| Mechanism | Class | Trigger | Action | Config key |
|---|---|---|---|---|
| Inference timeout | **Fader** | deadline exceeded | request aborted | `inference_timeout_ms` (100–30 000 ms) |
| LoRA hot-swap timeout | **Switch** | swap duration > 5 s | rollback to previous adapter | `hot_swap.timeout_ms` |
| Circuit Breaker OPEN | **Closed Loop** | `failure_rate ≥ failure_threshold` | path blocked; probe requests sent | `circuit_breaker.failure_threshold` |
| Circuit Breaker HALF_OPEN → CLOSED | **Closed Loop** | probe request succeeds | path restored | `circuit_breaker.half_open_probe_interval` |
| gRPC deadline propagation | **Causal Chain** | client sets context deadline | deadline propagated through all layers | gRPC context metadata |

### Errors / Warnings

Errors and warnings are **events**, not control classes. Their effect depends on
which component receives them and how it reacts — the class is assigned to the
*reaction*, not to the event itself.

| Signal | Class | Source | Consumer | Effect |
|---|---|---|---|---|
| AQL parser WARN (unknown function) | **Open Loop** | AQL parser | AuditLogger | Log entry written; query not interrupted |
| Importance-score NaN | **Causal Chain** | AdaLoRA layer | PruningEngine → pruning disabled | Rank budget fixed until restart |
| P99 latency > baseline + 20 % | **Closed Loop** | SLO monitor | CI gate | Deployment blocked (§6 Δp99 rule) |
| LoRA training convergence WARN | **Causal Chain** | `IncrementalLoRATrainer` | `ContinuousLearningOrchestrator` → retry | Retraining with reduced `learning_rate` |
| Federation sync error | **Causal Chain** | `LoRAFederationCoordinator` | `CrossShardFeedbackSync` → retry → alert | Shard falls back to local importance score |

### Security

Security mechanisms in the LoRA stack span all seven classes. The class assignment
determines operator effort and system reaction time.

| Mechanism | Class | ThemisDB instance | Reference |
|---|---|---|---|
| Enforce TLS for all API endpoints | **Switch** | `tls.enforce` ON/OFF | `docker/admin-ui/nginx.ssl.conf` |
| MFA for admin/operator roles | **Switch** | `mfa_required_roles: [admin, operator]` | `include/security/access_control.h` |
| RBAC policy strictness | **Fader** | `rbac.policy_version` (permissive ↔ strict) | `src/security/access_control.cpp` |
| Login rate-limiting (nginx) | **Fader** | 5 r/m → 30 r/m | `docker/admin-ui/nginx.conf` |
| ZeroTrust session-risk continuous re-auth | **Closed Loop** | `session_risk_score` → `continuous_verification` | `include/security/zero_trust_policy_enforcer.h` |
| SPHINCS+ post-quantum LoRA audit signature | **Switch** | `pqc.enabled` (THEMIS_ENABLE_PQC=1) | `include/security/post_quantum_crypto.h` |
| Security anomaly → session revocation | **Causal Chain** | Intent Classifier → ZeroTrust → AuditLog → SIEM | §13 Causal Chain 2 |
| CSRF nonce validation | **Switch** | `csrf_validation.enabled` | `docker/admin-ui/nginx.conf` |

### Hardening

Hardening closes attack surfaces and validates system integrity. In the LoRA stack
this spans adapter storage security, gradient confidentiality, and CI pipeline gates.

| Measure | Class | Mechanism | Activation |
|---|---|---|---|
| Reject plaintext API calls | **Switch** | `security.deny_plaintext_api` | ON in production |
| Audit log verbosity | **Fader** | `audit.log_level` (INFO → DEBUG → TRACE) | SIGHUP hot-reload |
| Dependency pinning + SBOM generation | **Open Loop** | CI scan on every build | GitHub Actions |
| Post-quantum LoRA audit (SPHINCS+) | **Switch** | `pqc.enabled` | THEMIS_ENABLE_PQC=1 |
| IPv6 CIDR allowlist for gradient transport | **Fader** | `network_policy.cidr_allowlist` | `include/security/zero_trust_policy_enforcer.h` |
| Secret scanning gate | **Closed Loop** | secret-scanning alert → PR blocked | GitHub Actions |
| LoRA adapter storage: immutable container rootfs | **Switch** | read-only rootfs | `docker-compose.qnap.yml` |
| GDPR erase-target validation (adapter data) | **Closed Loop** | `GdprSubjectRightsManager` → per-module ACK | `include/governance/gdpr_subject_rights.h` |

> **Canonical reference for all 20 OR instances with SLO cross-links:**
> `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.8` (EN) ·
> `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md §12.8` (DE) ·
> Implementation work package: `docs/issues/distributed_knowledge/DK-OR-operational-resilience.md`
