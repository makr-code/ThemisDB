# Cross-Encoder Re-Ranking Budget Gate Specification
**Phase 6 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q1 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Cross-encoder re-ranking improves retrieval quality but adds latency and cost. The reranking budget gate ensures re-ranking is applied only when ROI (quality improvement) exceeds cost, preventing wasteful re-ranking and maintaining cost efficiency.

**Key Objectives:**
- Apply re-ranking only when predicted quality gain > cost
- ≥80% of rerank calls show measurable nDCG lift (>2%)
- Transparent, auditable cost accounting
- Per-tenant budget enforcement

---

## 2. Architecture

### 2.1 Component Stack

```
Hybrid Retrieval Results (k=50)
    ↓
RerankerBudgetGate
    ├→ Estimate re-ranking cost (model latency + infrastructure)
    ├→ Estimate quality gain (nDCG improvement based on query)
    ├→ Compute ROI: gain / cost
    └→ Decision: rerank or bypass
    ↓
IF rerank decision:
    ↓
    CrossEncoderOrchestrator
    ├→ Load cross-encoder model
    ├→ Batch re-encoding (for efficiency)
    ├→ Re-rank results
    └→ Emit cost/quality metrics
    ↓
    Reranked Results (k=10)
    ↓
    RerankerCostAnalyzer
    ├→ Record cost: latency + model inference
    ├→ Record quality: nDCG improvement
    └→ Emit metrics to OTLP
ELSE:
    ↓
    Top-k from hybrid retrieval
```

### 2.2 Budget Enforcement

**Cost Model:**
```
cost = inference_time_ms * unit_cost + (context_tokens * token_cost)
```

**Per-Query Budget:**
```
budget_per_query = total_budget / expected_query_count
```

**Per-Tenant Budget:**
```
tenant_budget = fixed_quota + (percentage * usage_based_quota)
```

---

## 3. Component Specifications

### 3.1 RerankerBudgetGate

**Purpose:** Decide whether to apply re-ranking based on ROI estimation.

**API:**
```cpp
class RerankerBudgetGate {
  struct BudgetContext {
    std::string tenant_id;
    std::string query_id;
    const std::string& query_text;
    const std::vector<Document>& hybrid_results;  // k=50
    float available_budget;  // ms or credits
  };
  
  struct RankingROI {
    bool should_rerank;
    float estimated_quality_gain;  // nDCG@10 delta
    float estimated_cost;  // ms or credits
    float roi_ratio;  // gain / cost
    std::string rationale;  // explanation for decision
  };
  
  // Estimate ROI and make reranking decision
  RankingROI EstimateROI(const BudgetContext& context);
  
  // Check tenant budget status
  struct BudgetStatus {
    float consumed;
    float available;
    float percentage_used;
  };
  BudgetStatus GetTenantBudgetStatus(const std::string& tenant_id);
  
  // Enforce budget ceiling
  bool ConsumeBudget(const std::string& tenant_id, float cost);
};
```

**ROI Estimation:**
```
estimated_quality_gain = 
  base_gain * query_complexity_factor * result_uncertainty_score

estimated_cost = 
  cross_encoder_latency_p99 + (context_tokens * token_cost)

roi_ratio = estimated_quality_gain / estimated_cost

decision = (roi_ratio > roi_threshold) AND (available_budget >= estimated_cost)
```

**Thresholds:**
- ROI threshold: 0.1 (1% quality gain / 10ms cost = apply)
- Budget reserve: 10% (never consume beyond 90%)
- Max per-query cost: 100ms (hard cap)

**Query Complexity Factors:**
- Short queries (< 10 tokens): 0.6x (lower ROI expected)
- Medium queries (10-50 tokens): 1.0x (baseline)
- Long queries (> 50 tokens): 1.2x (higher ROI expected)

**Result Uncertainty Score:**
```
uncertainty = variance(hybrid_scores[0:5])
gain_boost = uncertainty / threshold
```
- High uncertainty → higher quality gain from reranking
- Low uncertainty (clear top result) → skip reranking

