# ThemisDB Monitoring Deployment

This directory contains Kubernetes manifests for deploying ThemisDB monitoring infrastructure.

## Components

### Prometheus
- Configuration: `prometheus/prometheus-config.yaml`
- Scrape targets for all ThemisDB components
- Alert rules for sharding, gossip, and replication

### Grafana Dashboards
- `grafana-dashboards/themisdb-sharding-dashboard.json` - Distributed Sharding Dashboard

## Quick Start

```bash
# Create monitoring namespace
kubectl create namespace themisdb-monitoring

# Deploy Prometheus ConfigMap
kubectl apply -f prometheus/prometheus-config.yaml -n themisdb-monitoring

# Deploy Prometheus (using kube-prometheus-stack or similar)
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace themisdb-monitoring \
  --set prometheus.prometheusSpec.additionalScrapeConfigsSecret.enabled=true

# Import Grafana Dashboard
# 1. Access Grafana UI
# 2. Go to Dashboards > Import
# 3. Upload themisdb-sharding-dashboard.json
```

## Dashboard Panels

### Cluster Overview
- Healthy/Unhealthy Shards
- Average Storage Usage
- Total Documents

### Routing & Query Performance
- Routing Requests per Second
- Query Latency (p50/p95/p99)

### Scatter-Gather & Cross-Shard Joins
- Scatter-Gather Operations
- Cross-Shard Joins by Strategy
- Data Transfer Volume

### Gossip Protocol & P2P
- Known Gossip Peers
- Message Rate (Sent/Received)
- Version Vector Size

### Data Migration & Rebalancing
- Migration Progress (Pending/Completed)
- Transfer Rate

## Alert Rules

| Alert | Severity | Description |
|-------|----------|-------------|
| ShardUnhealthy | Critical | Shard unhealthy for >2min |
| ShardStorageCritical | Warning | Storage >90% |
| HighRoutingErrorRate | Warning | Error rate >5% |
| GossipPeerCountLow | Warning | <2 known peers |
| MigrationStalled | Warning | No progress in 30min |
| HighQueryLatency | Warning | p99 >5s |
| CertificateExpiringSoon | Warning | Expires in <7 days |
| CrossShardJoinSlow | Warning | p95 >30s |

## Metrics Reference

See `docs/observability/PHASE6_MONITORING_COMPLETE.md` for the full list of 44 Prometheus metrics.
