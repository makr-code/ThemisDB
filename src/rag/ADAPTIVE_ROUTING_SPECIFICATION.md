# Adaptive Hybrid Routing & Learned Policy Specification
**Phase 5 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q1 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Adaptive hybrid routing optimizes retrieval strategy based on query intent classification, enabling per-query optimization of lexical vs dense vs graph retrieval without sacrificing latency or cost. Unlike static hybrid routing (fixed lexical/dense/graph weights), adaptive routing learns from telemetry feedback to dynamically adjust weights per query intent.

**Key Objectives:**
- Improve retrieval quality (nDCG@10) by +5% over static routing
- Keep latency overhead ≤10%
- Deterministic, auditable routing decisions
- Feedback loop for continuous optimization

---

## 2. Architecture

### 2.1 Component Stack

```
Query Request
    ↓
QueryIntentClassifier (NLP)
    ├→ Intent category: factual|temporal|multi-hop|comparison
    └→ Confidence score
    ↓
AdaptiveHybridRouter
    ├→ Lookup intent-specific weights from RouterPolicyStore
    ├→ Compute routing decision: (lexical%, dense%, graph%)
    └→ Execute retrieval with allocated weights
    ↓
Retrieval (BM25 + HNSW + Graph)
    ↓
Telemetry Aggregation
    ├→ Collect metrics: nDCG@10, latency, cost
    ├→ Group by intent + policy version
    └→ Store in RouterPolicyStore
    ↓
Offline Feedback Loop (async)
    └→ Retraining: update policy weights based on metrics
```

### 2.2 Data Flow

**Request-Time:**
1. Query arrives → QueryIntentClassifier extracts intent
2. AdaptiveHybridRouter retrieves current weights for intent from RouterPolicyStore
3. Computes (lexical%, dense%, graph%) from weights
4. Executes hybrid retrieval with allocated weights
5. Collects query-level metrics

**Feedback-Time (async, hourly/daily):**
1. Group queries by (intent, policy_version)
2. Compute aggregated metrics (nDCG@10, latency, cost)
3. Compare against baseline
4. If improvement > threshold: promote new policy
5. Otherwise: retain current policy with reduced confidence

---

## 3. Component Specifications

### 3.1 QueryIntentClassifier

**Purpose:** Classify query intent into categories for routing decisions.

**API:**
```cpp
class QueryIntentClassifier {
  enum class Intent { Factual = 0, Temporal = 1, MultiHop = 2, Comparison = 3 };
  
  struct ClassificationResult {
    Intent intent;
    float confidence;  // [0, 1]
    std::map<Intent, float> intent_scores;  // softmax over all intents
  };
  
  // Classification from query text
  ClassificationResult Classify(const std::string& query_text);
  
  // Batch classification
  std::vector<ClassificationResult> ClassifyBatch(
    const std::vector<std::string>& queries
  );
};
```

**Intent Categories:**

| Intent | Description | Routing Hint | Example |
|--------|-------------|--------------|---------|
| Factual | Direct answer lookup (person, place, date) | Dense > Lexical > Graph | "Who is the president of France?" |
| Temporal | Time-sensitive queries | Lexical + Graph > Dense | "COVID-19 case counts Q3 2026" |
| MultiHop | Requires chaining facts | Dense + Graph > Lexical | "Name founding members of bands that played Glastonbury 2025" |
| Comparison | Comparative reasoning | Lexical ≈ Dense, Graph for context | "Difference between TypeScript and Kotlin?" |

**Training:**
- Pre-trained on open QA datasets (SQUAD, NQ, HotpotQA, etc.)
- Fine-tuned on internal ThemisDB query logs
- Confidence threshold: 0.7 (below threshold → fallback to static routing)

**Error Handling:**
- Classify queries failing on NLP → Intent::Factual (safest)
- Null/empty queries → Intent::Factual
- Batch > 1000 queries → queue and process async

### 3.2 AdaptiveHybridRouter

**Purpose:** Route queries to lexical/dense/graph retrieval with adaptive weights.

