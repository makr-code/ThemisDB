# Research-Backed Eval Harness & Drift Detection Specification
**Phase 9 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q2 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Evaluation harnesses enable continuous quality assessment across diverse domains and detect behavior drift before impacting users. This phase implements multi-domain evaluation, adversarial testing, and drift detection to maintain model trustworthiness and operational safety.

**Key Objectives:**
- Evaluate RAG quality across 5+ domains (factual, temporal, multi-hop, comparison, reasoning)
- Detect quality drift within 1-2 evaluation cycles (daily/weekly)
- Automated regression testing with statistical rigor
- Research-backed metrics (NDCG, MRR, Precision@k, human correlation)

---

## 2. Architecture

### 2.1 Evaluation Pipeline

```
Evaluation Dataset
    ├→ Multi-domain: 5 categories × 200 queries = 1000 total
    ├→ Adversarial: edge cases, OOD, adversarial examples
    └→ Historical: baseline comparisons
    ↓
Eval Runner (async, daily/weekly)
    ├→ Query RAG with each test query
    ├→ Collect: top-k results, latency, cost
    └→ Compute: nDCG, MRR, Precision@k, BLEU
    ↓
Drift Detector
    ├→ Compare: current vs baseline metrics
    ├→ Compute: drift magnitude and confidence
    └→ Trigger: alert if drift > threshold
    ↓
Root Cause Analysis (RCA)
    ├→ Identify: which component caused drift
    ├→ Examples: retrieval quality, ranking, versioning
    └→ Recommendation: rollback or investigate
    ↓
Dashboard & Reporting
    ├→ Quality trend (hourly/daily)
    ├→ Drift detection results
    ├→ Domain-specific breakdowns
    └→ RCA recommendations
```

### 2.2 Evaluation Domains

| Domain | Example Query | Metric Priority | Baseline Quality |
|--------|---------------|-----------------|------------------|
| Factual | "Who invented the lightbulb?" | Precision | 0.85 |
| Temporal | "COVID cases in Sept 2026?" | Freshness | 0.72 |
| Multi-hop | "Authors of books banned in Texas?" | nDCG | 0.68 |
| Comparison | "Python vs Go for backends?" | MRR | 0.75 |
| Reasoning | "Why is photosynthesis important?" | BLEU | 0.70 |

---

## 3. Component Specifications

### 3.1 EvaluationDataset

**Purpose:** Manage evaluation queries and ground truth labels.

**API:**
```cpp
class EvaluationDataset {
  enum class Domain { Factual = 0, Temporal = 1, MultiHop = 2, Comparison = 3, Reasoning = 4 };
  
  struct EvalQuery {
    std::string query_id;        // "factual-001"
    std::string query_text;
    Domain domain;
    std::vector<std::string> relevant_doc_ids;  // ground truth
    std::vector<float> relevance_scores;  // per-doc graded relevance [0, 3]
    std::string created_at;
  };
  
  // Load dataset for domain
  std::vector<EvalQuery> LoadDomain(Domain domain);
  
  // Load all evaluation queries
  std::vector<EvalQuery> LoadAll();
  
  // Add new evaluation query
  bool AddQuery(const EvalQuery& query);
  
  // Get statistics
  struct DatasetStats {
    uint32_t total_queries;
    std::map<Domain, uint32_t> queries_per_domain;
    float avg_relevant_docs_per_query;
  };
  DatasetStats GetStats();
};
```

**Dataset Format (JSON):**
```json
{
  "queries": [
    {
      "query_id": "factual-001",
      "query_text": "Who invented the lightbulb?",
      "domain": "factual",
      "relevant_docs": [
        {"doc_id": "wiki-edison-001", "relevance_score": 3.0},
        {"doc_id": "wiki-incandescent-002", "relevance_score": 2.0}
      ]
    }
  ]
}
```

**Maintenance:**
- Quarterly review and refresh (200 new queries)
- Balanced: 200 queries per domain
- Ground truth: human-curated with inter-annotator agreement ≥0.8

### 3.2 EvaluationRunner

**Purpose:** Execute evaluation queries and compute quality metrics.

**API:**
```cpp
class EvaluationRunner {
  struct QueryResult {
    std::string query_id;
    std::vector<Document> top_k_results;  // k=10
    uint64_t latency_ms;
    float cost_usd;
  };
  
  // Run evaluation on dataset
  struct EvaluationRun {
    std::string run_id;  // "2026-09-24_daily_eval_run"
    int64_t timestamp_us;
    std::vector<QueryResult> results;
    std::map<std::string, float> metrics;  // metric_name → value
  };
  
  EvaluationRun RunEvaluation(
    const std::vector<EvaluationDataset::EvalQuery>& queries
  );
  
  // Compute metrics from results
  struct Metrics {
    float ndcg_at_10;
    float mrr;
    float precision_at_5;
    float precision_at_10;
    float map;  // Mean Average Precision
    float bleu;  // BLEU score for reasoning tasks
    float latency_p95_ms;
    float cost_mean_usd;
  };
  Metrics ComputeMetrics(const EvaluationRun& run);
};
```

