# Phase 4 Detailed Implementation Plan — Observability & Operations Hardening

**Duration:** 6-8 weeks (parallel with Phase 2/3/5, starts after Phase 1A)  
**Owner:** Operations & SRE Team (Team D)  
**Status:** 🟤 PENDING — Starts after Phase 1A (2026-08-12)  
**Target Completion:** 2026-09-30

---

## Objective

Complete observability, auditability, and SLO-signal coverage for production operations to achieve 99.99% SLA validation and comprehensive operational readiness.

---

## Scope

### In Scope

1. **Error Classification & Context Propagation**
   - Error code taxonomy expansion with operational semantics
   - Retry-safe vs. transient vs. permanent error categorization
   - Error context chain for debugging and observability

2. **Audit Logging & Compliance Instrumentation**
   - Security-sensitive operations logging (auth, role changes, data access)
   - Compliance audit trail (GDPR, HIPAA, SOC2 requirements)
   - Structured logging with correlation IDs

3. **SLO Signal Extraction**
   - Latency percentile export (p50, p90, p99, p99.9)
   - Error rate monitoring (count, rate, by category)
   - Availability percentiles (uptime %, request success %)
   - Resource utilization signals (CPU, memory, disk, I/O)

4. **Runbook Suite**
   - Installation & configuration
   - Failover & recovery procedures
   - Upgrade & rollback procedures
   - Troubleshooting guides
   - Performance tuning guides

5. **99.99% SLA Validation**
   - Wave 9b test (`w9b_sla_measurement_compliance`) confirms 5 nines uptime under chaos
   - Sustained load (Wave 7) + fault injection (Wave 8) combined

### Out of Scope (Phase 5+)

- Advanced metrics (trace sampling, flame graphs, custom business metrics)
- Cost optimization (optimization recommendations based on usage)
- Long-term monitoring strategy (beyond 6-month retention)
- Machine learning-based anomaly detection

---

## Work Breakdown

### Milestone 1: Error Classification & Taxonomy (Weeks 1-2)

**Owner:** Core Infrastructure Team

**Tasks:**

1. **Define Error Code Taxonomy**
   - [ ] Create document: `docs/architecture/ERROR_TAXONOMY.md`
   - [ ] Define error categories:
     - `PERMANENT_ERROR` (0x1000) — unrecoverable, do not retry
       - Examples: invalid argument, resource not found, permission denied
       - Suitable for circuit breaker (fail immediately)
     - `TRANSIENT_ERROR` (0x2000) — recoverable with backoff, safe to retry
       - Examples: timeout, throttle, temporary service unavailable
       - Suitable for exponential backoff + retry
     - `DEGRADED_ERROR` (0x3000) — partial success, degrade gracefully
       - Examples: single replica unhealthy, cache miss, fallback used
       - Suitable for fallback + monitoring
     - `INTERNAL_ERROR` (0x4000) — unexpected, needs investigation
       - Examples: panic, null pointer, out of memory, data corruption
       - Suitable for alert + telemetry

   - [ ] For each error category, define:
     - Category code (e.g., 0x1000)
     - Retry policy (NO_RETRY | EXPONENTIAL_BACKOFF | IMMEDIATE_RETRY)
     - Circuit breaker behavior (FAIL_FAST | DEGRADE | QUEUE)
     - Alert severity (INFO | WARNING | ALERT | CRITICAL)
     - Log level (DEBUG | INFO | WARNING | ERROR | CRITICAL)
     - Telemetry key (for metrics export)

   - [ ] Example error table:
     ```markdown
     | Code | Message | Category | Retry | CB | Alert | Example |
     |------|---------|----------|-------|----|----|---------|
     | E0001 | Invalid argument | PERMANENT | NO | FAIL_FAST | INFO | Invalid JSON input |
     | E0002 | Resource not found | PERMANENT | NO | FAIL_FAST | INFO | Query plan not in cache |
     | E0003 | Permission denied | PERMANENT | NO | FAIL_FAST | ALERT | Unauthorized role change |
     | E1001 | Timeout | TRANSIENT | EXP_BO | QUEUE | WARNING | Query timeout after 30s |
     | E1002 | Throttled | TRANSIENT | EXP_BO | QUEUE | INFO | Rate limit exceeded |
     | E1003 | Service unavailable | TRANSIENT | EXP_BO | QUEUE | WARNING | Replica offline |
     | E2001 | Cache miss | DEGRADED | N/A | N/A | DEBUG | Query plan not cached |
     | E2002 | Partial index | DEGRADED | N/A | N/A | WARNING | Index incomplete |
     | E4001 | Out of memory | INTERNAL | NO | FAIL_FAST | CRITICAL | malloc failed |
     | E4002 | Data corruption | INTERNAL | NO | FAIL_FAST | CRITICAL | Checksum mismatch |
     ```

