# API Transport Operations Runbook

**ThemisDB API Transport Critical-Path Operations Guide**

Version: 1.0  
Last Updated: 2026-08-19  
Wave: D (Q1 2027 operability deliverable)

---

## Table of Contents

1. [Overview](#1-overview)
2. [Common Failure Scenarios and Diagnosis](#2-common-failure-scenarios-and-diagnosis)
3. [Operator Remediation Steps](#3-operator-remediation-steps)
4. [Health Check and Alerting Thresholds](#4-health-check-and-alerting-thresholds)
5. [Escalation Path and SLA Targets](#5-escalation-path-and-sla-targets)

---

## 1. Overview

### Purpose

This runbook provides operator procedures for managing and diagnosing the
ThemisDB API transport layer in production environments.  It covers the four
critical transport paths and the OTLP distributed-tracing exporter that
instruments them.

### Scope

- All ThemisDB deployments that expose at least one transport endpoint
- Production, staging, and canary environments
- On-call engineers and SREs performing triage within SLA windows

### Transport Critical Paths

```
┌──────────────────────────────────────────────────────────────┐
│                        Client Layer                          │
└───────────────┬──────────────┬──────────────┬───────────────┘
                │              │              │
                ▼              ▼              ▼
         ┌──────────┐  ┌────────────┐  ┌───────────┐
         │ GraphQL  │  │    gRPC    │  │ WebSocket │
         │ /graphql │  │ :50051     │  │ /ws       │
         └────┬─────┘  └─────┬──────┘  └─────┬─────┘
              │              │               │
              └──────────────┴───────────────┘
                             │
                             ▼
                  ┌──────────────────────┐
                  │  Core Request Router │
                  │  (api_transport_     │
                  │   policy.cpp)        │
                  └──────────┬───────────┘
                             │
              ┌──────────────┴──────────────┐
              │                             │
              ▼                             ▼
   ┌──────────────────────┐    ┌───────────────────────┐
   │  AQL Query Engine    │    │   OTLP Span Exporter  │
   │  (aql/)              │    │   (otlp_exporter.cpp) │
   └──────────────────────┘    └───────────────────────┘
                                            │
                                            ▼
                               ┌───────────────────────┐
                               │  OpenTelemetry        │
                               │  Collector (external) │
                               └───────────────────────┘
```

### Prerequisites

- Read access to ThemisDB structured logs (JSON, field `module=api`)
- Access to Prometheus/Grafana dashboards
- `curl` or `grpcurl` available on the ops workstation
- ThemisDB admin credentials for `/healthz` and `/readyz` endpoints

---

## 2. Common Failure Scenarios and Diagnosis

### 2.1 GraphQL Endpoint Unresponsive

**Symptoms**
- HTTP 503 or connection refused on `/graphql`
- Prometheus alert `api_graphql_error_rate > 0.05`

**Diagnosis commands**
```bash
# Check HTTP health
curl -sf http://<host>:<port>/healthz | jq .

# Issue a minimal introspection query
curl -sf -X POST http://<host>:<port>/graphql \
  -H "Content-Type: application/json" \
  -d '{"query":"{ __typename }"}' | jq .

# Check structured logs for GraphQL parser errors
grep '"module":"api"' /var/log/themisdb/api.log \
  | grep '"component":"graphql"' \
  | tail -50
```

**Expected healthy response**
```json
{"data":{"__typename":"Query"}}
```

---

### 2.2 gRPC Server Not Accepting Connections

**Symptoms**
- `grpcurl` returns `Failed to dial target host`
- Prometheus alert `api_grpc_active_streams == 0` for > 60 s

**Diagnosis commands**
```bash
# Check gRPC server liveness (replace port as configured)
grpcurl -plaintext <host>:50051 grpc.health.v1.Health/Check

# Check for blocking-accept log (ERR_GRPC_ACCEPT_BLOCKED)
grep 'ERR_GRPC_ACCEPT_BLOCKED\|blocking_no_timeout' \
  /var/log/themisdb/api.log | tail -20

# Inspect file-descriptor limits (common cause of accept stall)
cat /proc/$(pgrep themisdb)/limits | grep 'open files'
```

**Expected healthy response**
```
{
  "status": "SERVING"
}
```

---

### 2.3 WebSocket Connections Dropping

**Symptoms**
- Clients report intermittent disconnections
- `api_ws_connection_resets_total` counter rising

**Diagnosis commands**
```bash
# Upgrade test with verbose output
curl -v --no-buffer \
  -H "Upgrade: websocket" \
  -H "Connection: Upgrade" \
  -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
  -H "Sec-WebSocket-Version: 13" \
  http://<host>:<port>/ws

# Look for session-limit errors
grep 'ERR_OBSERVABILITY_SESSION_LIMIT\|ERR_WS_SESSION_LIMIT' \
  /var/log/themisdb/api.log | tail -30
```

---

### 2.4 OTLP Span Exporter Not Delivering Traces

**Symptoms**
- `otlp_spans_dropped_total` counter rising faster than `otlp_spans_exported_total`
- No new traces visible in Jaeger/Tempo UI
- Log message: `OtlpExporter: export failed` or `ERR_OTLP_EXPORT_FAILED`

**Diagnosis commands**
```bash
# Verify OTLP collector endpoint is reachable
curl -sf -o /dev/null -w "%{http_code}" \
  http://<OTLP_ENDPOINT>/v1/traces

# Inspect exporter log stream
grep 'OtlpExporter\|ERR_OTLP' /var/log/themisdb/api.log \
  | grep -E 'WARN|ERROR' | tail -30

# Check Prometheus drop ratio
# If otlp_spans_dropped_total / otlp_spans_exported_total > 0.1 → alert
```

---

### 2.5 High Request Latency (p99 > SLA)

**Symptoms**
- Prometheus alert `api_request_latency_p99_ms > 2000`
- Client-visible timeout errors

**Diagnosis commands**
```bash
# Check current p95/p99 from Prometheus
curl -sg 'http://<prometheus>:9090/api/v1/query' \
  --data-urlencode 'query=histogram_quantile(0.99, rate(api_request_duration_ms_bucket[5m]))' \
  | jq '.data.result[].value[1]'

# Identify slow AQL queries
grep '"component":"aql_resolver"' /var/log/themisdb/api.log \
  | jq 'select(.duration_ms > 1000) | {query, duration_ms}' | head -20
```

---

## 3. Operator Remediation Steps

### ERR_API_INVALID_REQUEST

**Description:** Request was rejected at input validation (malformed method, path, or body).

**Remediation**
1. Inspect the client SDK version — update if older than the server's minimum required version.
2. Validate request schema against `/openapi/api.yaml` or `/graphql` introspection.
3. Check `X-Correlation-ID` in client logs to correlate with server-side structured log entry.
4. If caused by a proxy stripping required headers, configure the proxy to forward `Content-Type` and `Authorization`.

---

### ERR_API_UNAUTHORIZED

**Description:** ****** is missing, expired, or has insufficient scope.

**Remediation**
1. Renew credentials: `POST /auth/token` with valid client credentials.
2. Verify the token carries the required scope (`themisdb:read` or `themisdb:write`).
3. Check system clock skew — JWT validation fails if clocks differ by > 5 min.
4. If using mTLS, verify the client certificate is signed by the configured CA (`tls_ca_cert`).

---

### ERR_API_RATE_LIMITED

**Description:** Request rate exceeded the tenant's configured quota.

**Remediation**
1. Reduce request frequency: implement exponential back-off with jitter in the client.
2. Check `Retry-After` response header for the server-suggested back-off window.
3. To raise quota, file a support ticket with tenant ID and target RPS.
4. For bulk operations, batch requests where the API supports it (GraphQL batching, gRPC streaming).

---

### ERR_API_INTERNAL

**Description:** Unhandled internal error in a transport handler.

**Remediation**
1. Locate the correlated `X-Correlation-ID` in structured logs for a full stack trace.
2. Check recent deployments — roll back if the error started after a deploy.
3. If the error is in the AQL resolver path, verify the AQL query is syntactically valid.
4. File a bug report with the full log entry (redact PII before filing).

---

### ERR_OTLP_EXPORT_FAILED

**Description:** The OTLP span exporter could not deliver a batch after all retries.

**Remediation**
1. Verify `OTLP_ENDPOINT` is reachable from the ThemisDB host:
   ```bash
   curl -sf -o /dev/null -w "%{http_code}" http://<OTLP_ENDPOINT>/v1/traces
   ```
   Expected: `200` or `204`.
2. Check TLS configuration: if `tls_ca_cert` is set, ensure the CA certificate
   matches the collector's server certificate.
3. Inspect `otlp_spans_dropped_total` — if rising, the queue is exhausted; increase
   `max_queue_size` or reduce `flush_interval_ms`.
4. Enable debug logging (`THEMIS_LOG_LEVEL=DEBUG`) to see per-attempt HTTP response
   codes; a persistent `503` indicates a collector overload — scale the collector.

---

### ERR_OTLP_COLLECTOR_UNREACHABLE

**Description:** DNS resolution or TCP connection to the OTLP collector failed.

**Remediation**
1. Confirm DNS: `nslookup <collector-hostname>` from the ThemisDB host.
2. Confirm TCP: `nc -zv <collector-hostname> <port>`.
3. Check firewall rules — the ThemisDB host must have egress to the collector port.
4. If the collector is Kubernetes-internal, verify the service DNS (`<svc>.<ns>.svc.cluster.local`).

---

### ERR_GRPC_ACCEPT_BLOCKED

**Description:** The gRPC accept loop may be stalled due to resource exhaustion.

**Remediation**
1. Check open file descriptors: `lsof -p $(pgrep themisdb) | wc -l`.
2. Raise OS limit: `ulimit -n 65536` (or configure via systemd `LimitNOFILE`).
3. Check for zombie connections: `ss -s` — if `CLOSE_WAIT` count is high, a
   client is not closing connections cleanly; investigate the client-side keepalive config.
4. Restart the gRPC listener as a last resort (this is a soft restart — in-flight RPCs
   will complete before the listener port is re-bound).

---

### ERR_OBSERVABILITY_SESSION_LIMIT / ERR_OBSERVABILITY_QUEUE_FULL

**Description:** The in-process request queue or active-session counter hit its
configured capacity; new requests are being shed.

**Remediation**
1. Check whether the workload exceeds the provisioned instance size (CPU/memory).
2. Increase `max_queue_size` and `max_active_sessions` in `api_transport_policy` config
   (restart required).
3. Scale horizontally — add a second ThemisDB instance behind a load balancer.
4. Enable request back-pressure in upstream clients (circuit breaker pattern).

---

## 4. Health Check and Alerting Thresholds

### 4.1 Health Check Endpoints

| Endpoint    | Purpose                                | Expected Response |
|-------------|----------------------------------------|-------------------|
| `GET /healthz` | Liveness — is the process alive?    | `200 {"status":"ok"}` |
| `GET /readyz`  | Readiness — can it serve traffic?   | `200 {"status":"ready"}` |
| `GET /metrics` | Prometheus scrape endpoint          | `200 text/plain` |

**Frequency:** scrape every 15 s; liveness probe every 10 s; readiness probe every 5 s.

---

### 4.2 Alerting Thresholds

| Metric | WARNING | CRITICAL | Action |
|--------|---------|----------|--------|
| `api_request_latency_p95_ms` | > 500 ms | > 2 000 ms | Profile AQL queries; check DB latency |
| `api_request_latency_p99_ms` | > 1 000 ms | > 5 000 ms | Scale or throttle; page on-call |
| `api_error_rate` (5 m window) | > 1 % | > 5 % | Check logs; roll back recent deploy |
| `otlp_spans_dropped_total` rate | > 10/s | > 100/s | Scale OTLP collector; increase queue |
| `api_grpc_active_streams` | 0 for > 30 s | 0 for > 60 s | Check gRPC listener; restart if needed |
| `api_ws_connection_resets_total` rate | > 5/s | > 50/s | Check keepalive config; inspect TLS |
| `api_queue_depth` | > 80 % of max | > 95 % of max | Scale or throttle; raise `max_queue_size` |
| Process RSS memory | > 80 % of limit | > 95 % of limit | Investigate memory growth; restart |

---

### 4.3 Example Prometheus Alert Rules (YAML)

```yaml
groups:
  - name: themisdb_api
    rules:
      - alert: ApiHighErrorRate
        expr: |
          rate(api_requests_total{status=~"5.."}[5m]) /
          rate(api_requests_total[5m]) > 0.05
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "API error rate > 5%"
          runbook: "docs/API_TRANSPORT_RUNBOOK.md#err_api_internal"

      - alert: OtlpSpansDropping
        expr: rate(otlp_spans_dropped_total[5m]) > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "OTLP span drop rate > 10/s"
          runbook: "docs/API_TRANSPORT_RUNBOOK.md#err_otlp_export_failed"

      - alert: ApiP99LatencyCritical
        expr: |
          histogram_quantile(0.99,
            rate(api_request_duration_ms_bucket[5m])) > 2000
        for: 3m
        labels:
          severity: critical
        annotations:
          summary: "API p99 latency > 2 000 ms"
          runbook: "docs/API_TRANSPORT_RUNBOOK.md#25-high-request-latency-p99--sla"
```

---

## 5. Escalation Path and SLA Targets

### 5.1 SLA Targets

| Tier | Target Availability | p99 Latency Budget | Max Error Rate |
|------|---------------------|--------------------|----------------|
| Production | 99.9 % / month | 2 000 ms | 0.1 % |
| Staging | 99.0 % / month | 5 000 ms | 1.0 % |
| Development | Best-effort | No SLA | No SLA |

---

### 5.2 Escalation Path

```
Level 1 — On-Call SRE (0–30 min)
  • Triage using Section 2 diagnosis commands
  • Apply Section 3 remediation steps
  • If resolved: update incident log and close

Level 2 — API Module Owner (30–90 min)
  • Triggered when Level 1 cannot restore service
  • Deep log analysis; may apply config change or hot-fix deployment
  • Contact: see MAINTAINERS.md → api module owner

Level 3 — Engineering Leadership (90 min+)
  • Customer-impacting P0 incidents
  • Decision authority for: emergency rollback, traffic re-routing, SLA credit
  • Notify via incident channel and pagerduty escalation policy

External — ThemisDB Support (async, non-P0)
  • File at: SUPPORT.md
  • Include: log bundle, Prometheus snapshot, reproduction steps
```

---

### 5.3 Incident Classification

| Severity | Definition | Response Time |
|----------|------------|---------------|
| P0 — Critical | Complete API outage or data loss | 15 min |
| P1 — High | Single transport degraded (> 5 % errors) | 30 min |
| P2 — Medium | Elevated latency or elevated drop rate | 2 h |
| P3 — Low | Non-blocking anomaly, monitoring alert | Next business day |

---

### 5.4 Post-Incident Actions

1. Root-cause analysis (RCA) document filed within 24 h of P0/P1 resolution.
2. RCA linked from `CHANGELOG.md` in the release that ships the fix.
3. New test coverage added for the failure mode (see Wave D test pattern in
   `tests/api/test_api_wave_d_stress.cpp`).
4. Runbook updated if a new failure class was encountered.

---

*For module architecture context see `src/api/ROADMAP.md`.*  
*For CDC-specific operations see `docs/CDC_OPERATIONS_RUNBOOK.md`.*
