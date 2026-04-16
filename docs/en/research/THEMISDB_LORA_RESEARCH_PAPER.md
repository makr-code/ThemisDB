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
10. [Related Work](#10-related-work)
11. [Conclusion](#11-conclusion)
12. [References](#12-references)

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

| Metric | Definition | Target |
|---|---|---|
| **Advisor Accuracy** | % of recommendations improving p99 by > 10 % in sandbox replay | ≥ 75 % |
| **AQL Validity** | % of generated AQL queries that parse + execute without error | ≥ 95 % |
| **Hallucination Rate** | % of answers citing non-existent index types or parameters | < 3 % |
| **HNSW Parameter RMSE** | RMSE of recommended `efSearch` vs. rule-based tuner baseline | < 20 |

### 7.2 Online A/B Metrics

```
Control group:   HnswParameterTuner (current rule-based system)
Treatment group: ThemisDB-LoRA advisor (10 % traffic via deployVersionEx)

Primary metric:  Mean p99 query latency
Secondary:       DBA acceptance rate of recommendations (thumbs up/down)
Guard rail:      Zero regression in cluster stability (no failover events)
```

### 7.3 Benchmark Dataset

Minimum viable benchmark: **1 000 `(query, plan, Δlatency)` triples** from a
representative mixed workload (300 OLTP, 400 OLAP, 200 vector-search, 100
graph-traversal), generated via:

```bash
themisdb-cli optimizer-log export \
  --since 30d \
  --format training-pairs \
  --min-delta-p99 0.05 \
  --output db_optimizer_training_v1.jsonl
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

---

## 10. Related Work

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