### 3.2 CrossEncoderOrchestrator

**Purpose:** Execute cross-encoder re-ranking with batch efficiency.

**API:**
```cpp
class CrossEncoderOrchestrator {
  struct RerankerConfig {
    std::string model_name;        // e.g., "cross-encoder/ms-marco-MiniLMv2-L12-H384"
    uint32_t batch_size;          // default 32
    float confidence_threshold;   // below this score, skip reranking
  };
  
  // Initialize orchestrator
  explicit CrossEncoderOrchestrator(const RerankerConfig& config);
  
  struct RerankerBatch {
    std::string query_id;
    std::string query_text;
    std::vector<Document> documents;  // k=50
    std::vector<uint64_t> doc_ids;
  };
  
  struct RerankedResults {
    std::vector<Document> reranked_docs;  // k=10
    std::vector<float> relevance_scores;
    uint64_t latency_ms;
    uint32_t tokens_used;
  };
  
  // Rerank single query
  RerankedResults Rerank(const std::string& query_text,
                         const std::vector<Document>& docs);
  
  // Batch rerank (for efficiency)
  std::vector<RerankedResults> ReankBatch(
    const std::vector<RerankerBatch>& batches
  );
};
```

**Batching Strategy:**
- Accumulate rerank requests for 100ms or 32 queries (whichever first)
- Encode queries + documents together for efficiency
- Return results in original request order

**Fallback (on model load failure):**
- Return top-k from hybrid retrieval
- Log error with context
- Don't consume budget

**Caching:**
- Query embedding cache (for repeated queries)
- Document embedding cache (soft cache, 1-hour TTL)
- Hit rate target: ≥20% (saves reranking cost)

### 3.3 RerankerCostAnalyzer

**Purpose:** Track and enforce re-ranking costs per tenant and query.

**API:**
```cpp
class RerankerCostAnalyzer {
  struct CostRecord {
    std::string query_id;
    std::string tenant_id;
    uint64_t request_time_ms;
    uint32_t tokens_used;
    float quality_improvement;  // nDCG@10 delta
    bool was_beneficial;  // improvement > 2%
    std::string cost_breakdown;  // JSON for detailed accounting
  };
  
  // Record cost and quality outcome
  void RecordCostAndQuality(const CostRecord& record);
  
  // Get cost trends (hourly aggregation)
  struct CostMetrics {
    float p50_cost_ms;
    float p95_cost_ms;
    float p99_cost_ms;
    float mean_quality_improvement;
    float beneficial_rate;  // % with improvement > 2%
  };
  CostMetrics GetCostMetrics(
    const std::string& tenant_id,
    const std::chrono::system_clock::time_point& since
  );
  
  // Tenant cost allocation
  struct TenantCostAllocation {
    std::string tenant_id;
    float daily_budget;
    float consumed_today;
    float forecasted_total;
    bool is_at_risk;  // will exceed budget?
  };
  std::vector<TenantCostAllocation> GetTenantAllocations();
};
```

**Cost Tracking:**
```
cost_unit = base_unit_cost * tokens_used + latency_penalty * latency_ms

Examples:
- 100ms inference, 150 tokens: cost = 0.001 * 150 + 0.01 * 100 = 1.15 units
- 30ms inference, 100 tokens: cost = 0.001 * 100 + 0.01 * 30 = 0.4 units
```

**Beneficial Rate Target:** ≥80%
- Reranking is cost-effective only if ≥80% of calls improve quality
- Monitor daily; alert if below threshold

**Tenant Budget Forecasting:**
```
daily_consumption = current_hour * (24 / hour_of_day)
at_risk = daily_consumption > daily_budget * 0.8
```

---

## 4. Budget Allocation Models

### 4.1 Fixed Quota Model
```
tenant_daily_budget = tier_quota[tenant.tier]
Examples:
- Free: 1000 rerank operations/day
- Professional: 100K rerank operations/day
- Enterprise: Unlimited
```

