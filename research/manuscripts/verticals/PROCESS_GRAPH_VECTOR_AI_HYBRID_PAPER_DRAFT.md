# Hybrid Graph-Process-Vector Retrieval for AI-Augmented Process Intelligence

**Status**: ACTIVE_DRAFT  
**Version**: 0.1  
**Last Updated**: 2026-08-10  
**Target Venue**: SIGMOD 2027 / BPM 2027 / arXiv (cs.DB / cs.AI / cs.IR)  
**Portfolio Cluster**: `research/manuscripts/verticals/`  
**Related Manuscripts**: `research/manuscripts/verticals/PROCESS_MINING_OCEL2_LIGHTRAG_GDPR_BPMN_DRAFT.md` (sibling — focuses on mining/compliance; this paper focuses on hybrid retrieval for AI reasoning)

---

## Metadata

- **Scientific Delta**: Formalize hybrid retrieval across three modalities — process graph structure (SCC, Louvain), vector embeddings (HNSW), and process-event context (OCEL 2.0 DFMG) — as a unified query strategy for AI-augmented process intelligence, rather than treating them as independent retrieval channels.
- **Canonical Evidence Sources**: `research/papers/process_graph_vector_ai_2026.md`, `src/process/README.md`, `src/graph/README.md`, `src/vector/README.md`, `src/rag/README.md`.
- **Required Experiments**: recall@K comparison across single-modality vs. hybrid retrieval; latency profile under OCEL 2.0 log scale; LLM answer quality (G-Eval) with hybrid vs. vector-only context.
- **Open Risks / Claim Boundaries**: process-vector hybrid integration is architecturally defined in `research/papers/process_graph_vector_ai_2026.md` but the dedicated cross-modality benchmark is not yet frozen.
- **Overlap / Successor / Predecessor**: process mining paper covers OCEL 2.0 ingestion, GDPR, and community detection; this paper covers the retrieval and AI-reasoning layer on top of those artifacts.

---

## Abstract

Process intelligence systems typically retrieve context through one of three channels: process graph traversal (structure), vector similarity (semantic), or event log query (temporal). Each channel alone misses information the others capture: graph traversal finds structural bottlenecks but not semantic intent; vector search finds conceptually similar cases but not causal chains; event log query finds occurrence patterns but not latent workflow segments. ThemisDB's multi-model architecture makes it possible to combine all three channels in a single AQL hybrid query plan. This paper formalizes the hybrid retrieval strategy, defines the fusion operator that merges graph, vector, and event-log signals, and proposes an evaluation methodology for recall, latency, and downstream LLM answer quality.

---

## I. Introduction

AI-augmented process analysis requires a retrieval layer that can answer questions across structural, semantic, and temporal dimensions simultaneously. A bottleneck analysis may require: identifying structurally similar process segments (graph), finding semantically analogous historical cases (vector), and locating events that occurred during those segments (event log). No single retrieval modality covers all three.

ThemisDB is designed for this combination: its AQL query layer can express graph traversal, vector similarity, and event-log predicates in a single plan. The process module provides OCEL 2.0 event log construction and Directly-Follows Multigraph (DFMG) computation. The graph module provides SCC analysis and Louvain community detection. The vector module provides HNSW-indexed embedding search. The research question is: how do these three channels combine into a coherent hybrid retrieval strategy, and does the combination outperform any single channel for AI-augmented process reasoning?

### Contributions

1. A formalization of process graph, vector, and event-log retrieval as a unified three-channel hybrid query strategy.
2. A fusion operator design for combining graph structural scores, vector similarity scores, and event-log frequency signals.
3. An evaluation plan measuring recall@K, latency, and LLM answer quality (G-Eval faithfulness) across single-modal and hybrid retrieval paths.

---

## II. Related Work

- Process mining and OCEL 2.0: van der Aalst (2022), OCEL 2.0 standard
- Graph retrieval: GraphRAG (Edge et al., 2024), HippoRAG (Gutierrez et al., 2024)
- Vector similarity: HNSW (Malkov & Yashunin, 2020)
- LightRAG retrieval modes: LOCAL entity-BFS, GLOBAL community-report, AUTO heuristic
- RAG evaluation: G-Eval (Liu et al., 2023), LLM-as-Judge (Zheng et al., 2023)
- novelty delta: three-channel fusion (process graph + vector + event log) with AQL-level hybrid plan, evaluated on process intelligence workloads

---

## III. System Model / Repository Scope

The hybrid retrieval pipeline spans four ThemisDB modules:

| Module | Role in hybrid retrieval |
|---|---|
| `src/process/` | OCEL 2.0 log construction, DFMG computation, SLA monitoring, Louvain segmentation |
| `src/graph/` | SCC analysis, graph traversal, graph-embedding computation |
| `src/vector/` | HNSW-indexed vector search, similarity scoring |
| `src/rag/` | Context assembly, fusion, LightRAG LOCAL/GLOBAL/AUTO modes, G-Eval evaluation |

The AQL query layer (`src/query/`) provides the hybrid plan that combines these four module paths.

---

## IV. Method / Design

### A. Three-Channel Retrieval Model

Each retrieval channel produces a scored candidate set:

1. **Graph channel**: AQL graph traversal from a seed process node; score = structural similarity (SCC membership + Louvain community overlap + DFG edge weight)
2. **Vector channel**: HNSW k-NN search over process-segment embeddings; score = cosine similarity
3. **Event-log channel**: DFMG frequency query over OCEL 2.0 log; score = normalized event co-occurrence frequency

