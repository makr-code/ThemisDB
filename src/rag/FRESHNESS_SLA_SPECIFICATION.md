# Data Freshness SLA & Index Refresh Specification
**Phase 7 of RAG Readiness Audit Implementation**

**Document Version:** 1.0  
**Status:** Design Ready  
**Target Implementation:** Q1 2027  
**Author:** ThemisDB Contributors  
**Date:** 2026-09-24

---

## 1. Overview

Data freshness SLA ensures retrieval results reflect recent documents, preventing stale information from impacting user trust and application correctness. This phase implements latency monitoring, staleness-aware routing, and automated refresh scheduling to maintain <5min p95 staleness.

**Key Objectives:**
- Guarantee p95 index staleness < 5 minutes
- 99%+ SLA compliance over monthly windows
- Transparent staleness reporting to clients
- Automatic remediation on SLA breach

---

## 2. Architecture

### 2.1 Component Stack

```
Document Ingestion Pipeline
    ↓
IngestionLatencyMonitor
    ├→ Track: document arrival time vs index update time
    ├→ Aggregate: p50, p95, p99 staleness metrics
    └→ Emit: SLO compliance signals
    ↓
IndexRefreshScheduler
    ├→ Trigger: partial or full index refreshes
    ├→ Optimize: refresh frequency based on ingestion rate
    └→ Coordinate: with sharding strategy
    ↓
Query-Time Staleness Check
    ↓
StalenessAwareRouter
    ├→ Query current staleness from monitor
    ├→ Route to: fresh shard or fallback replica
    └→ Emit: staleness metadata in response
    ↓
Client Response
    ├→ Include: staleness_ms in metadata
    └→ Example: "Results accurate as of 2s ago"
    ↓
FreshnessSLAEnforcer
    ├→ Track: compliance window (monthly/weekly)
    ├→ Alert: if approaching SLA breach
    └→ Trigger: emergency refresh if needed
```

### 2.2 Data Flow

**Ingestion-Time:**
1. Document arrives with timestamp t_ingest
2. Document inserted into source DB
3. Change log captures t_ingest
4. Async ingestion pipeline picks up changes
5. Index updated with document at time t_index
6. IngestionLatencyMonitor records latency = t_index - t_ingest

**Query-Time:**
1. Query arrives at time t_query
2. StalenessAwareRouter checks: staleness = t_query - t_last_refresh
3. If staleness < SLA threshold (5min): route to standard index
4. Else: route to fallback replica or trigger urgent refresh
5. Include staleness_ms in response metadata

---

## 3. Component Specifications

### 3.1 IngestionLatencyMonitor

**Purpose:** Track document ingestion latency and aggregate staleness metrics.

**API:**
```cpp
class IngestionLatencyMonitor {
  struct IngestionRecord {
    std::string document_id;
    int64_t arrival_time_us;      // microseconds since epoch
    int64_t index_update_time_us;  // when actually indexed
    uint32_t document_size_bytes;
  };
  
  // Record a document ingestion
  void RecordIngestion(const IngestionRecord& record);
  
  // Get staleness percentiles (ms)
  struct StalenessMetrics {
    double p50_staleness_ms;  // median
    double p95_staleness_ms;  // 95th percentile
    double p99_staleness_ms;  // 99th percentile
    double max_staleness_ms;  // worst case
    uint64_t ingestion_count;  // documents ingested in window
    std::string window_start;   // ISO 8601 timestamp
    std::string window_end;
  };
  StalenessMetrics GetStalenessMetrics(
    const std::chrono::duration<int, std::milli>& window = 1h
  );
  
  // Check SLA compliance
  struct SLOStatus {
    bool is_compliant;         // p95 < 5min?
    double p95_staleness_ms;
    double slo_threshold_ms;   // 5min = 300000ms
    std::string status_message;
  };
  SLOStatus GetSLOStatus();
  
  // Stream staleness metrics to telemetry
  void EmitMetricsToTelemetry();
};
```

**Percentile Aggregation:**
- Time window: hourly buckets (e.g., [10:00-11:00 UTC])
- Collection method: streaming histogram (T-Digest or similar)
- Precision: ±2% rank error
- Retention: 30 days of hourly histograms

**SLO Threshold:**
- p95_staleness < 5 min (300,000 ms)
- Measured over rolling 30-day window
- Requires 99%+ of hours to be in-SLA

### 3.2 StalenessAwareRouter

**Purpose:** Route queries to appropriate index shard based on staleness.

