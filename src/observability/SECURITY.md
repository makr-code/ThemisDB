> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Observability Module

## Threat Model

### 1. Metrics Endpoint Exposure
- **Risk:** The `/metrics` Prometheus endpoint exposes internal system state (query rates, memory usage, error counts) to unauthenticated callers, potentially leaking operational intelligence.
- **Mitigation:** Authentication is required to access the `/metrics` endpoint in production deployments. Configuration enforces credential validation before serving Prometheus text output.
- **Status:** ⚠️ Open — authentication implementation is an active work item (OBS-SEC-01)

### 2. Trace Data PII Leakage
- **Risk:** Distributed trace spans may capture query parameters, user identifiers, or other PII as span attributes, which are then exported via OTLP to external systems.
- **Mitigation:** Span attribute sanitization is planned. A configurable allow-list of exportable attribute keys will be applied at the OTLP export layer before data leaves the process.
- **Status:** ⚠️ Open — PII scanning is a planned work item (OBS-SEC-02)

### 3. Log Injection
- **Risk:** Unsanitized user-controlled strings written into log output could corrupt log parsers, inject false log entries, or exploit log aggregation pipelines.
- **Mitigation:** All log output from `LogAggregator` uses structured JSON format. User-controlled values are JSON-encoded as string fields, preventing injection of newlines, control characters, or ANSI escape sequences into the log stream.
- **Status:** ✅ Implemented

### 4. Alert Webhook Spoofing
- **Risk:** An attacker crafts a forged alert payload and delivers it to the webhook endpoint, triggering false alerts or suppressing legitimate ones.
- **Mitigation:** Webhook endpoint validation enforces that inbound alert payloads originate from configured sources. URL and payload signature validation are applied before alert processing.
- **Status:** ✅ Implemented

### 5. Prometheus Cardinality Attacks
- **Risk:** An attacker or misconfigured client causes unbounded label cardinality (e.g., per-request labels), exhausting memory in the metrics registry.
- **Mitigation:** `MetricAggregator` enforces per-metric cardinality limits. Label sets exceeding the configured threshold are rejected, and a counter tracks dropped high-cardinality observations for operator visibility.
- **Status:** ✅ Implemented

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| OBS-SEC-01 | Metrics endpoint (`/metrics`) authentication is not fully implemented. The endpoint may be accessible without credentials in default deployments. | High | Open |
| OBS-SEC-02 | Trace span PII scanning is not implemented. Span attributes may contain query text or user identifiers that are exported via OTLP. | Medium | Open |

---

## Security Configuration Reference

| Parameter | Description | Recommended Value |
|-----------|-------------|-------------------|
| `metrics.auth.enabled` | Require authentication for `/metrics` endpoint | `true` |
| `metrics.auth.token` | Bearer token for Prometheus scrape authentication | Strong random ≥ 32 bytes |
| `tracing.export.attribute_allowlist` | Explicit allow-list of span attribute keys for OTLP export | Restrict to non-PII keys |
| `metrics.cardinality.max_labels_per_metric` | Maximum distinct label combinations per metric | 10 000 (tune per workload) |
| `alertmanager.webhook.validate_source` | Validate inbound alert webhook origin | `true` |
| `logging.format` | Log output format | `json` |

---

## Security Review History

| Date | Reviewer | Scope | Outcome |
|------|----------|-------|---------|
| 2026-03-12 | Internal security review | Metrics auth, trace PII, log injection, webhook validation, cardinality | OBS-SEC-01 and OBS-SEC-02 filed; remaining controls passed |
