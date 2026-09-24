# Observability SLO & Telemetry Specification
**Phase 8 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q2 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Observability ensures operators can detect, diagnose, and remediate SLO breaches within 60 seconds. This phase defines SLO contracts, real-time tracking, and automated telemetry integration to maintain 99% RAG service SLA.

**Key Objectives:**
- Define authoritative SLO targets (latency, quality, cost)
- Track compliance in real-time with sub-second latency
- Detect breaches and emit alerts within 60 seconds
- Integrate with OTLP for centralized observability

---

## 2. Architecture

### 2.1 SLO Stack

```
RAG Service Requests
    ↓
Request Instrumentation (spans + metrics)
    ├→ Latency: query reception → results delivered
    ├→ Quality: nDCG@10 estimation
    └→ Cost: tokens + model inference cost
    ↓
SLOTracker (real-time aggregation)
    ├→ Windowed percentile tracking (5min, 1h, 1d)
    ├→ Burn rate computation
    └→ Compliance status (in-SLO vs out-of-SLO)
    ↓
Alerting & Escalation
    ├→ 60s detection threshold (multi-window voting)
    ├→ Escalation: P3 (warning) → P2 (urgent) → P1 (critical)
    └→ Notification: Slack + PagerDuty
    ↓
Dashboard & Reporting
    ├→ Real-time SLO compliance
    ├→ Burn rate trends
    └→ Root cause analysis (by component/tenant)
```

### 2.2 SLO Definitions

**Three Tiers:**

| Tier | Latency p95 | Quality nDCG@10 | Cost $/query | Compliance | Penalty |
|------|-------------|-----------------|-------------|------------|---------|
| Standard | 1s | 0.65 | $0.10 | 95% | SLA credit |
| Premium | 500ms | 0.72 | $0.15 | 99% | SLA credit |
| Enterprise | 300ms | 0.75 | $0.20 | 99.5% | Service credit |

---

## 3. Component Specifications

### 3.1 SLODefinition

**Purpose:** Define and version SLO contracts.

**API:**
```cpp
class SLODefinition {
  enum class Tier { Standard = 0, Premium = 1, Enterprise = 2 };
  
  struct SLOSpec {
    Tier tier;
    uint32_t latency_p95_ms;
    uint32_t latency_p99_ms;
    float quality_target_ndcg;
    float cost_per_query_usd;
    uint32_t compliance_target_percent;  // 95, 99, 99.5
    std::string created_at;
    uint32_t version;
  };
  
  // Get SLO spec for tier
  std::optional<SLOSpec> GetSLOSpec(Tier tier);
  
  // Get all SLO specs
  std::map<Tier, SLOSpec> GetAllSLOSpecs();
  
  // Define custom SLO for tenant
  bool SetCustomSLO(const std::string& tenant_id, const SLOSpec& spec);
  
  // Get SLO for tenant
  std::optional<SLOSpec> GetTenantSLO(const std::string& tenant_id);
};
```

**SLO Versioning:**
- Version bumps on any threshold change
- Previous version remains active for 24h (migration window)
- Audit trail of all SLO changes

### 3.2 SLOTracker

**Purpose:** Track SLO compliance in real-time with sub-second updates.

**API:**
```cpp
class SLOTracker {
  struct RequestMetrics {
    std::string request_id;
    std::string tenant_id;
    int64_t timestamp_us;
    uint64_t latency_ms;
    float quality_ndcg10;
    float cost_usd;
  };
  
  // Record request metrics
  void RecordMetrics(const RequestMetrics& metrics);
  
  // Get real-time compliance status
  struct ComplianceStatus {
    bool is_compliant;           // within SLO?
    double compliance_percent;   // % of requests meeting SLO
    uint32_t requests_in_slo;
    uint32_t requests_out_of_slo;
    uint64_t burn_rate_percent;  // % of error budget consumed
    std::string status_message;
  };
  ComplianceStatus GetComplianceStatus(
    const std::string& tenant_id,
    const SLODefinition::Tier tier
  );
  
  // Get percentile distribution
  struct PercentileStats {
    uint64_t p50_ms, p95_ms, p99_ms;
    float quality_p50, quality_p95, quality_p99;
    float cost_p50, cost_p95, cost_p99;
  };
  PercentileStats GetLatencyStats(
    const std::string& tenant_id,
    const std::chrono::duration<int, std::milli>& window = 5min
  );
  
  // Multi-window voting (5min, 1h, 1d windows)
  struct ComplianceVote {
    bool window_5min_compliant;
    bool window_1h_compliant;
    bool window_1d_compliant;
    uint32_t votes_in_slo;  // 0-3
    bool consensus_breached;  // 2+ windows out-of-SLO
  };
  ComplianceVote VoteCompliance(const std::string& tenant_id, Tier tier);
};
```

**Windowing Strategy:**
- **5-minute window:** Real-time tracking, frequent updates
- **1-hour window:** Detect sustained issues
- **1-day window:** Trend analysis and SLA monthly calculation
- All windows updated on each request (streaming)

**Burn Rate Calculation:**
```
error_budget_daily = (100 - compliance_target) * 1%
Example: 99% SLA → 1% error budget per day

burn_rate = consumed_budget / elapsed_time
Example: 0.5% consumed in 1 hour → 12% hourly burn rate

alert_if: burn_rate > 10% (would exceed budget in 10 hours)
```

### 3.3 TelemetryGate

**Purpose:** Emit OpenTelemetry spans and metrics with guaranteed delivery.

