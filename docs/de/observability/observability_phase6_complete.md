# Phase 6: Monitoring - Complete Implementation Guide

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔍 Observability  
**Status:** ✅ COMPLETE (December 2025)

---

## 📑 Table of Contents

- [Overview](#overview)
- [Prometheus Metrics](#prometheus-metrics-reference)
- [Implementation](#implementation)

---

## Overview

Phase 6 of the horizontal scaling implementation provides comprehensive monitoring and observability for the ThemisDB sharding system. This includes Prometheus metrics, Grafana dashboards, and alerting rules.

## Prometheus Metrics Reference

### Shard Health Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_shard_health_status` | Gauge | `shard_id`, `status` | Current health status of each shard |
| `themis_shard_certificate_expiry_seconds` | Gauge | `shard_id` | Seconds until certificate expiration |
| `themis_cluster_size` | Gauge | - | Current number of shards |
| `themis_virtual_nodes_total` | Gauge | - | Total virtual nodes in consistent hash ring |

### Routing Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_routing_requests_total` | Counter | `type` | Total routing requests (local/remote/scatter_gather) |
| `themis_routing_errors_total` | Counter | `shard_id`, `error_type` | Routing errors by type |
| `themis_routing_latency_seconds` | Histogram | `operation` | Routing latency distribution |

### PKI/Security Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_pki_connections_total` | Counter | `shard_id`, `result` | mTLS connection attempts |
| `themis_pki_certificate_validations_total` | Counter | `result` | Certificate validation results |
| `themis_pki_crl_checks_total` | Counter | `result` | CRL check results |

### Migration Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_migration_records_total` | Gauge | `operation_id` | Records migrated |
| `themis_migration_bytes_total` | Gauge | `operation_id` | Bytes migrated |
| `themis_migration_progress_percent` | Gauge | `operation_id` | Migration progress (0-100) |
| `themis_migration_duration_seconds` | Gauge | `operation_id` | Migration duration |

### Query Performance Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_query_execution_seconds` | Histogram | `query_type` | Query execution time |
| `themis_scatter_gather_fanout` | Histogram | - | Number of shards hit per query |
| `themis_result_merge_time_seconds` | Histogram | - | Time to merge results from shards |

### Gossip Protocol Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_gossip_messages_total` | Counter | `type` | Gossip messages (heartbeat/peer_list/ack) |
| `themis_gossip_message_size_bytes` | Histogram | - | Message size distribution |
| `themis_gossip_roundtrip_seconds` | Histogram | - | Gossip round-trip latency |
| `themis_gossip_peer_count` | Gauge | - | Current peer count |
| `themis_gossip_failed_peers_total` | Counter | `peer_id` | Failed peer communications |
| `themis_gossip_version_vector` | Gauge | `peer_id` | Version vector for anti-entropy |

### Cross-Shard Join Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_cross_shard_joins_total` | Counter | `strategy` | Join operations (broadcast_hash/co_located) |
| `themis_cross_shard_join_duration_seconds` | Histogram | `strategy` | Join duration |
| `themis_cross_shard_join_left_rows` | Gauge | `strategy` | Left table row count |
| `themis_cross_shard_join_right_rows` | Gauge | `strategy` | Right table row count |
| `themis_cross_shard_join_result_rows` | Gauge | `strategy` | Result row count |
| `themis_hash_table_build_seconds` | Histogram | - | Hash table build time |
| `themis_probe_phase_seconds` | Histogram | - | Probe phase time |

### Content Processor Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_content_processor_invocations_total` | Counter | `type` | Processor invocations |
| `themis_content_processor_duration_seconds` | Histogram | `type` | Processing duration |
| `themis_content_processor_errors_total` | Counter | `type`, `error` | Processing errors |
| `themis_content_processor_last_input_bytes` | Gauge | `type` | Last input size |
| `themis_content_processor_last_output_bytes` | Gauge | `type` | Last output size |

### Metadata Store Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_metadata_store_operations_total` | Counter | `operation` | Operations (get/put/delete/watch) |
| `themis_metadata_store_latency_seconds` | Histogram | `operation` | Operation latency |
| `themis_metadata_store_errors_total` | Counter | `operation`, `error` | Operation errors |

### Health Check Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_health_check_executions_total` | Counter | `type` | Check executions |
| `themis_health_check_duration_seconds` | Histogram | `type` | Check duration |
| `themis_health_check_results_total` | Counter | `type`, `result` | Check results (healthy/warning/critical) |

### Cloud Agent / Multi-DC Metrics (NEW)

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `themis_cloud_agent_operations_total` | Counter | `operation` | Cloud agent operations |
| `themis_datacenter_latency_seconds` | Histogram | `datacenter` | DC latency |
| `themis_cross_dc_requests_total` | Counter | `source`, `target` | Cross-DC requests |

## Grafana Dashboard

### Dashboard JSON Location

```
config/grafana/dashboards/themis-sharding.json
```

### Recommended Panels

1. **Cluster Overview**
   - Cluster size gauge
   - Virtual nodes count
   - Shard health status map

2. **Routing Performance**
   - Requests per second by type
   - P50/P95/P99 latency charts
   - Error rate by shard

3. **Gossip Protocol Health**
   - Active peer count
   - Message rate by type
   - Gossip round-trip latency

4. **Cross-Shard Joins**
   - Join operations per minute
   - Join duration by strategy
   - Row counts (left/right/result)

5. **Content Processors**
   - Invocations by processor type
   - Processing duration heatmap
   - Error rate by processor

6. **Migration Progress**
   - Active migrations
   - Progress percentage
   - Records/bytes migrated

## Alerting Rules

### Critical Alerts

```yaml
groups:
  - name: themis-sharding-critical
    rules:
      - alert: ShardDown
        expr: themis_shard_health_status{status="unhealthy"} == 1
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Shard {{ $labels.shard_id }} is unhealthy"

      - alert: CertificateExpiringSoon
        expr: themis_shard_certificate_expiry_seconds < 604800  # 7 days
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Certificate for shard {{ $labels.shard_id }} expires in {{ $value | humanizeDuration }}"

      - alert: HighRoutingErrorRate
        expr: rate(themis_routing_errors_total[5m]) > 0.1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High routing error rate on shard {{ $labels.shard_id }}"
```

### Warning Alerts

```yaml
      - alert: GossipPeerCountLow
        expr: themis_gossip_peer_count < 2
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Gossip peer count is low ({{ $value }})"

      - alert: CrossShardJoinSlow
        expr: histogram_quantile(0.95, rate(themis_cross_shard_join_duration_seconds_bucket[5m])) > 5
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Cross-shard join P95 latency is high ({{ $value }}s)"

      - alert: ContentProcessorErrors
        expr: rate(themis_content_processor_errors_total[5m]) > 0.05
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Content processor {{ $labels.type }} has elevated error rate"
```

## Configuration

### Enable Metrics in ThemisDB Config

```yaml
monitoring:
  prometheus:
    enabled: true
    port: 9090
    path: /metrics
    enable_histograms: true
    histogram_buckets: 10

  metrics:
    gossip: true
    cross_shard_joins: true
    content_processors: true
    health_checks: true
    cloud_agent: true
```

### Prometheus Scrape Config

```yaml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['themisdb-1:9090', 'themisdb-2:9090', 'themisdb-3:9090']
    scrape_interval: 15s
    metrics_path: /metrics
```

## Integration with Existing Observability Stack

### OpenTelemetry Export

```cpp
// In prometheus_metrics.cpp
void PrometheusMetrics::exportToOTLP(const std::string& endpoint) {
    // Export metrics to OpenTelemetry collector
    // Implementation uses OTLP/gRPC protocol
}
```

### Jaeger Tracing Integration

Cross-shard operations automatically create distributed traces:

- Trace ID propagated via `X-Trace-ID` header
- Spans created for each shard operation
- Parent-child relationships maintained

## Files Changed

| File | Changes |
|------|---------|
| `include/sharding/prometheus_metrics.h` | Added 25+ new metric methods |
| `src/sharding/prometheus_metrics.cpp` | Implemented all new metrics |
| `docs/observability/PHASE6_MONITORING_COMPLETE.md` | This document (NEW) |

## Metrics Count Summary

| Category | Metrics Count |
|----------|---------------|
| Shard Health | 4 |
| Routing | 3 |
| PKI/Security | 3 |
| Migration | 4 |
| Query Performance | 3 |
| Gossip Protocol | 6 |
| Cross-Shard Joins | 7 |
| Content Processors | 5 |
| Metadata Store | 3 |
| Health Checks | 3 |
| Cloud Agent | 3 |
| **Total** | **44** |

## Next Steps

1. ✅ Implement all metric collection points in sharding code
2. ✅ Create Grafana dashboard templates
3. ✅ Define alerting rules
4. 🔄 Deploy to production monitoring stack
5. 🔄 Create runbooks for each alert