**API:**
```cpp
class StalenessAwareRouter {
  struct IndexShardStatus {
    std::string shard_id;
    int64_t last_refresh_time_us;
    uint64_t document_count;
    float freshness_score;  // 1.0 = fresh, 0.0 = stale
  };
  
  struct RoutingContext {
    std::string query_id;
    std::string query_text;
    int64_t query_time_us;       // current time
    float maximum_staleness_ms;  // client SLA requirement
  };
  
  // Get staleness of index shard
  int64_t GetStalenessMs(const std::string& shard_id);
  
  // Route query to appropriate shard
  struct RoutingDecision {
    std::string target_shard;
    int64_t staleness_ms;
    bool is_within_sla;
    std::string fallback_shard;  // if primary is stale
    std::string reasoning;
  };
  RoutingDecision Route(const RoutingContext& context);
  
  // Emit staleness metadata in response
  struct ResponseMetadata {
    int64_t query_time_us;
    int64_t index_refresh_time_us;
    int64_t staleness_ms;
    std::string staleness_category;  // "fresh" | "acceptable" | "stale"
  };
  ResponseMetadata ComputeMetadata(const RoutingDecision& decision);
};
```

**Routing Logic:**

```
staleness = current_time - last_refresh_time

IF staleness < 5 min AND staleness < client_required_freshness:
  route_to = primary_shard
  status = "FRESH"
ELSE IF staleness < 15 min AND replica_available:
  route_to = replica_shard (may trigger async refresh)
  status = "ACCEPTABLE"
ELSE IF staleness < 30 min AND fallback_cache_available:
  route_to = fallback_cache
  status = "STALE" (with warning)
ELSE:
  trigger_emergency_refresh()
  route_to = primary_shard (wait for refresh)
  status = "CRITICAL"
```

**Staleness Categories:**
- **FRESH:** < 5 min staleness (meets SLA)
- **ACCEPTABLE:** 5-15 min staleness (caution, use replica if available)
- **STALE:** 15-30 min staleness (warning to client, consider fallback)
- **CRITICAL:** > 30 min staleness (emergency action required)

### 3.3 IndexRefreshScheduler

**Purpose:** Trigger and coordinate index refreshes based on ingestion rates.

**API:**
```cpp
class IndexRefreshScheduler {
  struct RefreshPolicy {
    uint32_t min_refresh_interval_sec;  // don't refresh faster than this
    uint32_t max_staleness_sec;         // don't allow staleness > this
    bool incremental_only;              // prefer incremental vs full refresh
    uint32_t batch_size;                // documents per batch
  };
  
  // Set refresh policy
  void SetRefreshPolicy(const RefreshPolicy& policy);
  
  // Schedule a refresh (async)
  struct RefreshRequest {
    std::string shard_id;
    bool incremental;  // true = delta, false = full rebuild
    uint32_t max_batch_docs;
  };
  bool ScheduleRefresh(const RefreshRequest& request);
  
  // Get refresh schedule status
  struct ScheduleStatus {
    int64_t next_refresh_time_us;
    int64_t last_refresh_time_us;
    uint32_t pending_documents;
    std::string refresh_type;  // "incremental" | "full"
  };
  ScheduleStatus GetScheduleStatus(const std::string& shard_id);
  
  // Cancel scheduled refresh
  bool CancelRefresh(const std::string& shard_id);
};
```

**Refresh Triggers:**

| Condition | Action | Rationale |
|-----------|--------|-----------|
| Ingestion rate > 1000 docs/sec | Refresh every 30s (incremental) | Maintain freshness at high volume |
| Ingestion rate 100-1000 docs/sec | Refresh every 60s (incremental) | Balanced cadence |
| Ingestion rate < 100 docs/sec | Refresh every 300s or on batch size (incremental) | Efficient resource use |
| SLA breach detected | Refresh immediately (full) | Emergency recovery |

**Refresh Coordination:**
- Don't start new refresh until previous completes
- Queue up to 3 pending refreshes
- Skip refresh if staleness < 30s (too frequent)

### 3.4 FreshnessSLAEnforcer

**Purpose:** Ensure 99%+ SLA compliance over monthly window.

**API:**
```cpp
class FreshnessSLAEnforcer {
  struct SLAWindow {
    std::string window_id;       // "2026-09"
    int64_t start_time_us;
    int64_t end_time_us;
    uint32_t target_sla_percent; // 99
  };
  
  // Check SLA compliance for current window
  struct ComplianceReport {
    bool is_compliant;
    double compliance_percent;   // % of hours in-SLA
    uint32_t compliant_hours;
    uint32_t noncompliant_hours;
    int64_t hours_until_breach;  // hours until SLA violated (if declining)
  };
  ComplianceReport GetComplianceStatus();
  
  // Forecast SLA status if trends continue
  struct Forecast {
    bool will_breach;
    int64_t estimated_breach_time_us;
    std::string recommendation;  // "increase refresh rate" etc.
  };
  Forecast ForecastComplianceStatus();
  
  // Emergency actions
  bool TriggerEmergencyRefresh();
  bool TemporarilyReduceMaxStaleness(uint32_t temporary_threshold_ms);
};
```

**Compliance Calculation:**
```
compliant_hours = count(hourly_metrics where p95_staleness < 5min)
compliance_percent = compliant_hours / total_hours_in_window * 100

is_compliant = (compliance_percent >= 99.0)
```

