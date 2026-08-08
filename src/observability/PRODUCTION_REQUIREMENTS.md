# ThemisDB Observability Module - Production Requirements

**Status:** 2026-08-08 – Phase 6 Complete; Block B hardening acceptance  
**Document Version:** 2.0

## Purpose and Scope

This document is the **canonical reference for mandatory production requirements** of the Observability Module. It defines binding operational and security requirements for:
- Metrics Collection & Aggregation
- Distributed Tracing & Span Propagation
- SLO Measurement & Error Budget Tracking
- Query Profiling & Performance Diagnostics
- Provenance Storage & Audit Logging
- Alerting & Anomaly Detection

## Document Governance (Canonical Split)

- **`src/observability/PRODUCTION_REQUIREMENTS.md` (this document):** Mandatory production requirements (MUST/MUST NOT), security assumptions, operational limits.
- **`src/observability/README.md`:** Feature overview, architectural context, API and usage examples.
- **`src/observability/ROADMAP.md`:** Delivery phases, open/completed features, readiness planning.
- **`src/observability/FUTURE_ENHANCEMENTS.md`:** Medium/long-term enhancements and research directions.
- **`src/observability/PERFORMANCE_EXPECTATIONS.md`:** Benchmark gates, p95/p99 targets, release validation.

## Mandatory Production Requirements

### Metrics Ingest & Collection
- **MUST:** Metrics collector enforces bounded cardinality via label count and value size limits (kMaxMetricLabels=50, kMaxLabelKeyBytes=256, kMaxLabelValueBytes=1024)
- **MUST:** Oversized or malformed metrics are rejected with explicit error code (kMetricCardinalityExceeded, kMetricMalformedLabel) and counted in rejection counter
- **MUST:** Rejection counter (`malformed_telemetry_rejections_total{metric=...,reason=...}`) is surface and queryable for monitoring
- **MUST:** High-cardinality metrics are handled fail-closed (rejection, not silent drop)

### Tracing & Span Propagation
- **MUST:** Span context propagation follows W3C Trace Context specification or configured alternate standard
- **MUST:** Trace sampling is configured deterministically; no 100% sampling in production without explicit configuration and memory/retention policy
- **MUST:** Span depth limit (kMaxSpanDepth) is enforced; spans exceeding depth are recorded with overflow counter, not silently dropped
- **MUST:** Parent-child span linkage remains explicit; orphaned spans are detected and reported via diagnostics

### SLO Measurement & Error Budget
- **MUST:** SLO window alignment is deterministic (wall-clock second boundaries or configurable epoch)
- **MUST:** Partial windows (e.g., startup windows) are handled consistently (excluded from SLO status or explicitly marked partial)
- **MUST:** Error budget exhaustion triggers explicit SLO violation status and notification hook
- **MUST:** Clock skew tolerance is documented and enforced via timestamp validation

### Alert Delivery & Notification
- **MUST:** Alert rule evaluation is deterministic; identical input always produces identical output
- **MUST:** Alert delivery failures surface explicitly (exporter_failures_total counter, health status endpoint)
- **MUST NOT:** Silent alert loss; all delivery failures are logged and counted
- **MUST:** Notification channels (email, webhook, PagerDuty) are configurable with explicit delivery retry semantics

### Query Profiling & Performance Diagnostics
- **MUST:** Query profile recording remains bounded (adaptive sampling ≤1% overhead unless configured otherwise)
- **MUST:** Histogram bins are calculated deterministically from query phase boundaries
- **MUST:** Profiler data retention follows explicit time-window policy (no indefinite accumulation)

### Provenance Storage & Audit
- **MUST:** Provenance records are immutable after recording (append-only semantics)
- **MUST:** Retention policy is enforced explicitly (no automatic expiration without explicit policy)
- **MUST:** Audit event queries are consistent with snapshot isolation (no partial/torn reads)

## Mandatory Security Requirements

- **MUST:** Observability data contains no sensitive user data (PII); masking policy active for all telemetry flows
- **MUST:** Span context remains confidential (no credentials, API keys, or passwords in baggage)
- **MUST:** Metrics labels and alert rule conditions do not leak authorization decisions
- **MUST NOT:** Disable security checks in production paths; fail closed on validation failure
- **MUST:** All security-relevant configuration values validated at startup; missing/invalid values cause startup failure, not silent degradation
- **Mandatory Security Audit:** Dedicated control surfaces for security-relevant operations (authz checks, encryption, key rotation)

## Edge-Case Guarantees

### High-Cardinality Metrics Behavior
- **Guarantee:** Metrics collector remains responsive under unbounded label cardinality (via cardinality limits and rejection)
- **Behavior:** New label combinations beyond limit are rejected with diagnostic counter; existing combinations continue recording
- **Fallback:** Manual cardinality reduction via masking rules or label-name allowlist

### Malformed Telemetry Handling
- **Guarantee:** Malformed input does not cause silent data loss or corruption
- **Behavior:** Invalid metrics/spans are rejected and counted; error response surfaces to caller
- **Recovery:** Monitoring alerts on elevated rejection rate; diagnostic surface for troubleshooting

### Span Context Loss Recovery
- **Guarantee:** Orphaned spans are detected and do not create causality breaks in trace graphs
- **Behavior:** Orphaned spans are recorded with explicit "orphaned" marker; trace continuity is preserved via correlation IDs
- **Fallback:** Operator can manually re-correlate traces via query interface or correlation ID logs

### Clock Skew Handling
- **Guarantee:** Timestamp validation remains consistent across SLO windows and metric aggregations
- **Behavior:** Out-of-order or skewed timestamps trigger diagnostic event; window assignment follows configurable policy (use current time or reject)
- **Fallback:** Configuration option to accept time-skewed metrics with explicit logging