**Metrics Definitions:**

| Metric | Formula | Use Case |
|--------|---------|----------|
| nDCG@10 | (Σ rel_i / log(i+1)) / ideal | Overall quality |
| MRR | 1 / rank(first_relevant) | Ranking quality |
| Precision@5 | relevant(1:5) / 5 | Early-rank precision |
| Precision@10 | relevant(1:10) / 10 | Full result set precision |
| MAP | Σ Precision@k / |relevant| | Ranking stability |
| BLEU | N-gram overlap with reference | Reasoning quality |

**Evaluation Cadence:**
- **Daily:** Quick eval (200 queries, 20min runtime)
- **Weekly:** Full eval (1000 queries, 2h runtime)
- **Monthly:** Extended eval + human review (5000 queries)

### 3.3 DriftDetector

**Purpose:** Identify statistically significant quality degradation.

**API:**
```cpp
class DriftDetector {
  struct DriftResult {
    bool has_drifted;
    float drift_magnitude;  // % change from baseline
    float p_value;  // statistical significance
    std::string affected_metric;  // "ndcg_at_10" etc.
    std::string severity;  // "low" | "medium" | "high" | "critical"
    std::vector<std::string> affected_domains;  // which domains show drift
  };
  
  // Detect drift from baseline
  DriftResult DetectDrift(
    const EvaluationRunner::EvaluationRun& current_run,
    const EvaluationRunner::EvaluationRun& baseline_run
  );
  
  // Get drift trend over time
  struct DriftTrend {
    std::vector<float> metric_values;  // over last N runs
    float trend_slope;  // positive = improving, negative = degrading
    float trend_confidence;  // statistical confidence in trend
  };
  DriftTrend GetDriftTrend(
    const std::string& metric_name,
    uint32_t num_runs = 10
  );
  
  // Recommendation engine
  enum class Recommendation { NoAction, Investigate, Rollback, Escalate };
  struct RCARecommendation {
    Recommendation action;
    std::string rationale;
    std::vector<std::string> potentially_affected_components;
    std::string suggested_rollback_version;  // if applicable
  };
  RCARecommendation GetRCARecommendation(const DriftResult& drift);
};
```

**Drift Thresholds:**

| Severity | Condition | Action |
|----------|-----------|--------|
| Low | 1-2% nDCG drop, p > 0.05 | Log, monitor |
| Medium | 2-5% nDCG drop, p < 0.05 | Alert, investigate |
| High | 5-10% nDCG drop, p < 0.01 | Page on-call, RCA |
| Critical | >10% nDCG drop, p < 0.001 | Immediate escalation, rollback consideration |

**Statistical Testing:**
- Null hypothesis: current_metrics ≈ baseline_metrics
- Test: paired t-test on per-domain metrics
- Confidence level: 0.95 (p < 0.05 for significance)

### 3.4 AdversarialEval

**Purpose:** Test robustness to edge cases and adversarial inputs.

**API:**
```cpp
class AdversarialEval {
  enum class AdversaryType { OOD = 0, Typo = 1, Ambiguous = 2, Misleading = 3 };
  
  struct AdversarialQuery {
    std::string base_query_id;
    std::string adversarial_query;
    AdversaryType type;
    std::string description;  // what makes it adversarial
  };
  
  // Generate adversarial queries from seed dataset
  std::vector<AdversarialQuery> GenerateAdversarialQueries(
    const std::vector<EvaluationDataset::EvalQuery>& seed_queries,
    uint32_t num_variants_per_query = 3
  );
  
  // Evaluate adversarial robustness
  struct RobustnessMetrics {
    float recovery_rate;  // % of adversarial queries returning relevant results
    float quality_drop_percent;  // avg nDCG drop vs base query
    float false_positive_rate;  // % returning irrelevant results
  };
  RobustnessMetrics EvaluateRobustness(
    const std::vector<AdversarialQuery>& adversarial_queries
  );
};
```

**Adversarial Types:**

| Type | Example | Expected Behavior |
|------|---------|-------------------|
| OOD | Query in language not in training | Graceful degradation (quality drop <20%) |
| Typo | "photosynthesis" → "photosyntesis" | Spell-correction or semantic tolerance |
| Ambiguous | Pronouns without clear referent | Return top candidates for all interpretations |
| Misleading | Query containing false premise | Return contradictory information, not affirm false premise |