**Forecasting Logic:**
```
trend = linear_regression(last_7_days_compliance)
hours_until_breach = (99.0 - current_compliance) / trend

if hours_until_breach < 72:
  recommendation = "INCREASE_REFRESH_RATE"
```

**Emergency Escalation:**
- Auto-alert on SLA breach risk (48 hours remaining)
- Manual override available: TriggerEmergencyRefresh()
- Temporary mitigation: reduce SLA threshold to 10min (temporary_threshold_ms)

---

## 4. Staleness Metrics & Monitoring

### 4.1 Key Metrics

| Metric | Unit | SLO |
|--------|------|-----|
| p95 ingestion latency | milliseconds | < 300 ms (5min) |
| p99 ingestion latency | milliseconds | < 600 ms (10min) |
| SLA compliance | percent | ≥ 99% |
| Index refresh latency | seconds | < 10s (incremental) < 60s (full) |
| Document ingestion throughput | docs/sec | ≥ 100 docs/sec |

### 4.2 Dashboard Components

- **Freshness Trend:** p50, p95, p99 staleness over time
- **SLA Compliance:** daily compliance % with trend
- **Refresh Schedule:** upcoming refresh times and types
- **Alert History:** SLA breach alerts and resolutions

---

## 5. Acceptance Criteria

| Criterion | Threshold | Rationale |
|-----------|-----------|-----------|
| p95 staleness | < 5 min | Core SLO |
| SLA compliance | ≥ 99% over 30 days | Business commitment |
| Refresh latency (incremental) | < 10s | Operational overhead |
| Refresh latency (full) | < 60s | Operational overhead |
| Fallback availability | ≥ 99.9% | No single point of failure |
| Alert accuracy | ≥ 95% (true positives) | Avoid alert fatigue |

---

## 6. Configuration & Controls

**Environment Variables:**
```bash
THEMIS_FRESHNESS_SLA_ENABLED=true|false          # Feature gate
THEMIS_MAX_STALENESS_MS=300000                   # 5 min SLA
THEMIS_REFRESH_INTERVAL_SEC=60                   # default refresh cadence
THEMIS_EMERGENCY_REFRESH_THRESHOLD_MS=600000     # 10 min, trigger emergency
THEMIS_SLA_TARGET_PERCENT=99                     # 99% compliance target
```

**CMake Feature Gate:**
```cmake
option(THEMIS_ENABLE_FRESHNESS_SLA "Enable Phase 7 freshness SLA" ON)
```

---

## 7. Testing Strategy

**Unit Tests (test_ingestion_latency_monitor.cpp):**
- Staleness metric aggregation and percentile computation
- SLO status calculation (compliant vs non-compliant)
- Metric retention and windowing
- Edge cases: zero ingestions, clock skew

**Unit Tests (test_staleness_aware_router.cpp):**
- Staleness computation for shard
- Routing decision logic (fresh vs acceptable vs stale)
- Metadata generation and serialization
- Fallback shard selection

**Unit Tests (test_index_refresh_scheduler.cpp):**
- Refresh scheduling based on ingestion rate
- Policy application (min/max intervals)
- Schedule state transitions
- Queue management (pending refreshes)

**Unit Tests (test_freshness_sla_enforcer.cpp):**
- SLA compliance calculation
- Trend forecasting and breach prediction
- Emergency actions (force refresh, reduce threshold)
- Alert generation

**Integration Tests (test_freshness_e2e.cpp):**
- End-to-end freshness workflow: ingest → monitor → route → response
- Multi-shard staleness aggregation
- SLA compliance over 30-day simulated window
- Emergency recovery scenarios

**Performance Benchmarks:**
- Staleness computation: <1ms p99
- Routing decision: <5ms p99
- Refresh scheduling overhead: <10ms per operation

---

## 8. Rollout & Risk Mitigation

**Phased Rollout:**
1. **Week 1:** Monitoring only (no enforcement)
2. **Week 2:** Soft routing (prefer fresh, but no errors)
3. **Week 3:** Hard routing (enforce SLA with fallback)
4. **Week 4:** Full enforcement + SLA penalties

**Safeguards:**
- Audit mode: log all staleness breaches without failing queries
- Gradual SLA tightening: week 1 (10min) → week 2 (7.5min) → week 3+ (5min)
- Auto-disable if refresh latency > SLA window

**Monitoring:**
- Dashboard: staleness trend, SLA compliance, refresh success rate
- Alerts: SLA breach risk >48 hours, refresh latency >SLA
- SLA: 99%+ compliant over rolling 30-day window

---

## 9. Future Enhancements

- **Adaptive Refresh:** ML-based refresh frequency optimization
- **Sharding Strategy:** Smart shard allocation for balanced freshness
- **Multi-Region:** Geo-distributed index replicas with consistency
- **Weighted Freshness:** Different SLAs per document class/importance

---

**End of Phase 7 Specification**
