<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Observability Module

## Scope

Covers all public headers in `include/observability/`. Implementation hardening details in `../../src/observability/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Exfiltration of query/user data via trace spans | High — PII leakage | `OpenTelemetryTracer` filters spans against a deny-list of sensitive attribute keys before export |
| Unauthorized metric scrape endpoint | Medium — information disclosure | `MetricsStreamServer` requires mTLS client cert; bearer token optional |
| eBPF privilege escalation | High — kernel code execution | `EbpfTracer` programs are verified by the kernel verifier; CAP_BPF required, not CAP_SYS_ADMIN |
| Alertmanager webhook SSRF | Medium — internal network probe | `AlertmanagerClient` validates URLs against allowlist; no redirect following |
| ML model poisoning via injected metrics | Medium — false anomaly suppression | `MlAnomalyDetector` models retrained only from authenticated metric sources |
| Log injection (CRLF) | Low — log forging | `LogAggregator` escapes control characters before forwarding to Loki/OTLP |
| Flame graph data leakage | Medium — function/symbol exposure | `DistributedFlameGraph` redacts frame symbols based on RBAC role |

## Security Controls

1. **OTLP export over TLS 1.3** — `OpenTelemetryTracer` requires TLS; plaintext only in `localhost` dev mode.
2. **Attribute deny-list** — PII fields (`user_id`, `client_ip`, `query_text`) are stripped from exported spans.
3. **CAP_BPF least-privilege** — `EbpfTracer` loads programs under `CAP_BPF + CAP_PERFMON` only.
4. **mTLS on metrics endpoint** — `MetricsStreamServer` enforces mutual TLS for external scrapers.
5. **Allowlist-based alerting targets** — `AlertmanagerClient` rejects non-allowlisted webhook URLs.
6. **RBAC-gated flame graphs** — `DistributedFlameGraph` checks caller role before including kernel frames.

## Known Limitations

- `EbpfTracer` requires kernel ≥ 5.15 with BTF; falls back to userspace tracing on older kernels.
- Anomaly detection models (`MlAnomalyDetector`) are not adversarially hardened against deliberate metric manipulation — tracked for Q4 2026.
