# prometheus/

Prometheus configuration and alert rule files for ThemisDB observability.

## Purpose

This directory contains Prometheus alerting rules used by the ThemisDB monitoring stack.
Rules are loaded by Prometheus at runtime via the `rule_files` stanza in `grafana/prometheus.yml`
and evaluated on the configured `evaluation_interval` (default: 15 s).

## Directory Structure

```
prometheus/
└── rules/                  # Prometheus alert rule files (*.yml)
    ├── llm_alerts.yml      # LLM inference alert rules
    └── sla-rules.yml       # SLA / availability alert rules
```

## Rules Overview

### `rules/llm_alerts.yml`
Alert rules covering ThemisDB LLM inference health:

| Alert | Description |
|-------|-------------|
| `LLMHighErrorRate` | LLM request error rate exceeds threshold |
| `LLMHighLatency` | p99 inference latency exceeds SLO |
| `LLMQueueDepth` | Inference queue depth exceeds capacity |
| `LLMGPUUtilisation` | GPU utilisation anomaly / over-saturation |
| `LLMThroughputDrop` | Tokens-per-second throughput below baseline |
| `LLMCacheHitRateLow` | KV-cache hit rate below acceptable threshold |

### `rules/sla-rules.yml`
Alert rules covering overall ThemisDB service-level objectives:

| Alert | Description |
|-------|-------------|
| `ThemisDBAvailability` | Service availability below SLA target |
| `ThemisDBHighLatency` | Request latency SLO breach |
| `ThemisDBErrorRateSLO` | Error rate multi-window SLO breach |
| `ThemisDBErrorBudgetBurnRate` | Fast/slow burn rate budget exhaustion |

## How to Use

Reference both rule files in `grafana/prometheus.yml` (or your `prometheus.yml`) with a glob:

```yaml
rule_files:
  - '/etc/prometheus/rules/*.yml'
```

Or enumerate them explicitly:

```yaml
rule_files:
  - '/etc/prometheus/rules/llm_alerts.yml'
  - '/etc/prometheus/rules/sla-rules.yml'
```

Reload rules without restarting Prometheus:

```bash
curl -X POST http://localhost:9090/-/reload
```

## Runbooks

Runbooks for each alert are maintained in `docs/operations/`.
Follow the `runbook_url` annotation on each alert rule for a direct link.

## See Also

- [prometheus/ROADMAP.md](ROADMAP.md) — planned enhancements
- [prometheus/rules/README.md](rules/README.md) — per-file rule details
- [grafana/prometheus.yml](../grafana/prometheus.yml) — scrape & rule_files config