2. **Update Status/Error Classes**
   - [ ] Update `include/common/status.h`:
     - Add error category field: `category_t category()`
     - Add retry policy field: `retry_policy_t retry_policy()`
     - Add alert severity field: `severity_t alert_severity()`
   - [ ] Update all existing error codes to new taxonomy
   - [ ] Create error builders: `Status::Permanent(code)`, `Status::Transient(code)`, `Status::Degraded(code)`, `Status::Internal(code)`

   **Tests:**
   - [ ] `test_error_taxonomy.cpp` (ERR-01..ERR-08)
     - ERR-01: Permanent error → no retry, fail fast
     - ERR-02: Transient error → exponential backoff
     - ERR-03: Degraded error → fallback
     - ERR-04: Internal error → alert + log critical
     - ERR-05: Error context chain preserved through API boundary
     - ERR-06: Error codes mapped to telemetry keys
     - ERR-07: Alert severity routing to correct level
     - ERR-08: Backward compatibility: old error codes still work

3. **Error Propagation Chain**
   - [ ] Implement error context tracking: `ErrorContext` struct
     - `root_cause` (original error)
     - `call_stack` (list of function names)
     - `correlation_id` (unique ID for tracing)
     - `timestamp` (when error occurred)
     - `metadata` (key-value pairs for context)
   - [ ] Update Status to carry ErrorContext
   - [ ] Provide helpers: `Status::WithContext()`, `Status::WithCorrelationId()`

   **Definition of Done:**
   - ✅ Error taxonomy document published
   - ✅ Error codes updated to new taxonomy
   - ✅ ErrorContext implementation complete
   - ✅ ERR-01..ERR-08 tests passing

---

### Milestone 2: Audit Logging Instrumentation (Weeks 2-4)

**Owner:** Security & Audit Team

**Tasks:**

1. **Define Audit Logging Schema**
   - [ ] Create document: `docs/security/AUDIT_LOGGING_SCHEMA.md`
   - [ ] Define audit event structure:
     ```json
     {
       "timestamp": "2026-08-15T10:30:45.123Z",
       "event_type": "auth.role_assignment",
       "correlation_id": "req-abc123",
       "user": "alice@example.com",
       "actor_ip": "192.0.2.1",
       "action": "grant_role",
       "resource": "database:finance_db",
       "old_value": null,
       "new_value": "role:analyst",
       "status": "success",
       "reason": null,
       "duration_ms": 42
     }
     ```

   - [ ] Define audit event types (initial set):
     - `auth.login` / `auth.logout`
     - `auth.role_assignment` / `auth.role_revocation`
     - `auth.permission_grant` / `auth.permission_revoke`
     - `data.access` (select, insert, update, delete)
     - `data.export`
     - `config.change`
     - `security.policy_change`
     - `failover.state_change`
     - `plugin.load` / `plugin.unload`

2. **Implement Audit Logger**
   - [ ] Create: `src/audit/audit_logger.cpp` + `include/audit/audit_logger.h`
   - [ ] Audit logger features:
     - Structured logging (JSON output)
     - Async queue (non-blocking)
     - Rate limiting (prevent audit log flooding)
     - Rotation (hourly/daily)
     - Retention policy (30-day default, configurable)
   - [ ] Integration points:
     - Auth module: log role changes, permission changes
     - Query engine: log data access (on demand, configurable)
     - Config: log all configuration changes
     - Failover: log state transitions
     - Plugin: log load/unload operations
   - [ ] Thread-safe: use lock-free queue for performance

   **Tests:**
   - [ ] `test_audit_logger.cpp` (AUD-01..AUD-08)
     - AUD-01: Audit event serialized to JSON correctly
     - AUD-02: Correlation ID propagated through event
     - AUD-03: Async queue non-blocking (latency < 1 ms)
     - AUD-04: Rate limiting active (max 1k events/s)
     - AUD-05: Log rotation on schedule
     - AUD-06: Retention policy enforced
     - AUD-07: Log file readable by compliance tools
     - AUD-08: No sensitive data (PII) in audit logs

