# Cost-Latency-Quality Optimizer & Tenant SLO Enforcement Specification
**Phase 10 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q3 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Multi-objective optimization maximizes quality while meeting latency and cost constraints. This phase implements Pareto-frontier exploration to find optimal configuration points per tenant, with automated SLO enforcement and dashboard-driven decision-making.

**Key Objectives:**
- Find Pareto-optimal points on quality-latency-cost frontier
- Enforce tenant-specific SLA agreements automatically
- Provide transparent trade-off analysis to customers
- Reduce operational cost per query by 20-30%

---

## 2. Architecture

### 2.1 Optimizer Stack

```
Current RAG Config
    ├→ Hybrid routing weights
    ├→ Re-ranking budget
    ├→ Embedding freshness
    └→ Security policy strictness
    ↓
ParetoOptimizer
    ├→ Generate candidate configurations
    ├→ Evaluate each: quality, latency, cost
    ├→ Compute Pareto frontier
    └→ Score each point with tenant utility function
    ↓
SLO Constraint Checker
    ├→ Filter: configurations violating hard SLOs
    ├→ Rank: by marginal improvement per cost
    └→ Recommend: best configuration
    ↓
TenantSLOEnforcer
    ├→ Apply: recommended configuration
    ├→ Monitor: compliance with SLA
    └→ Auto-adjust: if SLO violated
    ↓
Dashboard & Decision Support
    ├→ Pareto frontier visualization
    ├→ Trade-off analysis (cost vs quality)
    ├→ Recommendation explanation
    └→ Manual override controls
```

### 2.2 Optimization Space

**Decision Variables:**
```
x = {
  lexical_weight ∈ [0, 0.5],
  dense_weight ∈ [0.3, 1.0],
  graph_weight ∈ [0, 0.3],
  reranker_budget_percent ∈ [0, 100],
  embedding_staleness_max_min ∈ [1, 120],
  security_policy_strictness ∈ [0.7, 1.0]
}
```

**Objective Functions:**
```
Maximize:
  - quality(x) = nDCG@10(x)
  - (quality_improvement_per_cost)

Minimize:
  - latency(x) = p95_latency_ms(x)
  - cost(x) = total_cost_per_query(x)

Constraints:
  - latency(x) ≤ SLA_latency
  - cost(x) ≤ SLA_budget
  - quality(x) ≥ baseline_quality * 0.95
```

---

## 3. Component Specifications

### 3.1 ParetoOptimizer

**Purpose:** Explore configuration space and identify Pareto frontier.

**API:**
```cpp
class ParetoOptimizer {
  struct ConfigPoint {
    std::map<std::string, float> config;  // variable name → value
    float quality_ndcg10;
    float latency_ms;
    float cost_usd;
    bool is_pareto_optimal;
    float hypervolume_contribution;
  };
  
  // Evaluate configuration
  ConfigPoint EvaluateConfig(const std::map<std::string, float>& config);
  
  // Generate and evaluate candidate configurations
  struct OptimizationResult {
    std::vector<ConfigPoint> pareto_frontier;
    std::map<std::string, float> recommended_config;
    float improvement_vs_current;  // % quality improvement
    float cost_reduction;  // $ saved per query
  };
  OptimizationResult OptimizeForTenant(
    const std::string& tenant_id,
    const SLODefinition::SLOSpec& slo
  );
  
  // Candidate generation strategies
  enum class SearchStrategy { GridSearch, RandomSearch, Bayesian, Genetic };
  void SetSearchStrategy(SearchStrategy strategy);
};
```

**Search Strategies:**

| Strategy | Candidates | Runtime | Accuracy |
|----------|-----------|---------|----------|
| Grid | 1000-5000 | 2-10 hours | High |
| Random | 1000 | 1-2 hours | Medium |
| Bayesian | 100-200 | 30 min | High |
| Genetic | 500-1000 | 1-3 hours | High |

**Candidate Evaluation:**
- Simulate with real query traffic sample (100-1000 queries)
- Measure: actual quality, latency, cost (not predictions)
- Aggregate: mean and percentile metrics

### 3.2 TenantSLOEnforcer

**Purpose:** Apply optimized configuration and enforce SLA compliance.

