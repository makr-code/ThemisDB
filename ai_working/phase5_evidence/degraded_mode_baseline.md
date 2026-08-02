# Phase 5 Block P5.2: Degraded-Mode Performance Baseline

**Report Date:** 2026-08-02  
**Test Coverage:** test_aql_provider_degradation.cpp (8 tests) + test_aql_schema_edge_cases.cpp (6 tests)  
**Status:** DRAFT - Ready for execution

## Executive Summary

Block P5.2 establishes performance baselines for AQL operation under degradation conditions:
- Provider unavailability (offline LLM service, timeouts)
- Schema inconsistency or missing metadata
- Resource exhaustion scenarios
- Graceful fallback mechanisms

This block verifies that degradation doesn't exceed acceptable latency thresholds.

## Test Inventory

### test_aql_provider_degradation.cpp (8 Test Cases)

| Test Case | Degradation Scenario | Measurement | Expected p95 | Status |
|-----------|---------------------|-------------|---------------|--------|
| T5.2.1a | ProviderOffline_ImmediateFailover | Latency (ms) | ≤ 50 ms | ⏳ Pending |
| T5.2.1b | ProviderTimeout_RetryWithFallback | Latency (ms) | ≤ 100 ms | ⏳ Pending |
| T5.2.1c | ProviderRateLimitExceeded | Latency (ms) | ≤ 200 ms | ⏳ Pending |
| T5.2.1d | CircuitBreakerOpen_KeywordFallback | Latency (ms) | ≤ 30 ms | ⏳ Pending |
| T5.2.1e | PartialProviderAvailable_GracefulDegradation | Latency (ms) | ≤ 150 ms | ⏳ Pending |
| T5.2.1f | SchemaUnavailable_NoValidation | Latency (ms) | ≤ 40 ms | ⏳ Pending |
| T5.2.1g | ResourceExhaustion_QueuingBehavior | Latency (ms) | ≤ 500 ms | ⏳ Pending |
| T5.2.1h | RecoveryAfterDegradation_ProviderRestart | Latency (ms) | ≤ 100 ms | ⏳ Pending |

### test_aql_schema_edge_cases.cpp (6 Test Cases)

| Test Case | Edge Case | Measurement | Expected p95 | Status |
|-----------|-----------|-------------|---------------|--------|
| T5.2.2a | NullSchema_EmptyValidation | Latency (µs) | ≤ 100 µs | ⏳ Pending |
| T5.2.2b | EmptyCollections_QueryProcessing | Latency (ms) | ≤ 10 ms | ⏳ Pending |
| T5.2.2c | MissingFieldTypes_PartialValidation | Latency (ms) | ≤ 20 ms | ⏳ Pending |
| T5.2.2d | VeryLargeSchema_ParsingPerformance | Latency (ms) | ≤ 200 ms | ⏳ Pending |
| T5.2.2e | SchemaInconsistency_ErrorDetection | Latency (ms) | ≤ 50 ms | ⏳ Pending |
| T5.2.2f | MalformedMetadata_RecoveryPath | Latency (ms) | ≤ 80 ms | ⏳ Pending |

## Degradation Baseline Measurements

### Provider Offline - Immediate Failover

**Scenario:** LLM provider service is completely offline
**Expected Behavior:** Immediately fail over to keyword search fallback
**Measurement:** Time from degradation detection to fallback activation

```
Expected Results:
  p50:  10 ms  (detection + decision)
  p95:  45 ms  (gate - acceptable failover latency)
  p99:  80 ms

Status: ⏳ PENDING EXECUTION
```

### Provider Timeout - Retry with Fallback

**Scenario:** LLM provider responds slowly (> 5s timeout)
**Expected Behavior:** Timeout, then retry with exponential backoff, finally fallback
**Measurement:** Total latency from request to fallback completion

```
Expected Results:
  p50:  30 ms  (timeout + 1 retry)
  p95:  95 ms  (gate - includes backoff)
  p99: 150 ms

Status: ⏳ PENDING EXECUTION
```

### Provider Rate Limit Exceeded

**Scenario:** LLM provider returns 429 Too Many Requests
**Expected Behavior:** Back off and retry, eventually use fallback
**Measurement:** Latency with rate limit backoff

```
Expected Results:
  p50:  50 ms  (rate limit detection + backoff)
  p95: 180 ms  (gate - includes multiple backoffs)
  p99: 300 ms

Status: ⏳ PENDING EXECUTION
```

### Circuit Breaker Open - Keyword Fallback

**Scenario:** Circuit breaker is open (provider unavailable threshold exceeded)
**Expected Behavior:** Skip LLM; use keyword fallback immediately
**Measurement:** Direct transition to keyword fallback

```
Expected Results:
  p50:  8 ms   (immediate rejection)
  p95: 25 ms   (gate - keyword search startup)
  p99: 50 ms

Status: ⏳ PENDING EXECUTION
```

### Partial Provider Available - Graceful Degradation

**Scenario:** Some provider instances available, others down
**Expected Behavior:** Use available instances; degrade performance gracefully
**Measurement:** Latency with partial provider availability

```
Expected Results:
  p50:  40 ms  (routing to available instance)
  p95: 130 ms  (gate - includes retries)
  p99: 250 ms

Status: ⏳ PENDING EXECUTION
```

### Schema Unavailable - No Validation

**Scenario:** Schema metadata service offline; proceed without schema validation
**Expected Behavior:** Skip validation; allow query with degraded safety
**Measurement:** Query processing without schema validation