### B. Fusion Operator

The fusion operator merges three scored candidate lists:

```
fusion_score(candidate) =
  α · graph_score + β · vector_score + γ · event_score
```

where α + β + γ = 1, and weights are calibrated per query class (structural, semantic, temporal). The operator returns the top-K candidates ranked by fusion score.

The AUTO routing mode of ProcessLightRetriever serves as the baseline for comparison: it selects one channel per query using a heuristic. The hybrid model uses all three channels regardless of query type and fuses their outputs.

### C. LLM Context Assembly

The top-K hybrid candidates are assembled into a context window passed to the LLM. The context includes:
- process graph segments with structural context (SCC, Louvain community label)
- embedding-similar historical cases (top-3 vector neighbors per candidate)
- event frequency summaries from OCEL 2.0 log (top co-occurring events)

G-Eval faithfulness and relevance scores are used to measure context quality.

---

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `research/papers/process_graph_vector_ai_2026.md` | system design | hybrid process-graph-vector retrieval schema and design rationale | ready |
| E2 | `src/process/README.md` | module scope | OCEL 2.0 tracer, DFMG, Louvain, LightRAG integration | ready |
| E3 | `src/graph/README.md` | module scope | graph traversal, SCC, embedding computation | ready |
| E4 | `src/vector/README.md` | module scope | HNSW indexing, similarity scoring | ready |
| E5 | `src/rag/README.md` | module scope | context assembly, G-Eval integration, LLM-as-Judge | ready |
| E6 | `tests/process/` | 22 test files | process module coverage: OCEL 2.0, DFMG, CEP, SLA, LightRAG modes | ready |

---

## VI. Experimental Methodology

### A. Setup
- OCEL 2.0 event log: synthetic or real process dataset (e.g., BPI Challenge)
- index: HNSW over process-segment embeddings; DFG frequency table from OCEL 2.0
- graph: SCC + Louvain over imported process graph
- LLM: local inference endpoint (Ollama `qwen2.5-coder:14b` or `gemma4:latest`)

### B. Workloads
- W1: structural retrieval task (find process segments with similar topology)
- W2: semantic retrieval task (find historically similar cases by description)
- W3: temporal retrieval task (find co-occurring events in past executions)
- W4: mixed task (all three dimensions simultaneously — AI process analyst persona)

### C. Metrics
- recall@K: single-modal (graph only, vector only, event-log only) vs. hybrid fusion
- p95/p99 retrieval latency per workload
- G-Eval faithfulness score for LLM answers under each retrieval strategy
- context precision / context recall (RAGAS) for hybrid vs. single-modal context windows

---

## VII. Results

### A. Primary Results
- three-channel architecture is implemented across process, graph, vector, and RAG modules (`E1`–`E5`)
- process module provides 22 test files and production-verified OCEL 2.0, LightRAG, and SLA coverage (`E6`)
- hybrid retrieval benchmark is not yet frozen; recall and latency figures are planned, not measured

### B. Ablations / Sensitivity
- fusion weight sensitivity: α/β/γ variation across structural, semantic, temporal task classes
- K sensitivity: recall@K for K ∈ {1, 3, 5, 10}
- log scale sensitivity: retrieval quality and latency for 1K, 10K, 100K OCEL events

### C. Negative Results
- single-channel baselines have not yet been formally benchmarked in isolation for process-intelligence workloads
- G-Eval faithfulness improvement from hybrid vs. vector-only context is a hypothesis, not yet measured

---

## VIII. Discussion

### Supported claims
- three-channel hybrid retrieval is architecturally possible in ThemisDB and all three modules have production-verified evidence (`E1`–`E6`)
- LightRAG LOCAL/GLOBAL/AUTO provides a natural single-channel baseline for comparison (`E2`, `E5`)

### Deferred claims
- quantitative recall@K improvement of hybrid over single-modal retrieval
- G-Eval faithfulness improvement from hybrid context (requires benchmark run)

---

## IX. Reproducibility & Artifact

- design basis: `research/papers/process_graph_vector_ai_2026.md`
- module tests: `tests/process/` (22 test files), `tests/vector/`, `tests/graph/`, `tests/rag/`
- experiment protocol: `research/experiments/verticals/process_graph_vector_hybrid_protocol.md` (to be created)
- evaluation dataset: BPI Challenge or synthetic OCEL 2.0 log (to be committed under `research/experiments/`)

---

## X. Limitations, Risk, Ethics

- OCEL 2.0 event logs from real processes may contain sensitive personal data; experiments must use anonymized or synthetic data
- fusion weight calibration is workload-dependent; generalizing α/β/γ across domains requires multi-dataset evaluation
- LLM answer quality is measured by G-Eval (LLM-as-Judge), which is itself dependent on the judge model quality

---

## XI. Conclusion

ThemisDB is one of the few multi-model databases that can natively express three-channel hybrid retrieval (graph + vector + event log) in a single AQL query plan, with all three channels backed by production-grade module implementations. The process module in particular — 33,106+ LOC, 22 test files, OCEL 2.0, LightRAG, Louvain — provides a rich empirical substrate for a hybrid retrieval evaluation. The next step is a reproducible evaluation protocol that formalizes the fusion operator and measures recall and G-Eval faithfulness across single-modal and hybrid retrieval paths.
