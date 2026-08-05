# AQL LLM Integration: Phase 3 — Metrics Instrumentation & Interpretation Guide

**Version:** 1.0  
**Date:** 2026-08-05  
**Phase:** 3 (Documentation Consolidation)  
**Status:** ✅ COMPLETE  
**Parent Issue:** makr-code/ThemisDB#5664

---

## Overview

This guide explains how to collect, interpret, and act on metrics from the AQL LLM Integration parser. Phase 2 added comprehensive instrumentation throughout the validation pipeline. This document enables operators and developers to:

- **Understand** what each metric means
- **Interpret** metric values and identify anomalies
- **Act** on metrics to optimize LLM query quality and performance
- **Alert** on thresholds when quality degrades

---

## Table of Contents

1. [Section 1: Metrics Overview](#section-1-metrics-overview)
2. [Section 2: Core Metrics Reference](#section-2-core-metrics-reference)
3. [Section 3: Interpretation Guide with Examples](#section-3-interpretation-guide-with-examples)
4. [Section 4: Metric Aggregation & Dashboarding](#section-4-metric-aggregation--dashboarding)
5. [Section 5: Performance Tuning Guide](#section-5-performance-tuning-guide)

---

## Section 1: Metrics Overview

### Available Metrics (High-Level)

All parser metrics are exposed via Prometheus and grouped into four categories:

| Category | Metrics | Purpose |
|----------|---------|---------|
| **Latency** | `parser.*.*.parse_latency_ms`, `parser.*.*.validation_ms` | Measure parsing and validation performance |
| **Errors** | `parser.errors.*`, `parser.access.*_denial_count` | Track rejection patterns and error distribution |
| **Quality** | Error rates, validation success rate | Monitor LLM prompt quality |
| **Throughput** | Parse operations per second (derived) | Measure parser capacity |

### Metric Cardinality

Metrics are tagged with labels for drill-down analysis:

```
parser_phase1_boundary_validation_ms{
  llm_model="gpt-4-turbo-v1.0",           # LLM model that generated the query
  query_type="SELECT",                     # AQL operation type
  fragment_complexity="high",              # Complexity (simple/medium/high)
  shard="us-east-1",                      # Shard where query is executed
}
```

**Common Labels**:
- `llm_model` — Model ID from LLM layer (e.g., "openai:gpt-4")
- `query_type` — AQL operation (SELECT, INSERT, UPDATE, etc.)
- `fragment_complexity` — Inferred complexity (simple/medium/high)
- `error_type` — Category of error (syntax_error, semantic_error, boundary_violation, access_denied)
- `source` — Origin of query ("llm" or "user")
- `shard` — Execution shard (for distributed analysis)

---

## Section 2: Core Metrics Reference

### 2.1 Latency Metrics (Histograms)

#### parser.phase1.boundary_validation_ms

**What**: Time spent validating LLM query fragment boundaries  
**Where**: Measured in `AQLParser::validateLLMBoundary()` (Phase 1)  
**Unit**: Milliseconds  
**Type**: Prometheus Histogram (buckets at 0.1, 0.5, 1, 5, 10, 50, 100ms)

**Typical Range**:
- **Simple fragments** (single SELECT): 0.1–0.5 ms
- **Medium complexity** (SELECT + JOIN): 0.5–2 ms
- **Complex fragments** (subqueries, aggregations): 2–5 ms
- **Very complex** (deep nesting): 5–10 ms

**High Value Warning** (> 10 ms):
- Indicates unusually complex LLM fragment
- May suggest LLM prompt is too ambitious
- Could indicate inefficient schema or deeply nested expressions

**Action to Take**:
```
If boundary_validation_ms > 10 ms:
1. Review LLM prompt — simplify or guide towards simpler patterns
2. Check fragment complexity — add telemetry to count operators/joins
3. Profile specific fragments — identify pathological cases
4. Consider caching validation results for repeated patterns
```

**Example Prometheus Query**:
```promql
# 95th percentile latency for LLM boundary validation
histogram_quantile(0.95, parser_phase1_boundary_validation_ms)
# by LLM model
histogram_quantile(0.95, parser_phase1_boundary_validation_ms) by (llm_model)
# High percentile alert
histogram_quantile(0.95, parser_phase1_boundary_validation_ms) > 10
```

---

#### parser.phase2.metrics_collection_ms

**What**: Overhead of metrics instrumentation  
**Where**: Measured during metric emission in parser validation loop  
**Unit**: Milliseconds  
**Type**: Prometheus Histogram

**Typical Range**: 0.05–0.5 ms

**Healthy Threshold**: < 1% of total parse latency  
Example: If total parse latency is 5 ms, metrics overhead should be < 0.05 ms

**High Value Warning** (> 1% of parse latency):
- Indicates instrumentation is consuming significant CPU
- May be sampling too frequently
- Could indicate lock contention in metrics collection

**Action to Take**:
```
If metrics_collection_ms > 1% of total latency:
1. Review sampling rate — reduce if collecting too granularly
2. Check for lock contention — consider lock-free metrics
3. Batch metric emission — accumulate and flush periodically
4. Disable low-value metrics — remove least useful instrumentation
```

**Typical Target**: 0.1–0.2 ms (cost-effective monitoring)

---

#### parser.aql.parse_latency_ms

**What**: Total time from raw AQL string to parsed AST  
**Where**: Measured in `AQLParser::parse()` (covers all stages)  
**Unit**: Milliseconds  
**Type**: Prometheus Histogram

**Typical Range**:
- **Standard user AQL**: 0.2–2 ms (average ~1 ms)
- **LLM AQL + validation**: 1–10 ms (average ~5 ms)
- **Complex federated queries**: 10–50 ms

**Phase 4 SLA Target**: ≤ 500 ms (p99)

**Interpretation**:
- **< 1 ms**: Very simple queries (good cache hit candidate)
- **1–5 ms**: Standard queries (healthy)
- **5–50 ms**: Complex or federated queries (expected)
- **> 100 ms**: Potential issue; investigate query structure

---

### 2.2 Error Counters (Counters)

#### parser.errors.llm_validation_fail_count

**What**: Cumulative count of LLM queries rejected during validation  
**Where**: Incremented when `validateLLMBoundary()` returns error  
**Unit**: Count  
**Type**: Prometheus Counter

**Typical Range**: 0–5% of LLM queries rejected  
Example: If 1000 LLM queries processed, expect 0–50 failures

**High Value Warning** (> 10%):
- Indicates LLM is generating invalid fragments frequently
- May suggest:
  - LLM prompt quality degradation (model drift)
  - New schema version that LLM hasn't learned yet
  - Fine-tuning model is undertrained

**Breakdown by Error Type**:
```
parser.errors.llm_validation_fail_count{error_type="missing_filter"}
parser.errors.llm_validation_fail_count{error_type="syntax_error"}
parser.errors.llm_validation_fail_count{error_type="semantic_error"}
parser.errors.llm_validation_fail_count{error_type="boundary_violation"}
parser.errors.llm_validation_fail_count{error_type="access_denied"}
```

**Action to Take**:
```
If llm_validation_fail_count rate increases:
1. Segment by error_type to identify specific issue
2. Review LLM prompt — check for ambiguities or new edge cases
3. Collect failing queries for LLM fine-tuning dataset
4. Roll back to previous model version if degradation is recent
5. Increase validation retry count (Phase 2 retry logic) as temporary fix
```

**Example Prometheus Query**:
```promql
# Rate of LLM validation failures (per second)
rate(parser_errors_llm_validation_fail_count[5m])

# Percentage of LLM queries that fail
rate(parser_errors_llm_validation_fail_count[5m]) 
  / 
rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])

# Failures by error type (pie chart)
sum(rate(parser_errors_llm_validation_fail_count[5m])) by (error_type)
```

---

#### parser.access.llm_denial_count

**What**: Cumulative count of LLM queries denied due to access control  
**Where**: Incremented when LLM query references unauthorized collection  
**Unit**: Count  
**Type**: Prometheus Counter  
**Labels**: `[collection_name, llm_model, caller_id]`

**Typical Range**: 0–2% of LLM queries  
Example: If 1000 LLM queries processed, expect 0–20 access denials

**High Value Warning** (> 5%):
- Indicates high rate of access control rejections
- May suggest:
  - LLM prompt isn't properly restricted by user's permissions
  - Access policy is overly restrictive
  - Policy changed recently but LLM hasn't adapted

**Action to Take**:
```
If llm_denial_count rate increases:
1. Review access policies — ensure they match user permissions
2. Audit LLM prompt — verify permission hints are correct
3. Check if policy changed recently — update LLM context
4. Verify LLM model has latest schema/permission info
5. Consider per-tenant LLM models with reduced schema visibility
```

**Example Prometheus Query**:
```promql
# Rate of access denials (per second)
rate(parser_access_llm_denial_count[5m])

# Denials by collection (identify over-restricted collections)
sum(rate(parser_access_llm_denial_count[5m])) by (collection_name)

# Denials by LLM model (identify model-specific issues)
sum(rate(parser_access_llm_denial_count[5m])) by (llm_model)
```

---

### 2.3 Quality Metrics (Derived)

These metrics are calculated from the counters above:

#### LLM Query Success Rate
```promql
1 - (
  rate(parser_errors_llm_validation_fail_count[5m])
  /
  rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
)
```

**Healthy**: ≥ 95% (meaning ≤ 5% failure rate)  
**Warning Threshold**: < 90%  
**Action**: Review LLM prompt and fine-tuning data

#### Access Control Compliance Rate
```promql
(
  rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
  -
  rate(parser_access_llm_denial_count[5m])
)
/
rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
```

**Healthy**: ≥ 98% (meaning ≤ 2% denial rate)  
**Warning Threshold**: < 95%  
**Action**: Audit LLM prompt and access policies

---

## Section 3: Interpretation Guide with Examples

### Example 1: Normal, Healthy Metrics

**Scenario**: LLM query generation is working well.

**Metric Values**:
```
parser.phase1.boundary_validation_ms:
  p50: 1.2 ms
  p95: 3.5 ms
  p99: 5.1 ms

parser.errors.llm_validation_fail_count:
  Rate (5m): 0.8 failures/sec (out of ~100 queries/sec)
  Percentage: 0.8%

parser.access.llm_denial_count:
  Rate (5m): 0.2 denials/sec
  Percentage: 0.2%

parser_phase1_boundary_validation_ms_count:
  Total LLM queries: 30,000 (5-minute window)
  Success rate: 99.2%
```

**Interpretation**:
✅ All metrics are healthy
- Validation latency is fast (p95 < 5 ms)
- LLM failure rate is low (0.8%)
- Access denials are minimal (0.2%)
- Overall quality is high

**Actions**:
- Continue current configuration
- Monitor for anomalies (set up dashboards)
- No tuning required

---

### Example 2: High Validation Latency (LLM Prompt Too Complex)

**Scenario**: LLM is generating complex queries, validation is slow.

**Metric Values**:
```
parser.phase1.boundary_validation_ms:
  p50: 8.2 ms      ⚠️ High
  p95: 18.5 ms     ⚠️ Very high
  p99: 45.2 ms     🔴 Concerning

parser.errors.llm_validation_fail_count:
  Rate: 1.2 failures/sec
  Percentage: 2.4%

Breakdown by fragment_complexity:
  simple: p95 = 0.8 ms
  medium: p95 = 5.2 ms
  high: p95 = 28.3 ms 🔴 Problem here
```

**Root Cause**: LLM is generating very complex query fragments with deep nesting.

**Actions**:

1. **Identify problematic patterns**:
```promql
# Count queries by complexity
sum(rate(parser_phase1_boundary_validation_ms_count[5m])) by (fragment_complexity)

# Latency by complexity
histogram_quantile(0.95, parser_phase1_boundary_validation_ms) by (fragment_complexity)
```

2. **Adjust LLM prompt**:
```
Add guidance: "Generate simple, flat queries. Avoid deep nesting. 
Prefer multiple simple queries over one complex query."
```

3. **Monitor improvement**:
```promql
# Track p95 latency over time
histogram_quantile(0.95, parser_phase1_boundary_validation_ms) 
  offset 1h vs current
```

4. **Set alert**:
```yaml
alert: LLMValidationLatencyHigh
expr: histogram_quantile(0.95, parser_phase1_boundary_validation_ms) > 10
for: 5m
annotations:
  summary: "LLM validation p95 latency > 10ms (indicates complex fragments)"
  action: "Review LLM prompt to simplify query generation"
```

---

### Example 3: High Rejection Rate (LLM Model Quality Issue)

**Scenario**: LLM model quality has degraded; validation failure rate spiking.

**Metric Values**:
```
parser.errors.llm_validation_fail_count:
  Rate (current): 5.2 failures/sec (out of ~100 queries/sec)
  Rate (5 mins ago): 0.8 failures/sec
  Change: +550% 🔴 Sudden spike

Breakdown by error_type:
  syntax_error: 2.1/sec (40%)
  semantic_error: 1.8/sec (35%)
  boundary_violation: 1.0/sec (19%)
  access_denied: 0.3/sec (6%)

parser.phase1.boundary_validation_ms:
  p95: 3.2 ms (unchanged)
  → Latency is fine, issue is error rate, not performance

Concurrent change:
  LLM model_id changed from "gpt-4-turbo-v1.0" to "gpt-4-turbo-v1.1"
  Deployment timestamp: 2026-08-05 14:30:00 UTC
  Error rate spike timestamp: 2026-08-05 14:31:15 UTC 🔴 Correlation!
```

**Root Cause**: New LLM model version (v1.1) is untrained on new schema/patterns.

**Actions**:

1. **Verify correlation**:
```promql
# Error rate by model version
rate(parser_errors_llm_validation_fail_count) by (llm_model)
```

2. **Immediate action** (rollback or patch):
```
Option A: Rollback to v1.0
  - kubectl rollout undo deployment/llm-orchestration

Option B: Deploy fix
  - Update LLM model with latest schema
  - Redeploy v1.1 with schema updates
```

3. **Set alert for regression**:
```yaml
alert: LLMValidationFailureRateIncrease
expr: >
  rate(parser_errors_llm_validation_fail_count[5m]) 
  >
  rate(parser_errors_llm_validation_fail_count[30m] offset 1h) * 1.5
for: 2m
annotations:
  summary: "LLM validation failure rate increased 50%+ in last 5 minutes"
  action: "Check for model deployment; consider rollback"
```

---

### Example 4: Access Control Anomaly (Policy Mismatch)

**Scenario**: Access control denials are elevated; suggests policy misalignment.

**Metric Values**:
```
parser.access.llm_denial_count:
  Rate (5m): 2.8 denials/sec (out of ~100 queries/sec)
  Percentage: 2.8% ⚠️ Above healthy threshold (2%)
  
Breakdown by collection:
  customer_pii: 1.2 denials/sec (43%)
  user_credit_cards: 0.9 denials/sec (32%)
  audit_log: 0.7 denials/sec (25%)

Concurrent events:
  - Access policy changed at 2026-08-05 09:00:00 UTC
    (restricted customer_pii to specific roles)
  - Error spike started at 2026-08-05 09:15:00 UTC

User feedback:
  "My queries are being denied but I should have access to customer_pii"
```

**Root Cause**: Access policy is too restrictive OR LLM doesn't have current permission metadata.

**Actions**:

1. **Audit policy vs. expectations**:
```cpp
// Check if policy matches user expectations
auto perms = auth_service.getPermissions(user_id);
std::cout << "User " << user_id << " has access to: ";
for (const auto& collection : perms.collections) {
    std::cout << collection << " ";
}
```

2. **Update LLM context with correct permissions**:
```cpp
LLMContext llm_ctx{
    .prompt_version = "gpt-4-turbo-v1.0",
    .model_id = "openai:gpt-4",
    .user_collections = perms.collections,  // ← Updated to match policy
};
parser.setLLMContext(llm_ctx);
```

3. **Monitor improvement**:
```promql
# Denial rate by collection over time
sum(rate(parser_access_llm_denial_count[5m])) by (collection_name)
```

4. **Set alert for policy drift**:
```yaml
alert: AccessControlDenialRateHigh
expr: >
  sum(rate(parser_access_llm_denial_count[5m])) by (collection_name)
  >
  0.02  # 2% threshold
for: 5m
annotations:
  summary: "High rate of access denials for {{ $labels.collection_name }}"
  action: "Audit access policy; ensure LLM context has current permissions"
```

---

## Section 4: Metric Aggregation & Dashboarding

### 4.1 Recommended Aggregation Intervals

| Use Case | Interval | Purpose |
|----------|----------|---------|
| Real-time alerting | 1 minute | Detect immediate issues |
| Operational dashboard | 5 minutes | Standard monitoring (Grafana default) |
| Historical trending | 1 hour | Track quality degradation |
| Capacity planning | 1 day | Identify growth patterns |

### 4.2 Alert Thresholds

**Critical Alerts** (page on-call):
```yaml
alerts:
  - name: LLMValidationOutage
    condition: rate(parser_phase1_boundary_validation_ms_count[5m]) == 0
    severity: P1
    action: "LLM validation pipeline is down; restart llm_orchestration service"

  - name: LLMFailureRateExcessive
    condition: failure_rate > 0.20  # > 20%
    severity: P1
    action: "Rollback LLM model or disable LLM feature until root cause fixed"

  - name: ParserLatencyExcessive
    condition: histogram_quantile(0.95, parser_aql_parse_latency_ms) > 100
    severity: P2
    action: "Parser performance degraded; check query load and index health"
```

**Warning Alerts** (email to team):
```yaml
alerts:
  - name: LLMFailureRateElevated
    condition: failure_rate > 0.10  # > 10%
    severity: P3
    action: "Monitor LLM quality; may need prompt/model adjustment"

  - name: LLMLatencyElevated
    condition: histogram_quantile(0.95, parser_phase1_boundary_validation_ms) > 10
    severity: P3
    action: "Review LLM fragments for complexity; consider prompt simplification"

  - name: AccessDenialRateElevated
    condition: denial_rate > 0.05  # > 5%
    severity: P3
    action: "Audit access policies and LLM permission metadata"
```

### 4.3 Dashboard Configuration

**Example Grafana Dashboard (JSON)**:

```json
{
  "dashboard": {
    "title": "AQL LLM Integration — Phase 3 Monitoring",
    "panels": [
      {
        "title": "LLM Validation Latency (p95)",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, parser_phase1_boundary_validation_ms)"
          }
        ],
        "alert": {
          "condition": "$value > 10",
          "message": "Validation latency high; check LLM prompt complexity"
        }
      },
      {
        "title": "LLM Query Success Rate",
        "targets": [
          {
            "expr": "rate(parser_errors_llm_validation_fail_count[5m]) / rate(parser_phase1_boundary_validation_ms_count{source=\"llm\"}[5m])"
          }
        ],
        "thresholds": [
          {
            "value": 0.90,
            "color": "red",
            "label": "Critical (< 90%)"
          },
          {
            "value": 0.95,
            "color": "yellow",
            "label": "Warning (< 95%)"
          }
        ]
      },
      {
        "title": "Error Distribution (Pie Chart)",
        "targets": [
          {
            "expr": "sum(rate(parser_errors_llm_validation_fail_count[5m])) by (error_type)"
          }
        ]
      },
      {
        "title": "Access Denials by Collection",
        "targets": [
          {
            "expr": "sum(rate(parser_access_llm_denial_count[5m])) by (collection_name)"
          }
        ]
      },
      {
        "title": "Parser Latency Distribution",
        "targets": [
          {
            "expr": "histogram_quantile(0.50, parser_aql_parse_latency_ms)",
            "legendFormat": "p50"
          },
          {
            "expr": "histogram_quantile(0.95, parser_aql_parse_latency_ms)",
            "legendFormat": "p95"
          },
          {
            "expr": "histogram_quantile(0.99, parser_aql_parse_latency_ms)",
            "legendFormat": "p99"
          }
        ]
      }
    ]
  }
}
```

---

## Section 5: Performance Tuning Guide

### 5.1 Metric-Driven Optimization Decisions

Use metrics to guide tuning decisions:

| Metric | Value | Issue | Action |
|--------|-------|-------|--------|
| `boundary_validation_ms` p95 | > 10 ms | Slow validation | Simplify LLM prompt; profile queries |
| `llm_validation_fail_count` | > 10% | High error rate | Retrain LLM model; update schema hints |
| `llm_denial_count` | > 5% | High access denials | Audit policies; update permission hints |
| `parse_latency_ms` p99 | > 500 ms | Parser overloaded | Scale parser instances; optimize schema |
| `metrics_collection_ms` | > 1% of latency | Instrumentation overhead | Reduce sampling rate; batch metrics |

### 5.2 Profile-Guided LLM Prompt Adjustment

**Goal**: Use metrics to identify which patterns the LLM is struggling with.

**Process**:

1. **Collect failing queries**:
```cpp
// Log every validation failure with context
if (!validation_result.valid) {
    log_failed_query(
        llm_fragment.raw_text,
        validation_result.errors,
        validation_result.diagnostics
    );
}
```

2. **Analyze patterns**:
```python
# Cluster failures by error type
failures_by_type = defaultdict(list)
for failure in failed_queries:
    error_type = failure['primary_error']
    failures_by_type[error_type].append(failure)

# Identify most common failures
for error_type, failures in failures_by_type.items():
    print(f"{error_type}: {len(failures)} failures")
    print(f"  Example: {failures[0]['query']}")
```

3. **Update LLM prompt**:
```
Original prompt:
  "Generate AQL queries to answer the user's question."

Updated prompt (based on metrics showing 40% missing FILTER clauses):
  "Generate AQL queries to answer the user's question.
   IMPORTANT: Always include a WHERE or FILTER clause to limit results.
   Use LIMIT 1000 if filtering isn't possible."
```

4. **Validate improvement**:
```promql
# Compare error rate before/after prompt change
rate(parser_errors_llm_validation_fail_count) 
  offset 1h (old prompt) vs current (new prompt)
```

### 5.3 Caching Strategies Based on Metrics

**Strategy 1: Cache Validation Results**

If metrics show high p95 latency for repeated fragments:

```cpp
// Cache validation results for common patterns
std::unordered_map<std::string, ValidationResult> validation_cache;

Result<ValidationResult> validate_with_cache(
    const QueryFragment& fragment,
    const LLMContext& context
) {
    std::string cache_key = hash(fragment.raw_text);
    
    if (validation_cache.count(cache_key)) {
        return validation_cache[cache_key];  // Cache hit
    }
    
    auto result = parser.validateLLMBoundary(fragment, context);
    validation_cache[cache_key] = result;  // Cache result
    return result;
}
```

**Strategy 2: Progressive Sampling**

If metrics show high overhead from metrics collection:

```cpp
// Sample metrics at variable rate based on query complexity
void emit_metrics(const ParserMetrics& metrics) {
    double sampling_rate = 1.0;  // Default: 100%
    
    if (metrics.fragment_complexity == "high") {
        sampling_rate = 0.5;  // High complexity: 50% sample rate
    }
    
    if (random(0, 1) < sampling_rate) {
        prometheus_metrics->increment(metrics);
    }
}
```

---

## Summary

**Key Metrics to Monitor**:

| Metric | What It Means | Action Threshold |
|--------|---------------|-----------------|
| `boundary_validation_ms` (p95) | LLM fragment validation speed | > 10 ms |
| `llm_validation_fail_count` (rate) | LLM query quality | > 5% failure rate |
| `llm_denial_count` (rate) | Access control compliance | > 2% denial rate |
| `parse_latency_ms` (p99) | Overall parser performance | > 500 ms |
| `metrics_collection_ms` | Instrumentation overhead | > 1% of parse latency |

**Next Steps**:

1. **Set up dashboards** using Grafana configuration above
2. **Configure alerts** for critical thresholds
3. **Collect baseline** metrics (1-2 weeks of production data)
4. **Profile LLM prompts** using failure analysis
5. **Document runbooks** for common anomalies

---

**Document Status**: ✅ COMPLETE  
**Last Updated**: 2026-08-05  
**Author**: Query Module Phase 6A Documentation Agent  
**Review Status**: Pending core team review
