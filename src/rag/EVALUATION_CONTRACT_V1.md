---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RAG Evaluation Contract v1

**Status:** Active  
**Created:** 2026-09-24  
**Last Updated:** 2026-09-24  
**Version:** 1.0.0  
**Scope:** Binding quality and performance metrics for `release_critical` RAG workflows  

> **Implementation status (checked 2026-11-01):** This file specifies intended
> acceptance criteria; its targets are not measured baselines. Four small,
> project-authored synthetic smoke fixtures now exist under
> `benchmarks/rag/eval_datasets_v1/`. They do not contain the 2,000/1,000/500
> query corpora named below and are not representative retrieval-quality data.
> `gate-pr-rag-eval.yml` validates documentation,
> the baseline JSON, and the report template; its metric comparison runs only
> when `eval_results.json` is supplied and checks Recall@10. It does not collect
> or enforce the full metric set by itself. The workflow's header says “6 core
> metrics” while its check enumerates seven; this count discrepancy remains
> unresolved.

---

## Purpose

This document specifies the intended evaluation contract for RAG
(Retrieval-Augmented Generation) workflows in ThemisDB. Its required metric
coverage and regression thresholds are acceptance criteria, not evidence that
current `release_critical` runs collect or enforce them.

---

## Core Metrics

### 1. Retrieval Quality Metrics

| Metric | Definition | Unit | Target Baseline | Regression Threshold | Notes |
|--------|-----------|------|-----------------|-------------------|-------|
| **Recall@10** | Fraction of relevant documents present in top-10 results | % | ≥ 75% | No regression > 2pp | Primary retrieval quality gate |
| **nDCG@10** | Normalized Discounted Cumulative Gain @ rank 10 | score | ≥ 0.65 | No regression > 0.05 | Measures ranking quality; penalizes misordering |
| **MRR@10** | Mean Reciprocal Rank (position of first relevant result) | score | ≥ 0.70 | No regression > 0.05 | Measures answer-finding speed |

### 2. LLM-Specific Metrics

| Metric | Definition | Unit | Target Baseline | Regression Threshold | Notes |
|--------|-----------|------|-----------------|-------------------|-------|
| **Faithfulness** | Fraction of generated claims attributable to context | % | ≥ 90% | No regression > 3pp | Guards against hallucinations |
| **Relevance** | Fraction of context paragraphs contributing to final answer | % | ≥ 85% | No regression > 3pp | Measures context utilization |

### 3. Performance Metrics

| Metric | Definition | Unit | Target SLA | Regression Threshold | Notes |
|--------|-----------|------|-----------|-------------------|-------|
| **p95 Latency** | 95th percentile end-to-end retrieval + ranking latency | ms | ≤ 500ms | No increase > 50ms | Per query under standard load |
| **p99 Latency** | 99th percentile end-to-end latency | ms | ≤ 1000ms | No increase > 100ms | Tail latency bound |

### 4. Cost Metrics

| Metric | Definition | Unit | Target SLA | Regression Threshold | Notes |
|--------|-----------|------|-----------|-------------------|-------|
| **Cost/Query** | Average API cost per retrieval + reranking + LLM call | $ | ≤ $0.05 | No increase > $0.005 | Aggregate of all sub-service costs |
| **Reranking Cost Ratio** | Fraction of queries receiving reranking | % | ≤ 20% | No increase > 5pp | Budget control for expensive operations |

---

## Test Datasets

### Domain Categories

The specification requires `release_critical` runs to validate against at
least one dataset from each category. The checked-in files are smoke fixtures
only; the full-size external datasets described below are not included, so the
dataset-level release acceptance requirement is not yet reproducible here.

The four JSON files use `schema_version`, `dataset_id`, `data_kind`,
`provenance`, `limitations`, and `records`. Each record contains a stable `id`,
`query`, candidate `documents` (`id` and `text`), and
`relevant_document_ids`. Run
`python3 benchmarks/rag/validate_eval_datasets.py` to validate file presence,
schema, unique identifiers, and relevance references. The validator does not
run retrieval or calculate evaluation metrics.