**API:**
```cpp
class TenantSLOEnforcer {
  struct EnforcementPolicy {
    std::string tenant_id;
    std::map<std::string, float> target_config;
    std::map<std::string, float> fallback_config;  // if current fails SLA
    float quality_floor;  // don't drop below
    float latency_ceiling;  // don't exceed
    float cost_ceiling;  // don't exceed
    bool auto_adjust_enabled;
  };
  
  // Apply configuration to tenant
  bool ApplyConfiguration(const EnforcementPolicy& policy);
  
  // Monitor SLA compliance
  struct ComplianceStatus {
    bool is_compliant;
    float quality_current;
    float quality_target;
    float latency_current_p95;
    float latency_target;
    float cost_current;
    float cost_budget;
    std::string status_message;
  };
  ComplianceStatus GetComplianceStatus(const std::string& tenant_id);
  
  // Auto-adjust if SLA breached
  bool AutoAdjustOnBreach(const std::string& tenant_id);
  
  // Export enforcement policy
  std::string ExportPolicyAsYAML(const std::string& tenant_id);
};
```

**Adjustment Logic:**
```
IF quality < floor:
  adjust = "increase_dense_weight"
ELSE IF latency > ceiling:
  adjust = "reduce_reranker_budget"
ELSE IF cost > budget:
  adjust = "increase_staleness_threshold"
ELSE:
  status = "COMPLIANT"
```

### 3.3 TradeoffAnalyzer

**Purpose:** Compute and visualize quality-latency-cost trade-offs.

**API:**
```cpp
class TradeoffAnalyzer {
  // 2D trade-off: quality vs cost
  struct TradeoffCurve {
    std::vector<ConfigPoint> curve_points;  // sorted by cost
    float pareto_area;  // area under curve
    std::map<std::string, float> recommendation;
  };
  TradeoffCurve GetQualityCostTradeoff(
    const std::string& tenant_id
  );
  
  // 3D trade-off: quality vs latency vs cost
  struct VolumetricTradeoff {
    std::vector<ConfigPoint> frontier;
    std::vector<std::string> recommendation_explanations;
    float total_hypervolume;
  };
  VolumetricTradeoff GetMultiObjectiveTradeoff(
    const std::string& tenant_id
  );
  
  // Marginal improvement analysis
  struct MarginalAnalysis {
    float cost_per_1pct_quality_improvement;  // $/1% nDCG gain
    float latency_per_1pct_quality_improvement;
    std::string efficiency_rating;  // "efficient" | "neutral" | "inefficient"
  };
  MarginalAnalysis GetMarginalEfficiency(
    const std::map<std::string, float>& config1,
    const std::map<std::string, float>& config2
  );
};
```

**Trade-off Visualization:**
- 2D scatter: cost (x-axis) vs quality (y-axis)
- 3D scatter: cost vs latency vs quality (with size = hypervolume contribution)
- Shading: feasible region (meets SLA) vs infeasible region (violates SLA)
- Interactive: hover for config details and recommendations

### 3.4 CostAttributor

**Purpose:** Break down costs by component for transparency.

**API:**
```cpp
class CostAttributor {
  struct CostBreakdown {
    float hybrid_retrieval_cost;  // BM25 + HNSW + graph
    float reranking_cost;
    float embedding_cost;
    float security_policy_cost;
    float freshness_monitoring_cost;
    float total_cost;
    std::string cost_breakdown_json;
  };
  
  // Get cost attribution for query
  CostBreakdown AttributeCost(
    const std::string& query_id,
    const std::map<std::string, float>& config
  );
  
  // Tenant cost summary
  struct TenantCostSummary {
    float daily_cost;
    float monthly_projected_cost;
    float cost_per_query;
    CostBreakdown component_breakdown;
    std::vector<std::string> cost_optimization_opportunities;
  };
  TenantCostSummary GetTenantCostSummary(const std::string& tenant_id);
};
```

**Cost Drivers:**

| Component | Cost Factor | Example |
|-----------|------------|---------|
| Dense embedding | tokens used × token_cost | 150 tokens × $0.001 = $0.15 |
| Re-ranking | latency × compute_cost | 50ms × $0.002/ms = $0.10 |
| Freshness monitoring | refresh frequency | 5 refreshes/min × $0.01 = $0.05 |
| Security enforcement | policy checks | 1 check × $0.01 = $0.01 |

---

## 4. Optimization Strategies

### 4.1 Tenant-Specific Utility Functions

```cpp
// Standard tier: optimize for cost
utility = 0.7 * (cost_baseline / cost) + 0.2 * (quality / quality_baseline)

// Premium tier: optimize for quality
utility = 0.6 * (quality / quality_baseline) + 0.3 * (latency_baseline / latency)

// Enterprise tier: balance all three
utility = 0.4 * (quality / quality_baseline) 
        + 0.3 * (latency_baseline / latency)
        + 0.3 * (cost_baseline / cost)
```

### 4.2 Constraint-Based Optimization

```
Tier: Premium
Hard constraints:
  - quality >= 0.72
  - latency_p95 <= 500ms
  - cost <= $0.15/query

Soft constraints (weighted penalty):
  - prefer latency < 300ms (soft deadline)
  - prefer cost < $0.10/query (cost optimization)

Objective: maximize nDCG@10 subject to constraints
```

