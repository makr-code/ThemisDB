# ThemisDB Monitoring Deployment

This directory contains Kubernetes manifests and documentation for deploying ThemisDB monitoring infrastructure with full Prometheus metrics integration (Phase 6).

## Phase 6: Complete Prometheus Integration

**Status:** ✅ COMPLETE (December 2025)

ThemisDB now includes comprehensive Prometheus metrics for all sharding components:
- Shard Router (routing requests, latency, errors)
- Data Migrator (migration progress, duration)
- Gossip Protocol (peer discovery, message exchange)
- Cross-Shard Joins (join strategies, performance)
- Health Checks (certificate expiry, storage, network)
- Cloud Agent (multi-DC operations, cross-DC requests)

### Quick Start

1. **Enable Metrics in Your Code**

```cpp
#include "sharding/prometheus_metrics.h"
#include "sharding/metrics_registry.h"
#include "sharding/shard_router.h"

// Create metrics instance
using namespace themis::sharding;

PrometheusMetrics::Config config;
config.enable_histograms = true;
auto metrics = std::make_shared<PrometheusMetrics>(config);

// Register globally for HTTP /metrics endpoint
ShardingMetricsRegistry::instance().registerMetrics(metrics);

// Pass to sharding components
auto router = std::make_shared<ShardRouter>(
    resolver, executor, router_config, metrics
);
```

2. **Access Metrics**

```bash
# Get all metrics including sharding
curl http://localhost:8080/metrics

# Filter for sharding metrics only
curl http://localhost:8080/metrics | grep themis_
```

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
