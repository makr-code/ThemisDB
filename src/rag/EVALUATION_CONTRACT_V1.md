# RAG Evaluation Contract v1

**Status:** Active  
**Created:** 2026-09-24  
**Last Updated:** 2026-09-24  
**Version:** 1.0.0  
**Scope:** Binding quality and performance metrics for `release_critical` RAG workflows  

---

## Purpose

This document defines the mandatory evaluation contract for RAG (Retrieval-Augmented Generation) workflows in ThemisDB. All `release_critical` RAG test runs must produce complete metric sets; regression beyond defined thresholds blocks release promotion.

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

All `release_critical` runs must validate against at least one dataset from each category:

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

### Pre-Test Setup

```bash
# 1. Load baseline metrics from config file
#    (baselines_v1.json is located at benchmarks/rag/data/baselines_v1.json)
export BASELINE_CONFIG="benchmarks/rag/data/baselines_v1.json"
echo "Using baseline config: ${BASELINE_CONFIG}"

# 2. Provision test datasets
#    Ensure the following dataset files are available under benchmarks/rag/eval_datasets_v1/:
#      wikipedia_rag_2k.json, code_rag_1k.json, multihop_qa_500.json
#    Download or generate them according to your environment's data provisioning process.
ls benchmarks/rag/eval_datasets_v1/

# 3. Warm up caches and indices
#    Start the ThemisDB server and run a small pre-warm query batch before timed tests.
#    Example: ctest -L warmup -R "^rag_warmup" --verbose
ctest -L warmup -R "^rag_warmup" --verbose || echo "No warmup target defined yet; proceed without warmup."
```

### Test Execution

```bash
# Run release_critical RAG gate
ctest --output-on-failure -L release_critical -R "^rag_" --verbose

# Collect metrics into structured output
benchmarks/rag/collect_metrics.py \
  --run_id ${GITHUB_RUN_ID} \
  --baseline baselines_v1.json \
  --output metrics_${GITHUB_RUN_ID}.json
```

### Post-Test Validation

```bash
# Validate metric completeness and thresholds
python scripts/validate_rag_metrics.py \
  --metrics metrics_${GITHUB_RUN_ID}.json \
  --baseline baselines_v1.json \
  --strict

# Generate acceptance report
python scripts/generate_rag_acceptance_report.py \
  --metrics metrics_${GITHUB_RUN_ID}.json \
  --template src/rag/RAG_EVAL_ACCEPTANCE_REPORT_TEMPLATE.md \
  --output RAG_EVAL_REPORT_${GITHUB_RUN_ID}.md
```

---

## Regression Detection & Response

### Automatic Detection

- All `release_critical` RAG runs automatically check metrics against baseline
- If any metric exceeds regression threshold:
  1. Run immediately flags with `release_critical_BLOCKED` label
  2. PR status blocked until regression is explained or metric issue is fixed
  3. Metrics are logged to artifacts for post-mortem analysis

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

All metric outputs must conform to this schema:

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

- This contract is the source-of-truth for `release_critical` RAG gates
- Module ROADMAPs (`src/rag/ROADMAP.md`, `src/llm/ROADMAP.md`) reference this contract
- CI workflow (`.github/workflows/gate-pr-rag-eval.yml`) must enforce all metrics

### Versioning

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-09-24 | Initial contract: 6 core metrics, 4 datasets, Q4 2026 gate |
| 2.0.0 | 2027-02-01 | (Planned) Add domain-specific SLOs, expand adversarial sets |

---

## References

- `audit/RAG_READINESS_AUDIT_2026-09-23.md` § 5.1 (RAG Eval Contract v1)
- `src/rag/ROADMAP.md` (module-level acceptance criteria)
- `src/llm/ROADMAP.md` (Wiki Phase C acceptance gates)
- `benchmarks/rag/` (evaluation harness and datasets)
- `.github/workflows/gate-pr-rag-eval.yml` (CI gate implementation)