3. **Auth Module Integration**
   - [ ] Update `src/auth/auth_manager.cpp`:
     - Log `auth.login` on successful login
     - Log `auth.role_assignment` on role grant
     - Log `auth.role_revocation` on role revoke
     - Log `auth.permission_grant` / `auth.permission_revoke`
   - [ ] Include: user, actor_ip, action, resource, status

4. **Query Engine Integration (Configurable)**
   - [ ] Add CMake option: `THEMIS_AUDIT_LOG_DATA_ACCESS` (default OFF)
   - [ ] When enabled, update query engine:
     - Log `data.access` event for each SELECT/INSERT/UPDATE/DELETE
     - Include: user, resource (table/collection), action, row count
   - [ ] Verify: low overhead when disabled; < 5% latency impact when enabled

   **Definition of Done:**
   - ✅ Audit logging schema defined
   - ✅ Audit logger implemented (async, rate-limited, rotated)
   - ✅ Auth module integration complete
   - ✅ Query engine integration complete (configurable)
   - ✅ AUD-01..AUD-08 tests passing

---

### Milestone 3: SLO Signal Extraction & Prometheus Export (Weeks 4-6)

**Owner:** Observability & SRE Team

**Tasks:**

1. **Define SLO Signals**
   - [ ] Create document: `docs/operations/SLO_SIGNALS.md`
   - [ ] Define signals per category:
     - **Latency signals** (milliseconds):
       - Query latency: p50, p90, p99, p99.9
       - Write latency: p50, p90, p99, p99.9
       - Replication latency: p50, p90, p99, p99.9
     - **Error rate signals** (per minute):
       - Error count (by category: permanent, transient, degraded, internal)
       - Error rate (errors / total requests)
     - **Availability signals** (per minute):
       - Request success rate (%)
       - Connection availability (%)
       - Replica health (% healthy replicas)
     - **Resource utilization** (per minute):
       - CPU usage (%)
       - Memory usage (%)
       - Disk I/O (MB/s)
       - Network I/O (Mbps)
       - Connection pool utilization (%)

2. **Implement Metrics Collection**
   - [ ] Create: `src/metrics/metrics_collector.cpp` + `include/metrics/metrics_collector.h`
   - [ ] Metrics types:
     - Counter: total count (errors, requests)
     - Gauge: instantaneous value (memory, CPU)
     - Histogram: distribution (latency)
     - Summary: quantiles (p50, p99)
   - [ ] Use Prometheus C++ client library (or custom implementation)
   - [ ] Thread-safe metric updates

   **Tests:**
   - [ ] `test_metrics_collection.cpp` (MET-01..MET-08)
     - MET-01: Counter increments correctly
     - MET-02: Gauge reflects current value
     - MET-03: Histogram records value correctly
     - MET-04: Quantiles computed correctly (p50, p99)
     - MET-05: Metrics thread-safe under concurrent updates
     - MET-06: Metric export non-blocking (< 1 ms)
     - MET-07: Cardinality limits enforced (prevent cardinality explosion)
     - MET-08: Metrics reset on explicit reset call

3. **Prometheus Export Endpoint**
   - [ ] Create: `src/metrics/prometheus_exporter.cpp`
   - [ ] HTTP endpoint: `GET /metrics` (port configurable)
   - [ ] Response format: Prometheus text format (`.004` version)
   - [ ] Include:
     - All signal metrics
     - Histogram buckets (10ms, 50ms, 100ms, 500ms, 1000ms, 5000ms)
     - Help text for each metric

   - [ ] Example output:
     ```
     # HELP query_latency_ms Query execution latency
     # TYPE query_latency_ms histogram
     query_latency_ms_bucket{le="10"} 100
     query_latency_ms_bucket{le="50"} 250
     query_latency_ms_bucket{le="100"} 400
     query_latency_ms_bucket{le="500"} 450
     query_latency_ms_bucket{le="1000"} 480
     query_latency_ms_bucket{le="+Inf"} 500
     query_latency_ms_sum 45000
     query_latency_ms_count 500

     # HELP errors_total Total errors by category
     # TYPE errors_total counter
     errors_total{category="permanent"} 10
     errors_total{category="transient"} 50
     errors_total{category="degraded"} 30
     errors_total{category="internal"} 2
     ```

   **Tests:**
   - [ ] `test_prometheus_export.cpp` (PROM-01..PROM-04)
     - PROM-01: HTTP endpoint responds with metrics
     - PROM-01: Metrics format valid Prometheus text format
     - PROM-03: Histogram buckets present and ordered
     - PROM-04: Export endpoint non-blocking (< 1 ms)

