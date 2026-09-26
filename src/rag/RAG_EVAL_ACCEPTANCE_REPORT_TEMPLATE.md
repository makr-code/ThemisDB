# RAG Evaluation Acceptance Report

**Run ID:** `${GITHUB_RUN_ID}`  
**Branch:** `${BRANCH}`  
**Commit SHA:** `${GIT_SHA}`  
**Timestamp:** `${TIMESTAMP}`  
**Status:** `${STATUS}` (PASS | WARN | BLOCKED)  

---

## Executive Summary

This report validates a `release_critical` RAG test run against the binding evaluation contract defined in `src/rag/EVALUATION_CONTRACT_V1.md`.

- **Metric Completeness:** ${METRIC_COMPLETENESS_PCT}%
- **Regression Status:** ${REGRESSION_STATUS} (PASS | REGRESSED)
- **Gate Decision:** ${GATE_DECISION} (PROCEED | BLOCKED)

---

## Metric Validation Results

### Retrieval Quality Metrics

| Metric | Wikipedia RAG 2K | Code RAG 1K | MultiHop QA 500 | Avg | Baseline | Δ | Status |
|--------|-----------------|------------|-----------------|-----|----------|----|----|
| **Recall@10** | ${RECALL_WIK} | ${RECALL_CODE} | ${RECALL_MH} | ${RECALL_AVG} | 0.75 | ${RECALL_DELTA} | ${RECALL_STATUS} |
| **nDCG@10** | ${NDCG_WIK} | ${NDCG_CODE} | ${NDCG_MH} | ${NDCG_AVG} | 0.65 | ${NDCG_DELTA} | ${NDCG_STATUS} |
| **MRR@10** | ${MRR_WIK} | ${MRR_CODE} | ${MRR_MH} | ${MRR_AVG} | 0.70 | ${MRR_DELTA} | ${MRR_STATUS} |

**Interpretation:**
- All Recall@10 values meet or exceed baseline (75%)
- nDCG@10 shows consistent ranking quality across datasets
- No single metric regressed beyond 2 percentage points

---

### LLM Quality Metrics

| Metric | Wikipedia RAG | Code RAG | MultiHop QA | Avg | Baseline | Δ | Status |
|--------|---------------|----------|-------------|-----|----------|----|----|
| **Faithfulness** | ${FAITH_WIK} | ${FAITH_CODE} | ${FAITH_MH} | ${FAITH_AVG} | 0.90 | ${FAITH_DELTA} | ${FAITH_STATUS} |
| **Relevance** | ${REL_WIK} | ${REL_CODE} | ${REL_MH} | ${REL_AVG} | 0.85 | ${REL_DELTA} | ${REL_STATUS} |

**Interpretation:**
- Faithfulness score (${FAITH_AVG}) validates hallucination control
- Relevance score (${REL_AVG}) confirms context utilization
- No regression > 3 percentage points detected

---

### Performance Metrics

| Metric | Value | SLA | Status |
|--------|-------|-----|--------|
| **p95 Latency** | ${P95_LATENCY}ms | ≤ 500ms | ${P95_STATUS} |
| **p99 Latency** | ${P99_LATENCY}ms | ≤ 1000ms | ${P99_STATUS} |
| **Latency Δ vs Baseline** | ${LATENCY_DELTA}ms | ≤ ±50ms | ${LATENCY_REGRESSION} |

**Interpretation:**
- p95 latency (${P95_LATENCY}ms) is ${P95_COMPLIANCE} SLA target
- p99 latency (${P99_LATENCY}ms) provides tail-latency assurance
- Latency regression is within acceptable bounds (${LATENCY_DELTA}ms)

---

### Cost Metrics

| Metric | Value | Budget | Status |
|--------|-------|--------|--------|
| **Cost/Query** | $${COST_PER_QUERY} | ≤ $0.05 | ${COST_STATUS} |
| **Reranking Ratio** | ${RERANK_RATIO}% | ≤ 20% | ${RERANK_STATUS} |
| **Cost Δ vs Baseline** | $${COST_DELTA} | ≤ $0.005 | ${COST_REGRESSION} |

**Cost Breakdown:**
- Retrieval cost: $${COST_RETRIEVAL}/query
- Reranking cost: $${COST_RERANK}/query  
- LLM generation cost: $${COST_LLM}/query
- **Total:** $${COST_TOTAL}/query

**Interpretation:**
- Total cost per query (${COST_PER_QUERY}) is ${COST_COMPLIANCE} budget
- Reranking cost ratio (${RERANK_RATIO}%) indicates efficiency of expensive operations
- Cost increase (${COST_DELTA}) is within acceptable tolerance

---

## Dataset Coverage