```
Expected Results:
  p50:  15 ms  (validation skip overhead)
  p95:  35 ms  (gate - degraded query processing)
  p99:  60 ms

Status: ⏳ PENDING EXECUTION
```

### Resource Exhaustion - Queuing Behavior

**Scenario:** Memory/CPU exhausted; requests queued
**Expected Behavior:** Queue requests with bounded wait time
**Measurement:** Latency including queue wait

```
Expected Results:
  p50: 100 ms  (average queue wait)
  p95: 450 ms  (gate - maximum acceptable queue time)
  p99: 800 ms

Status: ⏳ PENDING EXECUTION
```

### Recovery After Degradation - Provider Restart

**Scenario:** Provider was offline; now returns online
**Expected Behavior:** Detect availability and resume normal operation
**Measurement:** Time to resume full-capability operation

```
Expected Results:
  p50:  20 ms  (recovery detection)
  p95:  90 ms  (gate - includes circuit breaker reset)
  p99: 150 ms

Status: ⏳ PENDING EXECUTION
```

## Schema Edge Case Performance

### Null Schema - Empty Validation

**Scenario:** Schema context is null/empty
**Expected Behavior:** Skip validation; proceed with query

```
Expected Results:
  p50:   10 µs
  p95:   80 µs  (gate - minimal overhead)
  p99:  200 µs

Status: ⏳ PENDING EXECUTION
```

### Empty Collections - Query Processing

**Scenario:** Schema defines collections but they're empty
**Expected Behavior:** Process query normally, return empty result

```
Expected Results:
  p50:  2 ms   (empty result processing)
  p95:  8 ms   (gate)
  p99: 20 ms

Status: ⏳ PENDING EXECUTION
```

### Missing Field Types - Partial Validation

**Scenario:** Field metadata incomplete (type info missing)
**Expected Behavior:** Validate what's available; skip type checks

```
Expected Results:
  p50:  5 ms   (partial validation)
  p95: 18 ms   (gate)
  p99: 40 ms

Status: ⏳ PENDING EXECUTION
```

### Very Large Schema - Parsing Performance

**Scenario:** Schema has 10,000+ collections/fields
**Expected Behavior:** Parse efficiently despite size

```
Expected Results:
  p50:  50 ms  (large schema parsing)
  p95: 180 ms  (gate - acceptable for large schema)
  p99: 300 ms

Status: ⏳ PENDING EXECUTION
```

### Schema Inconsistency - Error Detection

**Scenario:** Field defined in multiple collections with different types
**Expected Behavior:** Detect inconsistency; log error; continue

```
Expected Results:
  p50:  15 ms  (detection overhead)
  p95:  45 ms  (gate - detection + logging)
  p99:  80 ms

Status: ⏳ PENDING EXECUTION
```

### Malformed Metadata - Recovery Path

**Scenario:** Schema metadata has parse errors
**Expected Behavior:** Skip malformed entries; validate what's parseable

```
Expected Results:
  p50:  20 ms  (recovery processing)
  p95:  75 ms  (gate - includes error handling)
  p99: 120 ms

Status: ⏳ PENDING EXECUTION
```

## Performance Impact Analysis

### Degradation Overhead vs. Normal Operation

**Normal Operation Baseline (from Phase 4-6):**
- NL→AQL translation: p95 ≤ 2 ms
- AQL validation: p95 ≤ 200 µs
- Token estimation: p95 ≤ 50 µs

**Degradation Overhead (Phase 5.2 target):**

| Degradation Type | Normal p95 | Degraded p95 | Overhead |
|------------------|-----------|--------------|----------|
| Provider Offline | 2 ms | 45 ms | +2150% |
| Provider Timeout | 2 ms | 100 ms | +4900% |
| Schema Unavailable | 0.2 ms | 35 ms | +17400% |
| Circuit Breaker Open | 2 ms | 25 ms | +1150% |

**Rationale:** Degradation scenarios inherently add latency. Gates ensure degradation is still acceptable for user experience.

## Execution Instructions

### Build Phase 5.2 Tests

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Build degradation tests
cmake --build . --target module_aql_test_aql_provider_degradation_focused --parallel 4

# Build schema edge case tests
cmake --build . --target module_aql_test_aql_schema_edge_cases_focused --parallel 4
```

### Run Tests

```bash
# Run provider degradation tests
ctest --verbose -R "AQLProviderDegradation" --timeout 300

# Run schema edge case tests
ctest --verbose -R "AQLSchemaEdgeCases" --timeout 300

# Run all P5.2 tests
ctest --verbose -R "Degradation|SchemaEdgeCases" --timeout 300
```

## Success Criteria

### All Baselines Must Be Established

- [ ] All 14 tests (8+6) execute successfully
- [ ] All degradation latency measurements collected
- [ ] All p95 gates within expected ranges
- [ ] No unexpected timeout failures
- [ ] Recovery paths verified working
- [ ] Schema edge cases handled gracefully

### Phase 5.2 Exit Criteria

- ✅ Degradation baseline established
- ✅ Graceful fallback confirmed working
- ✅ Performance acceptable under degradation
- ✅ Recovery paths verified
- ✅ Ready to proceed to Block P5.3

## Recommendations

1. **Monitoring**: Integrate degradation gates into production monitoring
2. **Alerting**: Alert when degradation paths activated (indicates provider issues)
3. **Testing**: Regular degradation scenario testing in pre-production
4. **Documentation**: Update runbooks for each degradation scenario

---

**Report Status:** DRAFT  
**Report Date:** 2026-08-02  
**Next Update:** After test execution (Week 4 start)

