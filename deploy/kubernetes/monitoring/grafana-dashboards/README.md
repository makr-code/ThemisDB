# ThemisDB Shard Network Grafana Dashboard

## Overview

This Grafana dashboard provides comprehensive monitoring for ThemisDB's distributed shard network, including:

- **Hub-Shard Network Overview**: Active hub/worker shards, cross-shard edges, URN cache metrics
- **Cross-Shard Query Performance**: Query rates, latency, and fanout distribution
- **Link Discovery**: Real-time tracking of automatic link detection across shards
- **Graph Topology Analysis**: Hub nodes, leaf nodes, degree distributions
- **Shard Communication**: Network traffic, routing patterns, error rates

## Dashboard Sections

### 1. Hub-Shard Network Overview
- Active hub and worker shards
- Total cross-shard edges tracked
- URN cache hit rate
- Average query fanout across shards

### 2. Cross-Shard Query Performance
- Graph search and hybrid search rates
- Scatter-gather query performance
- P95/P99 latency metrics
- Query fanout distribution

### 3. Link Discovery
- Link discovery rates by method (URN pattern, NLP, metadata)
- Documents scanned and links validated
- Cross-shard link tracking
- Confidence distribution histogram

### 4. Graph Topology Analysis
- **Top 10 Referenced Entities**: Most important hub nodes (authorities, policies)
- **InDegree Distribution**: How many references each entity receives
- **Low-Referenced Entities**: Orphaned or isolated documents
- **Average Degree Trends**: Graph connectivity over time
- **Max InDegree**: Most heavily referenced entity

### 5. Shard Communication
- Local vs remote vs scatter-gather request breakdown
- Network traffic between shards
- Routing error rates

## Prerequisites

- Grafana 9.0+ installed
- Prometheus data source configured
- ThemisDB with Prometheus metrics enabled

## Installation

### Method 1: Import via Grafana UI

1. Log in to your Grafana instance
2. Navigate to **Dashboards** → **Import**
3. Click **Upload JSON file**
4. Select `themisdb-shard-network-dashboard.json`
5. Select your Prometheus data source
6. Click **Import**

### Method 2: Import via kubectl (Kubernetes)

```bash
# Apply the dashboard ConfigMap
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: themisdb-shard-network-dashboard
  namespace: monitoring
  labels:
    grafana_dashboard: "1"
data:
  themisdb-shard-network-dashboard.json: |
$(cat themisdb-shard-network-dashboard.json | sed 's/^/    /')
EOF
```

### Method 3: Grafana Operator (Kubernetes)

```yaml
apiVersion: integreatly.org/v1alpha1
kind: GrafanaDashboard
metadata:
  name: themisdb-shard-network
  namespace: monitoring
spec:
  json: |
    <paste dashboard JSON here>
  datasources:
    - inputName: "DS_PROMETHEUS"
      datasourceName: "Prometheus"
```

## Required Prometheus Metrics

This dashboard expects the following metrics to be exposed by ThemisDB:

### Hub-Shard Metrics
```
themisdb_shard_healthy{role="hub|worker"}
themisdb_hub_cross_shard_edges_total
themisdb_hub_urn_cache_hit_rate
themisdb_hub_query_fanout_avg
themisdb_hub_graph_searches_total
themisdb_hub_hybrid_searches_total
```

### Routing Metrics
```
themisdb_routing_requests_total{type="local|remote|scatter_gather"}
themisdb_routing_latency_ms_bucket
themisdb_routing_errors_total
themisdb_scatter_gather_fanout
```

### Link Discovery Metrics
```
themisdb_link_discovery_documents_scanned_total
themisdb_link_discovery_links_found_total
themisdb_link_discovery_links_validated_total
themisdb_link_discovery_cross_shard_links_total
themisdb_link_discovery_links_by_method{method="urn_pattern|nlp_entity|metadata"}
themisdb_link_discovery_confidence_bucket
```

### Graph Topology Metrics
```
themisdb_graph_node_incoming_edges{urn="...", type="..."}
themisdb_graph_node_outgoing_edges{urn="...", type="..."}
themisdb_graph_indegree_bucket
```

### Network Metrics
```
themisdb_remote_executor_bytes_sent_total{shard_id="..."}
themisdb_remote_executor_bytes_received_total{shard_id="..."}
```

## Configuration

### Prometheus Scrape Configuration