#### A. Wikipedia RAG (General Knowledge Retrieval)
- **Dataset ID:** `wikipedia_rag_2k_v1`
- **Scope:** 2,000 real-world queries over Wikipedia corpus
- **Location:** `benchmarks/rag/eval_datasets_v1/wikipedia_rag_2k.json`
- **Relevant Metrics:** Recall@10, nDCG@10, MRR@10, Faithfulness
- **Gold Standard:** Human-curated relevance judgments (2-level: relevant/not relevant)

#### B. Code Search RAG (Specialized Domain Retrieval)
- **Dataset ID:** `code_rag_1k_v1`
- **Scope:** 1,000+ code search queries with function/class relevance labels
- **Location:** `benchmarks/rag/eval_datasets_v1/code_rag_1k.json`
- **Relevant Metrics:** Recall@10, nDCG@10, MRR@10
- **Gold Standard:** Cross-validated with real code-search logs

#### C. Multi-Hop QA (Complex Reasoning Retrieval)
- **Dataset ID:** `multihop_qa_500_v1`
- **Scope:** 500 queries requiring multi-step reasoning across documents
- **Location:** `benchmarks/rag/eval_datasets_v1/multihop_qa_500.json`
- **Relevant Metrics:** All quality + faithfulness metrics
- **Gold Standard:** Hand-annotated answer chains with required document sets

#### D. Cross-Lingual IR (Multilingual Robustness)
- **Dataset ID:** `crosslingual_ir_500_v1`
- **Scope:** 500 queries in German/French/Spanish with English corpus
- **Location:** `benchmarks/rag/eval_datasets_v1/crosslingual_500.json`
- **Relevant Metrics:** Recall@10, nDCG@10
- **Gold Standard:** Professional translation + manual verification

---

## Acceptance Criteria

### Mandatory Pass Conditions

1. **Complete Metric Coverage:** All seven core metrics (Recall@10, nDCG@10, MRR@10, Faithfulness, Relevance, p95 latency, cost/query) must be present in test output
2. **No Single-Metric Regressions:** Any individual metric regression > threshold blocks release
3. **Cross-Dataset Consistency:** Metric deltas across datasets must not exceed 5pp (percentage points) without investigation
4. **Latency Bounds:** Both p95 and p99 must remain within SLA
5. **Cost Transparency:** Full cost breakdown (retrieval + reranking + LLM) must be auditable

### Optional Enhancements

- [ ] Bias audit: gender/demographic fairness across subgroups (target: balanced representation)
- [ ] Adversarial robustness: performance on paraphrased/adversarial queries (target: <5pp drop)
- [ ] Cold-start evaluation: performance with new embedding models (target: canary validation before cutover)

---

## Test Execution Workflow

The commands and scripts previously described for dataset provisioning,
`collect_metrics.py`, `validate_rag_metrics.py`, and
`generate_rag_acceptance_report.py` are not present in this checkout. No
repository command currently executes this full dataset-level contract. The
focused RAG unit tests are registered from `tests/rag/CMakeLists.txt`; they
validate component behavior, not these dataset-level thresholds. See
`src/rag/RESEARCH.md` and `benchmarks/rag/CMakeLists.txt` for current benchmark
source and registration limitations.

---

## Regression Detection & Response

### Current workflow behavior

- The workflow validates this contract, the baseline registry, and the report
  template.
- If `benchmarks/rag/data/eval_results.json` is present, it checks Recall@10
  against the configured threshold; it does not collect results or compare the
  other specified metrics.
- The separate release-critical test job runs only when the PR has a
  `rag-tests` or `release_critical` label.
- The workflow can apply labels and post a report after its validation job
  succeeds. The report text is not a measured evaluation result.

### Investigation Checklist