### 4.2 Usage-Based Model
```
tenant_daily_budget = base_quota + (usage_percentage * overage_quota)

Example:
- Base: 10K reranks/day included
- Overage: $0.001 per rerank, up to $100/day
- If used 15K reranks: 10K (base) + 5K (at $0.001) = $5 charge
```

### 4.3 SLO-Aware Model
```
budget_multiplier = tenant_slo_compliance_ratio

If tenant maintains 99%+ SLA: multiplier = 1.2x (bonus)
If tenant below 95%: multiplier = 0.7x (penalty)
```

---

## 5. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| Beneficial rerank rate | ≥80% | Cost-effectiveness |
| Quality improvement (when beneficial) | >2% nDCG@10 | Minimum ROI bar |
| Cost accounting accuracy | ±5% variance | Trust in billing |
| Budget enforcement | 100% within ceiling | Financial control |
| Fallback availability | ≥99.9% | Graceful degradation |
| Cache hit rate | ≥20% | Cost reduction |

---

## 6. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_RERANKER_ENABLED=true|false              # Feature gate
THEMIS_RERANKER_ROI_THRESHOLD=0.1               # ROI decision threshold
THEMIS_RERANKER_BATCH_SIZE=32                   # Batch size
THEMIS_RERANKER_MODEL_NAME="cross-encoder/ms-marco-MiniLMv2-L12-H384"
THEMIS_RERANKER_COST_MODEL=token_cost|time_cost # Cost computation method
THEMIS_BUDGET_ENFORCEMENT_MODE=hard|soft        # hard = reject, soft = warn
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_RERANKER_BUDGET_GATE "Enable Phase 6 reranker gate" ON)
```

---

## 7. Testing Strategy

**Unit Tests (test_reranker_budget_gate.cpp):**
- ROI estimation correctness on diverse queries
- Budget enforcement logic (consume, check balance)
- Tenant budget status computation
- Edge cases: zero budget, negative cost, overflow

**Unit Tests (test_cross_encoder_orchestrator.cpp):**
- Single query reranking with model loading
- Batch reranking determinism
- Cache hit/miss behavior
- Fallback on model load failure

**Unit Tests (test_reranker_cost_analyzer.cpp):**
- Cost record storage and retrieval
- Aggregation metrics (p50, p95, p99)
- Tenant allocation forecasting
- At-risk detection

**Integration Tests (test_reranking_e2e.cpp):**
- Full pipeline: hybrid results → budget gate → reranker → cost tracking
- Budget ceiling enforcement with concurrent requests
- Multi-tenant cost isolation
- SLA-aware budget multiplier

**Performance Benchmarks:**
- Batch reranking throughput: >100 queries/sec
- Cost estimation latency: <5ms p99
- Cache lookup: <1ms p99

---

## 8. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Disabled by default; internal QA with monitoring
2. **Week 2:** Enabled for beta tenants (10% traffic)
3. **Week 3:** Enabled for all tenants with audit mode (no budget enforcement)
4. **Week 4:** Full budget enforcement

**Safeguards:**
- Soft enforcement (warning) for first week
- Hard ceiling: max 100ms per query (no exceptions)
- Auto-disable if beneficial rate drops below 70%

**Monitoring:**
- Dashboard: beneficial rate, cost/query, budget consumption
- Alerts: beneficial rate <75%, cost anomaly >2σ
- SLA: 99%+ reranker availability

---

## 9. Cost Transparency

**Customer Visibility:**
- Per-query cost breakdown (model latency + tokens)
- Daily cost summary by tenant
- Billing invoice with reranking line item
- Cost projection and recommendations

**Audit Trail:**
- Every rerank decision logged (question + cost + quality improvement)
- Queryable audit trail for compliance
- Export for financial reconciliation

---

## 10. Future Enhancements

- **Adaptive ROI:** Personalized ROI thresholds per tenant
- **Ensemble Rerankers:** Multiple models for diversity
- **Learned Budget Allocation:** ML-based budget assignment
- **Quality-Cost Trade-offs:** Pareto-frontier exploration

---

**End of Phase 6 Specification**