**API:**
```cpp
class AdaptiveHybridRouter {
  struct RoutingDecision {
    float lexical_weight;      // [0, 1]
    float dense_weight;        // [0, 1]
    float graph_weight;        // [0, 1]
    std::string policy_version;
    uint64_t decision_id;      // for telemetry tracking
  };
  
  struct QueryContext {
    std::string query_text;
    QueryIntentClassifier::ClassificationResult intent;
    std::string tenant_id;
  };
  
  // Compute routing decision
  RoutingDecision Route(const QueryContext& context);
  
  // Retrieve RRF-fused results
  std::vector<Document> RetrieveAdaptive(
    const QueryContext& context,
    const RoutingDecision& decision
  );
};
```

**Routing Logic:**
1. Look up current policy for intent from RouterPolicyStore
2. Extract (lexical%, dense%, graph%) weights
3. Validate weights: must sum to 1.0 ± 0.01
4. Emit routing decision with unique ID (for telemetry correlation)
5. Execute hybrid retrieval: RRF-fuse results with weights

**Fallback (on policy miss):**
- Use static defaults: (lexical=0.3, dense=0.5, graph=0.2)
- Log miss event for debugging

**Telemetry Emission:**
- Emit OpenTelemetry span with decision_id, intent, weights, policy_version
- Span attributes: `rag.intent`, `rag.lexical_weight`, `rag.dense_weight`, `rag.graph_weight`

### 3.3 RouterPolicyStore

**Purpose:** Persist and version routing policies with RocksDB backend.

**Schema:**
```
RocksDB {
  "default" CF:
    "active_policy_version"   → uint32 (current version)
    "policy:<intent>:<version>" → PolicySpec JSON
    "policy_metrics:<intent>:<version>" → MetricsSnapshot JSON
  "policy_history" CF:
    uint64_be(timestamp)      → HistoryEntry JSON (version, intent, metrics_delta, decision)
}
```

**PolicySpec JSON:**
```json
{
  "version": 1,
  "intent": "factual",
  "lexical_weight": 0.3,
  "dense_weight": 0.55,
  "graph_weight": 0.15,
  "created_at": "2026-09-24T10:00:00Z",
  "baseline_ndcg": 0.72,
  "confidence": 0.95
}
```

**API:**
```cpp
class RouterPolicyStore {
  struct PolicySpec {
    uint32_t version;
    QueryIntentClassifier::Intent intent;
    float lexical_weight, dense_weight, graph_weight;
    std::string created_at;
    float baseline_ndcg;
    float confidence;  // 0-1, drop below 0.5 for review
  };
  
  // Get current policy for intent
  std::optional<PolicySpec> GetCurrentPolicy(QueryIntentClassifier::Intent intent);
  
  // Get policy by version
  std::optional<PolicySpec> GetPolicyByVersion(
    QueryIntentClassifier::Intent intent,
    uint32_t version
  );
  
  // Update policy (new version)
  bool UpdatePolicy(const PolicySpec& new_policy);
  
  // Rollback to previous policy version
  bool RollbackPolicy(QueryIntentClassifier::Intent intent, uint32_t version);
  
  // Store query-level metrics for feedback loop
  bool RecordMetrics(
    const std::string& decision_id,
    const QueryIntentClassifier::Intent intent,
    float nDCG_at_10,
    float latency_ms,
    float cost
  );
};
```

---

## 4. Offline Feedback Loop

**Purpose:** Train and update routing policies based on query-level telemetry.

**Frequency:** Daily batch run (configurable, e.g., hourly for high-volume tenants)

**Algorithm:**
```
FOR EACH intent IN [Factual, Temporal, MultiHop, Comparison]:
  LOAD metrics for (intent, active_policy_version)
  COMPUTE mean(nDCG@10), mean(latency), mean(cost)
  
  IF mean(nDCG@10) > baseline + 0.02:
    // Improvement found
    IF confidence(improvement) > 0.95:
      PROMOTE policy to next version
      RESET confidence to 0.5 (conservative)
    ELSE:
      // Not yet confident
      INCREMENT training_count
  ELSE IF mean(nDCG@10) < baseline - 0.02:
    // Regression
    ROLLBACK to previous policy
    LOG alert: "Policy regression for {intent}"
  ELSE:
    // No significant change
    NO-OP
```

**Input:** Query-level metrics from RouterPolicyStore  
**Output:** New or promoted PolicySpec  
**Trigger:** Batch job (daily 00:00 UTC)  
**Implementation:** Async task runner

