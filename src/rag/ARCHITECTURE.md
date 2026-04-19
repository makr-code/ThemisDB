# RAG Module — Architecture Guide

> **Status:** 2026-04-19 – Architekturtext gegen realen Sourcecode verifizieren; Abweichungen mit `<!-- TODO -->` markiert.

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/rag/`

---

## 1. Overview

The RAG (Retrieval-Augmented Generation) module implements ThemisDB's full RAG pipeline:
hybrid retrieval (vector + BM25 with Reciprocal Rank Fusion), streaming MMR-based context
filling, multi-dimensional LLM-judge evaluation (faithfulness, relevance, completeness,
coherence, bias), knowledge gap detection, hallucination monitoring, and continuous learning.

The module also provides an agentic RAG variant (`agentic_rag.cpp`) that uses iterative
retrieval and self-evaluation loops for complex multi-hop queries.

---

## 2. Design Principles

- **Quality by Default** – every RAG answer is evaluated by multiple dimensions;
  low-quality answers are flagged before delivery.
- **Hybrid Retrieval** – vector similarity and BM25 lexical search are fused using
  Reciprocal Rank Fusion (RRF) to combine semantic and lexical relevance.
- **Streaming Context** – `streaming_retriever.cpp` fills the context window incrementally,
  token-budget-aware, with MMR deduplication to avoid redundant context.
- **Multi-Judge Evaluation** – faithfulness, relevance, completeness, coherence, and bias
  are evaluated independently and combined by a judge ensemble.
- **Continuous Learning** – the continuous learning orchestrator feeds evaluation results
  back into the system to improve retrieval and generation over time.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `rag_pipeline.cpp` (implied) | Main RAG orchestrator: retrieve → augment → generate |
<!-- TODO: verify symbol exists in source -->
| `hybrid_retriever.cpp` | Vector + BM25 hybrid retrieval with RRF fusion |
| `streaming_retriever.cpp` | Token-budget-aware streaming context filling with MMR |
| `document_splitter.cpp` | Split documents into retrieval-friendly chunks |
| `document_summarizer.cpp` | Summarize long documents for context window |
| `agentic_rag.cpp` | Iterative multi-hop agentic RAG |
| `rag_judge.cpp` | Multi-dimensional evaluation orchestrator |
| `faithfulness_evaluator.cpp` | Fact-checking: are claims grounded in sources? |
| `relevance_evaluator.cpp` | Query-answer alignment score |
| `completeness_evaluator.cpp` | Query aspect coverage |
| `coherence_evaluator.cpp` | Structure and readability score |
| `bias_detector.cpp` | Ethical bias checking |
| `claim_extractor.cpp` | Extract atomic claims for fact-checking |
| `cot_evaluator.cpp` | Chain-of-thought quality evaluation |
| `geval_evaluator.cpp` | G-Eval framework (Liu et al., 2023) |
| `judge_ensemble.cpp` | Multi-judge voting strategies (majority, weighted) |
| `pairwise_comparator.cpp` | Head-to-head answer comparison |
| `rubric_evaluator.cpp` | Custom rubric-based evaluation |
| `llm_judge_integration.cpp` | LLM-as-judge orchestration |
| `llm_meta_analyzer.cpp` | Meta-analysis of judge performance |
| `knowledge_gap_detector.cpp` | Three-level gap detection (concept, temporal, domain) |
| `llm_integration.cpp` | LLM bridge for generation |
| `citation_highlighter.cpp` | Source citation extraction and highlighting |
| `hallucination_dashboard.cpp` | Hallucination monitoring and alerting |
| `ab_testing_framework.cpp` | A/B testing for RAG pipeline variants |
| `bayesian_optimizer.cpp` | Bayesian hyperparameter optimization for RAG config |
| `continuous_learning_orchestrator.cpp` | Feedback loop for continuous improvement |
| `continuous_learning_client.cpp` | Client for continuous learning service |
| `response_parser.cpp` | Parse LLM evaluation responses |
| `prompt_templates.cpp` | RAG-specific prompt templates |
| `judge_config.cpp` | Judge configuration validation |
| `http_metrics_client.cpp` | HTTP metrics export to external monitoring |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  RAG Query (user question)                       │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    HybridRetriever                               │
│  VectorSearch(question_embedding) + BM25(keywords)              │
│  → Reciprocal Rank Fusion → ranked candidates                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │ candidates
┌──────────────────────────▼──────────────────────────────────────┐
│                    StreamingRetriever                            │
│  MMR deduplication → token-budget-aware context fill            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ context
┌──────────────────────────▼──────────────────────────────────────┐
│              LLM Integration (src/llm/)                         │
│  prompt = question + context → LLM → answer                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │ answer
┌──────────────────────────▼──────────────────────────────────────┐
│                     RAG Judge                                    │
│  Faithfulness │ Relevance │ Completeness │ Coherence │ Bias      │
│  → JudgeEnsemble → combined score                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ evaluation result
┌──────────────────────────▼──────────────────────────────────────┐
│    ContinuousLearningOrchestrator: feed metrics → improve       │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Standard RAG Query

```
RAG(question: "What is ThemisDB?")
    │
    ├─ embed question → query_vector
    ├─ hybrid_retriever: vector_search(k=20) + bm25_search(k=20) → RRF → top 20
    ├─ streaming_retriever: MMR dedup + token budget → context (≤2000 tokens)
    ├─ llm.generate(prompt = system + context + question) → answer
    ├─ rag_judge.evaluate(question, answer, context):
    │       faithfulness: 0.92, relevance: 0.88, completeness: 0.85
    │       coherence: 0.91, bias: none
    │       overall: 0.89 (PASS)
    └─ return {answer, citations, score: 0.89}