**API:**
```cpp
class TelemetryGate {
  // Emit OTLP span for request
  struct SpanContext {
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    std::string request_id;
    std::string tenant_id;
    std::string operation_name;  // "rag.query" etc.
  };
  
  void EmitRequestSpan(
    const SpanContext& ctx,
    const SLOTracker::RequestMetrics& metrics
  );
  
  // Emit custom metric
  void EmitMetric(
    const std::string& metric_name,
    double value,
    const std::map<std::string, std::string>& labels = {}
  );
  
  // Emit compliance alert
  struct AlertContext {
    std::string tenant_id;
    SLODefinition::Tier tier;
    std::string breach_type;  // "latency" | "quality" | "cost"
    double current_value;
    double slo_threshold;
    uint32_t duration_sec;
  };
  bool EmitAlert(const AlertContext& ctx);
};
```

**OTLP Span Attributes:**
```
Span: rag.query_execution
  Attributes:
    - rag.request_id (string)
    - rag.tenant_id (string)
    - rag.latency_ms (int)
    - rag.quality_ndcg10 (float)
    - rag.cost_usd (float)
    - rag.tier (string: standard|premium|enterprise)
    - rag.compliance_status (string: compliant|out_of_slo)
    - http.status_code (int)
  Events:
    - "slo_breach" when latency > threshold
    - "quality_degradation" when nDCG < target
```

**Metric Types:**
```
Metrics:
  - rag.request_count (Counter, by tenant + tier)
  - rag.request_latency_ms (Histogram, p50/p95/p99)
  - rag.request_quality_ndcg10 (Histogram)
  - rag.request_cost_usd (Histogram)
  - rag.slo_compliance_percent (Gauge, updated every 5min)
  - rag.burn_rate_percent (Gauge, updated every minute)
```

**Telemetry Delivery:**
- Batches: 100 spans per batch or 5s timeout
- Compression: gzip
- Retry: exponential backoff (max 3 retries, 5min total)
- Fallback: drop metrics if OTLP unavailable (don't fail requests)

---

## 4. Alert Rules

### 4.1 Alert Thresholds

| Alert Level | Condition | Duration | Action |
|-------------|-----------|----------|--------|
| P3 (Warning) | 1 SLO window breached | 5min | Slack notification, create dashboard link |
| P2 (Urgent) | 2+ windows breached OR burn_rate > 50% | 5min | Slack + Page on-call engineer |
| P1 (Critical) | 3 windows breached OR burn_rate > 100% | 1min | All above + incident escalation |

### 4.2 Alert Context

Every alert includes:
- Tenant ID and tier
- Metrics: current value, SLO threshold, % over
- Trend: improving or worsening
- Estimated SLA impact: "5% SLA at risk in 2 hours"
- Recommended action: "increase index refresh" / "reduce re-ranker budget"

---

## 5. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| SLO compliance tracking | ±2% accuracy | Trust in metrics |
| Alert detection latency | < 60s | Operator response time |
| Alert accuracy (true positives) | ≥ 95% | Avoid alert fatigue |
| Telemetry delivery rate | ≥ 99.9% | Complete observability |
| Dashboard update latency | < 5s | Real-time visibility |

---

## 6. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_SLO_ENABLED=true|false                    # Feature gate
THEMIS_SLO_TIER=standard|premium|enterprise      # Default SLO tier
THEMIS_ALERT_DETECTION_WINDOW_SEC=60             # Alert latency SLA
THEMIS_OTLP_ENDPOINT=http://otel:4317            # OTLP collector
THEMIS_TELEMETRY_SAMPLE_RATE=1.0                 # 1.0 = 100%, 0.1 = 10%
THEMIS_BURN_RATE_THRESHOLD_PERCENT=50            # P2 alert threshold
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_OBSERVABILITY_SLO "Enable Phase 8 observability SLO" ON)
option(THEMIS_TELEMETRY_EXPORT_OTLP "Export telemetry to OTLP" ON)
```

---

## 7. Testing Strategy

**Unit Tests (test_slo_definition.cpp):**
- SLO spec versioning
- Tier-based SLO retrieval
- Custom tenant SLO override
- Edge cases: invalid thresholds

**Unit Tests (test_slo_tracker.cpp):**
- Metrics recording and windowing
- Percentile computation (p50/p95/p99)
- Burn rate calculation
- Multi-window voting logic

**Unit Tests (test_telemetry_gate.cpp):**
- OTLP span emission
- Metric aggregation
- Alert context generation
- Retry and fallback behavior

**Integration Tests (test_observability_e2e.cpp):**
- Full pipeline: metrics → tracking → alerting
- Alert accuracy on synthetic SLO breaches
- Dashboard data consistency
- OTLP export to mock collector

**Performance Benchmarks:**
- Metrics recording: <100µs p99
- Compliance calculation: <10ms p99
- Alert evaluation: <5ms p99

---

## 8. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Telemetry collection only (no alerting)
2. **Week 2:** Alert generation disabled (log only)
3. **Week 3:** Alerts enabled for critical (P1) only
4. **Week 4:** Full alerting (P3/P2/P1)

**Safeguards:**
- Telemetry failures don't impact RAG requests (graceful degradation)
- Alert storm protection: max 1 alert per tenant per 5min
- Manual override: disable alerts per tenant if noisy

**Monitoring:**
- Meta-dashboard: "Is observability working?"
- Telemetry delivery success rate
- Alert volume and type distribution
- OTLP export latency

---

## 9. Future Enhancements

- **Custom SLOs:** Per-intent or per-use-case SLO specialization
- **Predictive Alerting:** Forecast breaches before they happen
- **Anomaly Detection:** ML-based outlier identification
- **Cost Attribution:** Per-feature cost tracking for ROI analysis

---

**End of Phase 8 Specification**