4. **SLO Signal Integration into Release-Critical Paths**
   - [ ] For each release-critical operation, instrument SLO signals:
     - Query execution → latency histogram + error counter
     - Write operation → latency histogram + error counter
     - Replication → latency histogram + replica health gauge
     - Failover → latency histogram + error counter
   - [ ] Verify: zero-overhead when metrics disabled; < 2% overhead when enabled

   **Definition of Done:**
   - ✅ SLO signals defined and documented
   - ✅ Metrics collection implemented
   - ✅ Prometheus export endpoint working
   - ✅ All release-critical paths instrumented
   - ✅ MET-01..MET-08 + PROM-01..PROM-04 tests passing

---

### Milestone 4: Runbook Suite & Operations Procedures (Weeks 6-8)

**Owner:** Operations & Documentation Team

**Tasks:**

1. **Installation & Configuration Runbook**
   - [ ] Create: `docs/operations/RUNBOOK_INSTALL.md`
   - [ ] Sections:
     - Prerequisites (OS, memory, disk, network)
     - Download & verify signature
     - Installation steps (binary, container, source)
     - Post-install verification
     - Initial configuration (ports, TLS, auth)
     - Health check: `curl localhost:8080/health`
     - Logging setup (audit log, application log)
     - Monitoring setup (Prometheus scrape target, alerting rules)

2. **Failover & Recovery Runbook**
   - [ ] Create: `docs/operations/RUNBOOK_FAILOVER_RECOVERY.md`
   - [ ] Sections:
     - Failure detection (symptoms, logs to check)
     - Manual failover procedure (disable primary, promote replica)
     - Automatic failover monitoring (check cluster status)
     - Recovery procedure (rebuild replica, resync)
     - Data consistency check (verify checksums)
     - Switchback to primary (planned recovery)
     - Monitoring failover metrics (latency, error rate)

3. **Upgrade & Rollback Runbook**
   - [ ] Create: `docs/operations/RUNBOOK_UPGRADE_ROLLBACK.md`
   - [ ] Sections:
     - Pre-upgrade checks (backup, capacity, compatibility)
     - Rolling upgrade procedure (one node at a time)
     - Compatibility check (version mismatch detection)
     - Smoke tests post-upgrade
     - Rollback procedure (if upgrade fails)
     - Data migration (if schema changes)
     - Monitoring upgrade progress (logs, metrics)

4. **Troubleshooting Guide**
   - [ ] Create: `docs/operations/TROUBLESHOOTING_GUIDE.md`
   - [ ] Common issues:
     - High query latency: check metrics, cache hit rate, resource utilization
     - High error rate: check error logs, error category distribution
     - Replica lag: check replication latency metric, network bottlenecks
     - OOM (out of memory): check memory usage gauge, connection pool size
     - Authentication failures: check audit logs, role assignments
     - Plugin not loading: check manifest, edition gating, error codes
   - [ ] For each issue: symptoms, diagnosis steps, resolution steps

5. **Performance Tuning Guide**
   - [ ] Create: `docs/operations/PERFORMANCE_TUNING.md`
   - [ ] Tuning parameters:
     - Query cache size (tuned by plan-cache hit rate)
     - Connection pool size (tuned by connection pool utilization)
     - Thread pool size (tuned by CPU utilization + p99 latency)
     - Replication batch size (tuned by replication latency)
     - Audit logging rate limit (tuned by audit log latency impact)
   - [ ] Monitoring metrics to check before tuning:
     - p99 query latency
     - Cache hit rate
     - Resource utilization (CPU, memory, I/O)

   **Definition of Done:**
   - ✅ All 5 runbooks published
   - ✅ Runbooks validated by operations team (sign-off)
   - ✅ Runbooks tested in lab environment (dry run)
   - ✅ Runbooks linked from main documentation

---

### Milestone 5: 99.99% SLA Validation & Chaos Testing (Weeks 8+)

**Owner:** QA & Performance Team

**Tasks:**