Ensure your Prometheus is configured to scrape ThemisDB metrics:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb-hub-shards'
    static_configs:
      - targets:
        - 'themisdb-hub:9090'
    relabel_configs:
      - source_labels: [__address__]
        target_label: shard_id
        replacement: 'hub_001'
  
  - job_name: 'themisdb-worker-shards'
    kubernetes_sd_configs:
      - role: pod
        namespaces:
          names:
            - themisdb
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_label_app]
        action: keep
        regex: themisdb-worker
      - source_labels: [__meta_kubernetes_pod_name]
        target_label: shard_id
```

### ThemisDB Configuration

Enable Prometheus metrics in ThemisDB configuration:

```yaml
# config/monitoring.yaml
monitoring:
  prometheus:
    enabled: true
    port: 9090
    path: /metrics
    
  metrics:
    # Hub-Shard metrics
    hub_shard:
      enabled: true
      track_cross_shard_edges: true
      track_urn_cache: true
    
    # Link discovery metrics
    link_discovery:
      enabled: true
      track_by_method: true
      track_confidence: true
    
    # Graph topology metrics
    graph_topology:
      enabled: true
      track_degree: true
      snapshot_interval_seconds: 60
```

## Alerts

The dashboard can be extended with alerting rules. Example Prometheus alerts:

```yaml
# prometheus-alerts.yml
groups:
  - name: themisdb_shard_network
    interval: 30s
    rules:
      - alert: HighCrossShardLatency
        expr: histogram_quantile(0.95, rate(themisdb_routing_latency_ms_bucket{type="scatter_gather"}[5m])) > 1000
        for: 5m
        annotations:
          summary: "High cross-shard query latency"
          description: "P95 scatter-gather latency is {{ $value }}ms"
      
      - alert: LowURNCacheHitRate
        expr: themisdb_hub_urn_cache_hit_rate < 0.8
        for: 10m
        annotations:
          summary: "Low URN cache hit rate"
          description: "URN cache hit rate is {{ $value | humanizePercentage }}"
      
      - alert: HighInDegreeAnomaly
        expr: (themisdb_graph_node_incoming_edges - themisdb_graph_node_incoming_edges offset 1h) > 100
        for: 5m
        annotations:
          summary: "Unusual spike in entity references"
          description: "Entity {{ $labels.urn }} has {{ $value }} new references in the last hour"
      
      - alert: WorkerShardDown
        expr: count(themisdb_shard_healthy{role="worker"}) < 3
        for: 2m
        annotations:
          summary: "Worker shard count below threshold"
          description: "Only {{ $value }} worker shards are active"
```

## Troubleshooting

### No Data Displayed

1. **Check Prometheus connectivity**:
   ```bash
   curl http://prometheus:9090/api/v1/query?query=up{job="themisdb-hub-shards"}
   ```

2. **Verify metrics are being exposed**:
   ```bash
   curl http://themisdb-hub:9090/metrics | grep themisdb_
   ```

3. **Check Grafana data source**:
   - Navigate to Configuration → Data Sources
   - Test the Prometheus connection

### Metrics Not Updating

1. Check ThemisDB monitoring configuration is enabled
2. Verify Prometheus scrape interval and targets
3. Check for errors in ThemisDB logs:
   ```bash
   kubectl logs -l app=themisdb-hub -c themisdb --tail=100 | grep -i metric
   ```

### Dashboard Panels Empty

1. Adjust the time range (top-right corner)
2. Check if metrics exist in Prometheus:
   ```promql
   count(themisdb_graph_node_incoming_edges)
   ```
3. Verify metric names match your ThemisDB version

## Customization

### Adding Custom Panels

You can extend this dashboard with additional panels:

1. Click **Add Panel** → **Add a new panel**
2. Configure your query using Prometheus expressions
3. Save the dashboard

### Example Custom Queries

**Cross-Shard Edge Growth Rate**:
```promql
rate(themisdb_hub_cross_shard_edges_total[1h])
```

**Shard Load Distribution**:
```promql
sum(rate(themisdb_routing_requests_total[5m])) by (shard_id)
```

**Link Discovery Success Rate**:
```promql
rate(themisdb_link_discovery_links_validated_total[5m]) / 
rate(themisdb_link_discovery_links_found_total[5m])
```

## Support

For issues or questions:
- Documentation: `/docs/sharding/sharding_redundancy.md`
- Metrics Reference: `/docs/observability/`
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues

## Version History

- **v1.0** (2025-12-13): Initial release
  - Hub-shard network monitoring
  - Link discovery tracking
  - Graph topology analysis
  - Cross-shard query performance