---

## 5. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| Cost reduction opportunity | ≥ 20% | Financial impact |
| Quality change | ≤ -2% (no regression) | Maintain baseline |
| Pareto frontier accuracy | ≥ 90% | Trust in recommendations |
| Optimization latency | < 2 hours | Practical for daily runs |
| SLA enforcement accuracy | 99%+ | Reliable compliance |

---

## 6. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_OPTIMIZER_ENABLED=true|false              # Feature gate
THEMIS_OPTIMIZER_STRATEGY=bayesian|genetic|grid  # Search method
THEMIS_OPTIMIZER_CANDIDATE_COUNT=500             # Configurations to try
THEMIS_OPTIMIZER_SIMULATION_QUERIES=1000         # Queries to simulate
THEMIS_AUTO_SLO_ENFORCEMENT=true|false           # Auto-apply recommendations
THEMIS_ENFORCEMENT_CHECK_INTERVAL_SEC=300        # How often to check SLA
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_COST_OPTIMIZER "Enable Phase 10 cost optimizer" ON)
option(THEMIS_ENABLE_SLO_ENFORCEMENT "Enable automated SLO enforcement" ON)
```

---

## 7. Testing Strategy

**Unit Tests (test_pareto_optimizer.cpp):**
- Pareto frontier computation (verify optimality)
- Configuration evaluation accuracy
- Search strategy correctness
- Edge cases: single objective, no feasible solutions

**Unit Tests (test_tenant_slo_enforcer.cpp):**
- Policy application and enforcement
- SLA compliance status calculation
- Auto-adjust logic (quality floor, latency ceiling, cost budget)
- Fallback configuration handling

**Unit Tests (test_tradeoff_analyzer.cpp):**
- Trade-off curve computation
- Marginal efficiency analysis
- Hypervolume calculation
- Recommendation generation

**Unit Tests (test_cost_attributor.cpp):**
- Cost breakdown computation (component attribution)
- Tenant cost summary aggregation
- Cost optimization opportunity identification
- Edge cases: zero cost components

**Integration Tests (test_optimizer_e2e.cpp):**
- Full optimization pipeline: candidate generation → evaluation → frontier
- Multi-tenant optimization with different SLO tiers
- SLO enforcement with compliance monitoring
- Auto-adjustment on SLA breach

**Performance Benchmarks:**
- Pareto frontier: <2 hours for 1000 candidates
- Configuration evaluation: <1min per candidate
- SLA compliance check: <10ms p99
- Trade-off analysis: <30s

---

## 8. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Optimization analysis only (no auto-enforcement)
2. **Week 2:** Manual enforcement (dashboard recommendations)
3. **Week 3:** Auto-enforcement for beta tenants (5% of traffic)
4. **Week 4:** Full auto-enforcement

**Safeguards:**
- Manual approval required before applying new configuration
- Automatic rollback if SLA breached after enforcement
- Max 10% quality degradation allowed (hard stop)
- Daily optimization only (not per-query)

**Monitoring:**
- Dashboard: recommended vs applied configurations
- Cost savings realized per tenant
- SLA compliance post-enforcement
- Accuracy of recommendations vs actual outcomes

---

## 9. Dashboard & Reporting

### 9.1 Pareto Frontier Dashboard

```
Title: Cost-Quality Trade-off Analysis

Left Panel:
  - 2D scatter: cost (x) vs quality (y)
  - Points: all configurations
  - Highlighted: Pareto frontier
  - Shaded region: infeasible (SLA violation)

Right Panel:
  - Table: top 10 configurations
  - Columns: quality, latency_p95, cost, utility_score
  - Actions: [Apply] [Save] [Share]

Bottom:
  - Cost reduction opportunity: 23%
  - Quality change if applied: -0.5%
  - Confidence: 92%
```

### 9.2 Tenant SLO Dashboard

```
Tile per tenant:
  [Tenant Name] - [Tier]
  
  Status: ✓ COMPLIANT | ⚠ AT RISK | ✗ BREACHED
  
  Quality: 0.72 / 0.72 (100%)
  Latency p95: 450ms / 500ms (90%)
  Cost: $0.12 / $0.15 (80%)
  
  Trend: ↑ Improving | → Stable | ↓ Degrading
  
  [View Details] [Apply Optimization]
```

---

## 10. Future Enhancements

- **Online Learning:** Real-time frontier refinement from query feedback
- **Predictive Optimization:** Forecast future SLA needs and pre-optimize
- **Federated Optimization:** Per-region or per-feature optimization
- **Human-in-the-Loop:** Expert input on utility functions per tenant

---

**End of Phase 10 Specification**