1. **Wave 9b SLA Measurement Test**
   - [ ] Create test: `tests/integration/pipeline/w9b_sla_measurement_compliance_test.cpp`
   - [ ] Test scenario:
     - Sustained load: 1000 req/s for 72 hours (Wave 7 baseline)
     - Fault injection: random component failures (every 30 min)
       - Network partition (1 min)
       - Replica crash (2 min recovery)
       - Query timeout (5% of requests)
       - Audit log overflow (1 min)
     - Measurement window: 72 hours
     - SLA target: 99.99% uptime (< 52 minutes downtime)

   - [ ] SLA calculation:
     - Availability: (total_requests - failed_requests) / total_requests
     - Target: ≥ 99.99%
     - Acceptable downtime: < 52 minutes in 72-hour window

   **Tests:**
   - [ ] `test_sla_measurement_compliance.cpp` (SLA-01..SLA-04)
     - SLA-01: 99.99% uptime achieved with no faults
     - SLA-02: 99.99% uptime maintained with network partition (1 min)
     - SLA-03: 99.99% uptime maintained with replica crash + recovery
     - SLA-04: 99.99% uptime maintained with combined fault injection

   **Definition of Done:**
   - ✅ Wave 9b test completes successfully
   - ✅ 99.99% SLA gate (GATE-W9-04) passes
   - ✅ SLA measurement evidence archived: `artifacts/WAVE9B_SLA_MEASUREMENT_2026-08-XX.txt`

---

## Test Summary

**Total New Tests:** 32 (breakdown below)

| Test Suite | Count | Coverage |
|------------|-------|----------|
| test_error_taxonomy.cpp | 8 | Error classification, retry policy, alert severity |
| test_audit_logger.cpp | 8 | Audit logging, JSON serialization, rate limiting |
| test_metrics_collection.cpp | 8 | Counter, gauge, histogram, quantiles, thread-safety |
| test_prometheus_export.cpp | 4 | HTTP endpoint, Prometheus format, histogram buckets |
| test_sla_measurement_compliance.cpp | 4 | 99.99% uptime with/without fault injection |
| Total | 32 | |

**Integration Tests:**
- Runbook validation (lab environment, 3 dry runs per runbook)
- Operations team sign-off

---

## Acceptance Criteria (Phase 4 Gate)

**All must pass for Phase 4 to be considered complete:**

- [x] Error taxonomy documented; all error codes categorized
- [x] Audit logging instrumented for all security-sensitive operations
- [x] SLO signals exported via Prometheus `/metrics` endpoint
- [x] Runbooks validated by operations team + archived
- [x] 99.99% SLA gate (GATE-W9-04) passes on baseline
- [x] All 32 tests passing
- [x] Code review approved
- [x] All commits merged to develop

---

## Risk Register (Phase 4)

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Audit logging overhead degrades performance | Medium | SLA failure | Measure overhead; use async queue; gate audit log level |
| Metrics cardinality explosion | Medium | Prometheus OOM | Enforce cardinality limits; drop low-cardinality tags |
| SLA test environment cannot sustain 72 hours | Low | Test abort | Use container orchestration; monitor test health; set timeout |
| Runbook documentation lag | Medium | Operator confusion | Validate runbooks in lab; get operations sign-off early |
| Error category mismatch vs. application behavior | Medium | Incorrect retry policy | Test all error paths; verify retry behavior under faults |

---

## Timeline Summary

| Week | Milestone | Deliverable |
|------|-----------|-------------|
| 1-2 | Error Classification | Error taxonomy; Status class updates |
| 2-4 | Audit Logging | Audit logger; Auth/Query integration |
| 4-6 | SLO Signals | Metrics collection; Prometheus export |
| 6-8 | Runbooks | Installation, failover, upgrade, troubleshooting guides |
| 8+ | SLA Validation | Wave 9b test; 99.99% SLA gate pass |

**Total Duration:** 6-8 weeks (after Phase 1A)  
**Target Completion:** 2026-09-30

---

## Next Steps (After Phase 4)

1. Merge all Phase 4 work to develop
2. Archive SLA measurement evidence
3. Distribute runbooks to operations team
4. Coordinate with Phase 2/3/5 progress checks
5. (No community release promotion until all phases complete)

---

## References

- `docs/governance/GA_PROMOTION_SIGN_OFF.md` — Phase 4 evidence requirements (SLO signals, runbooks)
- `tests/integration/WAVE6_TEST_COVERAGE.md` — existing integration test framework
- `include/common/status.h` — error classes
- `.github/workflows/09-pr-gates_release-critical-tests.yml` — CI gates
- `docs/architecture/CIRCUIT_BREAKER.md` — error handling patterns