### Exporter Unavailability
- **Guarantee:** Exporter failures do not block metrics recording or trace collection
- **Behavior:** Export failures update health status and failure counter; buffered data is retained up to configured memory limit
- **Fallback:** Circuit breaker disables export temporarily; data is retained and re-exported on recovery

## Operational Requirements

### Resource Limits

| Resource Type | Limit | Justification |
|---------------|-------|---------------|
| Max Metric Labels | 50 per metric | Bounded cardinality; prevents label explosion |
| Label Key Size | 256 bytes | Reasonable key identifier length |
| Label Value Size | 1024 bytes | Reasonable value length; prevents oversized strings |
| Max Span Depth | 100 levels | Reasonable nesting depth for call hierarchies |
| Baggage Size | 8KB | W3C Trace Context baggage limit |
| Max SLO Window | 30 days | Bounded error budget tracking; prevents month-level loss |
| Alert Rule Count | 10,000 | Reasonable rule set for distributed deployments |
| Provenance Retention | 90 days | Balance retention cost vs. audit requirements |

### External Dependency Configuration

| Dependency | Requirement |
|------------|-------------|
| Metrics Exporter (Prometheus/OpenTelemetry) | Connection timeout ≤10s; retry policy 3x exponential backoff |
| Tracing Backend (Jaeger/OpenTelemetry Collector) | Connection timeout ≤5s; buffer size 100K spans; drop-on-overflow |
| Alert Notifier (Email/Webhook/PagerDuty) | Delivery timeout ≤30s; retry policy 5x exponential backoff |
| Audit Store (Database/S3/HDFS) | Connection timeout ≤10s; max batch size 10K records |

### Production Environment Validation Checklist

- [ ] Module configuration complete and validated at startup
- [ ] Security and authorization checks active
- [ ] Resource limits explicitly configured (no unlimited defaults)
- [ ] Audit logging active for security events
- [ ] External dependencies configured with timeout and retry policies
- [ ] Production mode via `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT` set
- [ ] Observability metrics exporter running and accepting connections
- [ ] Trace collector configured and accepting spans
- [ ] Alert rules validated and syntax-checked
- [ ] Provenance retention policy set and validated

## Monitoring & Observability Requirements

| Metric | Purpose | Alert Threshold |
|--------|---------|------------------|
| `telemetry_rejections_total` | Rejected metrics/spans due to malformation or limits | >1% of total ingest |
| `exporter_failures_total` | Export backend failures (Prometheus, Jaeger, etc.) | >10 per minute |
| `exporter_health_status` | Boolean status of exporter connection | False = alert |
| `slo_violations_total` | SLO violations detected | >5 per hour |
| `span_orphaned_count` | Orphaned spans without parent context | >1% of spans |
| `alert_delivery_failures` | Failed alert notifications | >5 per hour |
| `profiler_overhead_percent` | Query profiler CPU overhead | >2% |

## Documentation References

- **API Contracts:** `include/observability/observability_api_contract.h`, `include/observability/metrics_collector.h`, `include/observability/opentelemetry_tracer.h`
- **Specifications:** W3C Trace Context, OpenTelemetry Specification, Prometheus Exposition Format
- **Operational Runbooks:** `docs/observability/OPERATIONAL_RUNBOOK.md` (forthcoming)
- **Monitoring Dashboards:** Grafana dashboards available in `dashboards/observability/`

## Verification & Audit

### Self-Audit Checklist (Phase 6)

**Configuration:**
- [ ] Metrics cardinality limits configured and validated
- [ ] Trace sampling policy configured (rate ≤1.0)
- [ ] SLO window boundaries aligned and synchronized
- [ ] Alert rules syntax-checked and loaded

**Operations:**
- [ ] Observability exporter health monitored and alerts active
- [ ] Rejection/failure counter thresholds set and monitored
- [ ] Provenance retention policy applied and data rotated
- [ ] Clock synchronization checked across all nodes (skew <1s)

**Security:**
- [ ] Span context masking rules applied (no credentials in baggage)
- [ ] Audit logging verified for security events
- [ ] Authorization checks tested in production-like environment
- [ ] External dependency connections encrypted (TLS/mTLS)

**Performance:**
- [ ] Observability gates (ORG/OBA/OBB) verified to PASS on current hardware
- [ ] Exporter latency within SLA (<5ms baseline)
- [ ] Query profiler overhead <1% on representative workload
- [ ] Memory usage stable under sustained high-cardinality load (no leaks)

## Known Limitations & Mitigations

| Limitation | Impact | Mitigation |
|-----------|--------|-----------|
| Metric cardinality hard limit (50 labels) | High-cardinality workloads may exceed limit | Pre-aggregate labels; use label allowlist |
| Trace sampling non-deterministic under load | Sampling bias under high throughput | Use deterministic hash-based sampling |
| SLO window alignment wall-clock bound | Startup windows create SLO boundary artifacts | Ignore first window; or configure skip-startup |
| Span context baggage 8KB limit | Large trace metadata cannot fit | Use correlation ID and external lookup table |
| Exporter buffer limited to 100K spans | Large trace bursts cause span drop | Increase buffer size or add buffering tier |

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-06-01 | Initial German version with basic requirements |
| 2.0 | 2026-08-08 | Phase 6 expansion: comprehensive English version with Block B hardening specifics |

---

**Document Signed Off:** 2026-08-08  
**Verification Timestamp:** 2026-08-08 14:02:00 UTC  
**Next Review Target:** 2026-10-08 (Q4 2026 release)