---

## 5. A/B Testing Framework

**Purpose:** Validate policy improvements before promotion.

**Structure:**
```cpp
class AdaptiveRoutingABTest {
  struct TestConfig {
    QueryIntentClassifier::Intent intent;
    PolicySpec control;      // current (baseline)
    PolicySpec treatment;    // candidate
    uint32_t sample_size;    // queries to allocate to treatment
    float confidence_threshold;  // 0.95 for 95% CI
  };
  
  // Start A/B test
  bool StartTest(const TestConfig& config);
  
  // Get test status
  struct TestResult {
    bool is_winner;  // treatment better than control?
    float improvement;  // (nDCG_treatment - nDCG_control) / nDCG_control
    float p_value;
    std::string conclusion;
  };
  TestResult GetTestResult(QueryIntentClassifier::Intent intent);
};
```

**Allocation:**
- Control (baseline policy): 90% of queries
- Treatment (candidate policy): 10% of queries
- Sample size: min(1000, daily_queries / 10)

**Stopping Rule:** 
- If treatment nDCG > control by ≥2% with p-value < 0.05 → declare winner
- Min sample size: 100 queries per group
- Max duration: 7 days

---

## 6. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| nDCG@10 improvement | ≥+5% vs static routing | Primary quality metric |
| Latency overhead | ≤10% | Ensure responsiveness |
| Intent classification accuracy | ≥100% on validation set | Routing must be correct |
| Policy promotion confidence | ≥0.95 (statistical) | Avoid false positives |
| A/B test p-value | <0.05 | Standard statistical rigor |
| Fallback rate | ≤1% (policy misses) | System availability |

---

## 7. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_ADAPTIVE_ROUTING_ENABLED=true|false       # Feature gate
THEMIS_ROUTER_INTENT_THRESHOLD=0.7               # Classification confidence
THEMIS_ROUTER_IMPROVEMENT_THRESHOLD=0.02         # 2% nDCG delta
THEMIS_ROUTER_FEEDBACK_FREQUENCY_HOURS=24       # Batch update cadence
THEMIS_ROUTER_CONFIDENCE_THRESHOLD=0.95          # Policy promotion CI
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_ADAPTIVE_ROUTING "Enable Phase 5 adaptive routing" ON)
```

---

## 8. Testing Strategy

**Unit Tests (test_query_intent_classifier.cpp):**
- Intent classification accuracy on 20+ sample queries
- Confidence score correctness (softmax validation)
- Batch classification determinism
- Fallback on NLP errors

**Unit Tests (test_adaptive_hybrid_router.cpp):**
- Routing decision computation (weights sum to 1.0)
- Policy lookup and fallback behavior
- Telemetry emission with correct span attributes
- RRF fusion with adaptive weights

**Unit Tests (test_router_policy_store.cpp):**
- CRUD operations (Create, Read, Update, Rollback)
- Version management and history tracking
- Metrics recording and aggregation
- JSON serialization round-trips

**Integration Tests (test_adaptive_routing_e2e.cpp):**
- End-to-end query flow: intent → routing → retrieval
- Feedback loop: metrics collection → policy update
- A/B testing framework validation
- Promotion criteria validation

**Performance Benchmarks:**
- Intent classification latency: <5ms p99
- Routing decision latency: <1ms p99
- Policy store lookup: <2ms p99

---

## 9. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Internal QA with small query volume (1% traffic)
2. **Week 2:** Beta tenants with monitoring (5% traffic)
3. **Week 3:** General availability with alerts (100% traffic)

**Rollback Plan:**
- Auto-rollback if nDCG regression > 3%
- Manual rollback via environment variable flip
- Policy rollback to previous version at any time

**Monitoring:**
- Dashboard: nDCG trend, latency distribution, cost/query
- Alerts: regression >2%, confidence drop <0.5
- SLA: 99%+ adaptive routing availability

---

## 10. Future Enhancements

- **Online Learning:** Real-time policy updates (online gradient descent)
- **Federated Learning:** Per-tenant policy specialization
- **Temporal Decay:** Weight recent feedback more heavily
- **Cost Optimization:** Balance quality vs. cost per tenant

---

**End of Phase 5 Specification**