---

## 4. Root Cause Analysis (RCA)

### 4.1 RCA Algorithm

```
IF drift detected THEN:
  candidates = []
  
  FOR component IN [retrieval, ranking, versioning, security]:
    component_metrics = eval_run.component_metrics[component]
    baseline_component = baseline_run.component_metrics[component]
    
    IF delta(component_metrics, baseline_component) > threshold:
      candidates.push_back(component)
  
  IF len(candidates) == 1:
    recommendation = Rollback(candidates[0])
  ELSE IF len(candidates) == 0:
    recommendation = Investigate("unknown cause")
  ELSE:
    recommendation = Investigate(candidates)  // multiple possible causes
```

### 4.2 Component-Level Metrics

```
retrieval_metrics:
  - bm25_recall@50
  - hnsw_recall@50
  - graph_recall@50
  - hybrid_fusion_quality

ranking_metrics:
  - cross_encoder_ranking_quality
  - reranker_cache_hit_rate
  - budget_gate_effectiveness (ROI)

versioning_metrics:
  - embedding_version_freshness
  - index_version_consistency
  - canary_promotion_impact

security_metrics:
  - policy_enforcement_accuracy
  - false_positive_rate
  - false_negative_rate
```

---

## 5. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| Drift detection latency | ≤ 2 evaluation cycles | Timely detection |
| Drift detection accuracy | ≥ 95% (F1 score) | Minimize false alarms |
| RCA precision | ≥ 80% (correct cause identified) | Effective troubleshooting |
| Multi-domain coverage | ≥ 5 domains | Comprehensive evaluation |
| Adversarial robustness | Recovery rate ≥ 80% | Acceptable degradation |

---

## 6. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_EVAL_ENABLED=true|false                   # Feature gate
THEMIS_EVAL_CADENCE=daily|weekly|hourly          # Evaluation frequency
THEMIS_EVAL_DATASET_PATH=/path/to/eval_queries   # Dataset location
THEMIS_DRIFT_DETECTION_THRESHOLD=0.02            # 2% nDCG threshold
THEMIS_EVAL_P_VALUE_THRESHOLD=0.05               # Statistical significance
THEMIS_ADVERSARIAL_EVAL_ENABLED=true|false       # Include adversarial tests
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_RESEARCH_EVAL "Enable Phase 9 research eval harness" ON)
option(THEMIS_ADVERSARIAL_EVAL "Include adversarial robustness tests" ON)
```

---

## 7. Testing Strategy

**Unit Tests (test_evaluation_dataset.cpp):**
- Dataset loading and validation
- Query statistics computation
- Ground truth label consistency
- Edge cases: missing labels, malformed queries

**Unit Tests (test_evaluation_runner.cpp):**
- Metric computation (nDCG, MRR, Precision, MAP, BLEU)
- Metric aggregation by domain
- Latency and cost tracking
- Deterministic evaluation (same input → same output)

**Unit Tests (test_drift_detector.cpp):**
- Drift magnitude calculation
- Statistical significance testing (paired t-test)
- Severity classification
- Trend analysis and forecasting

**Unit Tests (test_adversarial_eval.cpp):**
- Adversarial query generation
- Robustness metric computation
- OOD/typo/ambiguous/misleading classification
- Recovery rate calculation

**Integration Tests (test_eval_pipeline_e2e.cpp):**
- Full evaluation pipeline: dataset → runner → metrics → drift detection
- Multi-run comparison and trend analysis
- RCA recommendation validation
- Dashboard data consistency

**Performance Benchmarks:**
- Daily eval (200 queries): <20min runtime
- Weekly eval (1000 queries): <2h runtime
- Drift detection: <10s computation
- RCA: <5s

---

## 8. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Evaluation logging only (no action)
2. **Week 2:** Drift detection enabled, alert-only
3. **Week 3:** RCA recommendations enabled
4. **Week 4:** Full integration (dashboards, reports)

**Safeguards:**
- Evaluation runs don't impact production queries
- Async evaluation with timeout (max 2h per run)
- Evaluation failures don't trigger alerts
- Manual review before any auto-rollback

**Monitoring:**
- Eval run success/failure rate
- Metric stability over time
- RCA recommendation accuracy
- Dashboard update frequency

---

## 9. Future Enhancements

- **Human-in-the-Loop:** Human reviewers for disagreement resolution
- **Online Learning:** Real-time model retraining from eval feedback
- **Transfer Learning:** Eval metrics from similar domains
- **Federated Eval:** Per-tenant evaluation with privacy preservation

---

**End of Phase 9 Specification**
