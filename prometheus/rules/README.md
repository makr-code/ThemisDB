# prometheus/rules/

This directory contains all Prometheus alerting rule files for ThemisDB.
Files follow the standard Prometheus `rule_files` convention and are loaded
via `grafana/prometheus.yml`.

## Files

### `llm_alerts.yml`
Alert rules for ThemisDB LLM inference health:

| Alert | Condition | Severity |
|-------|-----------|----------|
| `LLMHighErrorRate` | LLM request error rate exceeds threshold | critical |
| `LLMHighLatency` | p99 inference latency exceeds SLO | warning |
| `LLMQueueDepth` | Inference queue depth exceeds capacity | warning |
| `LLMGPUUtilisation` | GPU utilisation anomaly / over-saturation | warning |
| `LLMThroughputDrop` | Tokens-per-second throughput below baseline | warning |
| `LLMCacheHitRateLow` | KV-cache hit rate below acceptable threshold | warning |

### `sla-rules.yml`
Alert rules for ThemisDB service-level objectives (SLOs):

| Alert | Condition | Severity |
|-------|-----------|----------|
| `ThemisDBAvailability` | Service availability drops below SLA target | critical |
| `ThemisDBHighLatency` | Request latency breaches SLO budget | warning |
| `ThemisDBErrorRateSLO` | Error rate multi-window SLO breach | critical |
| `ThemisDBErrorBudgetBurnRate` | Fast or slow error budget burn rate | warning |

## Loading Rules

Both files are loaded by adding them to the `rule_files` list in your Prometheus config:

```yaml
rule_files:
  - '/etc/prometheus/rules/llm_alerts.yml'
  - '/etc/prometheus/rules/sla-rules.yml'
```

Or with a glob (recommended):

```yaml
rule_files:
  - '/etc/prometheus/rules/*.yml'
```

## Validation

Validate rules locally with `promtool`:

```bash
promtool check rules prometheus/rules/llm_alerts.yml
promtool check rules prometheus/rules/sla-rules.yml
```

## See Also

- [prometheus/README.md](../README.md) — top-level overview
- [prometheus/ROADMAP.md](../ROADMAP.md) — planned enhancements
- [grafana/prometheus.yml](../../grafana/prometheus.yml) — scrape & rule_files config