When regression is detected, investigate:

- [ ] Embedding model or chunking changed? → re-baseline required (Phase 3 Versioning)
- [ ] Index is stale/corrupted? → reindex and re-run
- [ ] Infrastructure change (latency anomaly)? → infrastructure team review
- [ ] Test data cache expired? → refresh test datasets
- [ ] New dataset bias in gold labels? → manual review of edge cases
- [ ] Legitimate algorithm improvement (e.g., better ranking)? → accept and re-baseline

### Re-baselining Procedure

Automatic re-baselining is allowed only after:

1. Root cause documented in PR comment
2. Manual human approval from RAG module owner
3. New baseline metrics stored with change tracking in `benchmarks/rag/baseline_history/`
4. Changelog entry noting reason for re-baseline

---

## Metric Artifact Schema

The following illustrative JSON shows the proposed schema; its values are
placeholders, not measured results. Actual metric collection and full-schema
validation are not implemented by the current evaluation workflow.

```json
{
  "run_id": "github_run_id",
  "timestamp": "2026-09-24T05:43:22+00:00",
  "phase": "release_critical",
  "metrics": {
    "retrieval_quality": {
      "recall_at_10": {
        "wikipedia_rag_2k": 0.76,
        "code_rag_1k": 0.78,
        "multihop_qa_500": 0.72,
        "average": 0.75
      },
      "ndcg_at_10": { ... },
      "mrr_at_10": { ... }
    },
    "llm_quality": {
      "faithfulness": {
        "wikipedia_rag_2k": 0.92,
        "code_rag_1k": 0.89,
        "multihop_qa_500": 0.91,
        "average": 0.91
      },
      "relevance": { ... }
    },
    "performance": {
      "p95_latency_ms": 485,
      "p99_latency_ms": 950,
      "latency_by_dataset": { ... }
    },
    "cost": {
      "cost_per_query": 0.045,
      "reranking_cost_ratio": 0.18,
      "cost_breakdown": {
        "retrieval": 0.010,
        "reranking": 0.025,
        "llm_generation": 0.010
      }
    }
  },
  "baseline_comparison": {
    "recall_at_10_delta": -0.01,
    "ndcg_at_10_delta": 0.02,
    "p95_latency_delta_ms": 20,
    "cost_delta": 0.001,
    "status": "PASS"
  },
  "test_datasets": [
    "wikipedia_rag_2k",
    "code_rag_1k",
    "multihop_qa_500"
  ],
  "git_sha": "abc123...",
  "branch": "develop"
}
```

---

## Governance & Updates

### Metric Stability

- Core metrics freeze: v1.0 is locked for Q4 2026
- Baseline re-calibration: permitted only on major algorithm changes (Phase 5+)
- New datasets: can be added mid-quarter if approved by RAG module owner

### Documentation Sync

- This file specifies intended `release_critical` RAG gates; actual workflow enforcement is limited as described above.
- Module ROADMAPs (`src/rag/ROADMAP.md`, `src/llm/ROADMAP.md`) reference this contract
- CI workflow (`.github/workflows/gate-pr-rag-eval.yml`) currently validates contract artifacts and conditionally compares Recall@10 when a results file is supplied; full-metric enforcement remains unverified.

### Versioning

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-09-24 | Initial specification: 7 listed mandatory metrics, 4 named datasets, intended Q4 2026 gate |
| 2.0.0 | 2027-02-01 | (Planned) Add domain-specific SLOs, expand adversarial sets |

---

## References

- `audit/RAG_READINESS_AUDIT_2026-09-23.md` § 5.1 (RAG Eval Contract v1)
- `src/rag/ROADMAP.md` (module-level acceptance criteria)
- `src/llm/ROADMAP.md` (Wiki Phase C acceptance gates)
- `benchmarks/rag/` (evaluation harness and datasets)
- `.github/workflows/gate-pr-rag-eval.yml` (CI gate implementation)