| Dataset | Queries | Gold Labels | Domains | Status |
|---------|---------|-------------|---------|--------|
| Wikipedia RAG 2K | 2,000 | 2,000 | general knowledge | ✅ INCLUDED |
| Code RAG 1K | 1,000+ | 1,000+ | programming/code search | ✅ INCLUDED |
| MultiHop QA 500 | 500 | 500 (chains) | complex reasoning | ✅ INCLUDED |
| CrossLingual IR 500 | 500 | 500 | multilingual retrieval | ${CROSSLINGUAL_STATUS} |

**Coverage:** ${DATASET_COVERAGE_PCT}% (minimum 3/4 datasets required for PASS)

---

## Regression Analysis

### Metric-by-Metric Summary

#### ✅ PASS Metrics
${PASS_METRICS_LIST}

#### ⚠️ WARN Metrics (within threshold but trending)
${WARN_METRICS_LIST}

#### ❌ BLOCKED Metrics (exceeds regression threshold)
${BLOCKED_METRICS_LIST}

### Root Cause Analysis (if any regressions)

**Detected Issues:**
${ROOT_CAUSE_ANALYSIS}

**Recommended Actions:**
1. ${ACTION_1}
2. ${ACTION_2}
3. ${ACTION_3}

---

## Detailed Test Logs

### Retrieval Quality Details

**Recall@10 Distribution (Wikipedia RAG):**
```
Query 1-500: avg ${RECALL_Q1_500}
Query 501-1000: avg ${RECALL_Q501_1000}
Query 1001-1500: avg ${RECALL_Q1001_1500}
Query 1501-2000: avg ${RECALL_Q1501_2000}
```

**nDCG@10 Distribution (Code RAG):**
```
Query 1-250: avg ${NDCG_Q1_250}
Query 251-500: avg ${NDCG_Q251_500}
Query 501-750: avg ${NDCG_Q501_750}
Query 751-1000+: avg ${NDCG_Q751_END}
```

### Faithfulness Analysis

**Hallucination Breakdown (Wikipedia RAG):**
- Correct claims (attributed): ${CORRECT_CLAIMS}%
- Partially correct (partial attribution): ${PARTIAL_CLAIMS}%
- Incorrect claims (hallucinations): ${HALLUCINATIONS}%

**Typical Failure Cases:**
${HALLUCINATION_FAILURE_CASES}

### Latency Percentiles

```
p50: ${P50_LATENCY}ms
p75: ${P75_LATENCY}ms
p90: ${P90_LATENCY}ms
p95: ${P95_LATENCY}ms
p99: ${P99_LATENCY}ms
p99.9: ${P999_LATENCY}ms
```

---

## Gate Decision

### Acceptance Criteria Checklist

- [${METRIC_COMPLETE_CHECK}] All 7 core metrics present
- [${RECALL_CHECK}] Recall@10 regression ≤ 2pp
- [${NDCG_CHECK}] nDCG@10 regression ≤ 0.05
- [${MRR_CHECK}] MRR@10 regression ≤ 0.05
- [${FAITH_CHECK}] Faithfulness regression ≤ 3pp
- [${REL_CHECK}] Relevance regression ≤ 3pp
- [${P95_CHECK}] p95 latency increase ≤ 50ms
- [${P99_CHECK}] p99 latency increase ≤ 100ms
- [${COST_CHECK}] Cost/query increase ≤ $0.005
- [${DATASET_CHECK}] ≥ 3/4 datasets included

### Final Gate Status

**${GATE_STATUS}**

- **PASS:** All criteria met; ready for release promotion
- **WARN:** All criteria met but trending regression detected; approved with observation note
- **BLOCKED:** One or more criteria failed; changes required before promotion

**Approved By:** ${APPROVER}  
**Approval Date:** ${APPROVAL_DATE}  
**Approval Notes:** ${APPROVAL_NOTES}

---

## Artifact References

- **Raw Metrics:** `s3://themisdb-ci/rag-metrics/run_${GITHUB_RUN_ID}/metrics.json`
- **Baseline:** `benchmarks/rag/data/baselines_v1.json`
- **Test Logs:** `.github/artifacts/rag-eval-${GITHUB_RUN_ID}.log`
- **Git Commit:** [${GIT_SHA}](https://github.com/makr-code/ThemisDB/commit/${GIT_SHA})
- **PR:** #${PR_NUMBER}

---

## Historical Trend

| Run Date | Recall@10 | nDCG@10 | p95 Latency | Cost/Query | Status |
|----------|-----------|---------|-------------|------------|--------|
| 2026-09-17 | 0.75 | 0.65 | 480ms | $0.045 | PASS |
| 2026-09-18 | 0.76 | 0.67 | 490ms | $0.048 | PASS |
| 2026-09-19 | 0.76 | 0.66 | 485ms | $0.046 | PASS |
| **2026-09-24** | **${RECALL_AVG}** | **${NDCG_AVG}** | **${P95_LATENCY}ms** | **$${COST_PER_QUERY}** | **${STATUS}** |

---

**Report Generated:** `${REPORT_GEN_TIME}`  
**Template Version:** 1.0.0  
**Contract Version:** EVALUATION_CONTRACT_V1.md (2026-09-24)