```

### 4.2 Agentic RAG (Multi-Hop)

```
Complex query: "Compare X and Y in the context of Z"
    │
    ├─ AgenticRAG: decompose → sub-questions [Q1, Q2, Q3]
    ├─ for each sub-question: standard RAG pipeline
    ├─ KnowledgeGapDetector: any gaps in sub-answers?
    │       → gap found → retrieve additional context → re-generate
    └─ synthesize final answer from sub-answers
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/llm/` | LLM generation and embedding |
| **Uses** | `src/index/` | Vector search and inverted index for BM25 |
| **Uses** | `src/search/` | BM25 full-text search |
| **Uses** | `src/prompt_engineering/` | RAG-specific prompt templates |
| **Called by** | `src/server/` | RAG API endpoints |
| **Called by** | `src/aql/` | LLM RAG command |

---

## 6. Threading & Concurrency Model

- RAG pipeline runs per-request; multiple concurrent requests are handled by the
  server's thread pool.
- `HybridRetriever` parallelizes vector and BM25 search using `std::async`.
- Judge evaluators run in parallel for independent dimensions.
- `ContinuousLearningOrchestrator` runs on a background thread.

---

## 7. Performance Architecture

| Mode | Latency | Use Case |
|---|---|---|
| Fast | ~100 ms | High-throughput production |
| Balanced | ~500 ms | Standard RAG pipeline |
| Thorough | ~2 s | Research, benchmarking |

| Technique | Detail |
|---|---|
| RRF fusion | Combines vector + lexical without requiring score normalization |
| MMR deduplication | Reduces redundant context; improves answer diversity |
| Token budget | Avoids exceeding LLM context window |
| Judge parallelism | 5 judge dimensions evaluated concurrently |

---

## 8. Security Considerations

- LLM outputs are evaluated for bias and ethical compliance before delivery.
- Citation highlighter links claims to source documents for auditability.
- Hallucination dashboard monitors and alerts on elevated hallucination rates.
- RAG queries are scoped to the authenticated tenant's accessible documents.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `rag.retrieval.vector_k` | 20 | Vector candidates per query |
| `rag.retrieval.bm25_k` | 20 | BM25 candidates per query |
| `rag.context.max_tokens` | 2000 | Context window token budget |
| `rag.judge.enabled` | true | Enable multi-dimensional evaluation |
| `rag.judge.min_score` | 0.7 | Minimum acceptable quality score |
| `rag.mode` | "balanced" | fast / balanced / thorough |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| No retrieval results | Return "I don't have information on this topic" |
| LLM timeout | Return retrieval results without generation |
| Low judge score | Flag answer with quality warning; optionally retry |
| Knowledge gap detected | Retrieve additional context; re-generate |

---

## 11. Known Limitations & Future Work

- Agentic RAG multi-hop is experimental; reasoning loop depth is bounded.
- BM25 requires an inverted index; currently depends on the search module's index.
- Continuous learning feedback loop is in progress.
- Multi-modal RAG (images + text) is planned.

---

## 12. References

- `src/rag/README.md` — module overview
- `src/rag/QUALITY_CONTROL_README.md` — quality control framework
- `docs/rag/` — RAG documentation
- `ARCHITECTURE.md` (root) — full system architecture
