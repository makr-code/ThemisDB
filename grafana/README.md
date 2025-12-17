# ThemisDB Grafana Dashboards

This directory contains thematic Grafana dashboard definitions (JSON files) for monitoring ThemisDB metrics through Prometheus.

## Overview

These dashboards provide comprehensive monitoring and visualization of ThemisDB's various subsystems and operations. All dashboards are designed to work with Prometheus as the data source and use the metrics exposed by ThemisDB's `/metrics` endpoint.

## Available Dashboards

### 1. System Overview (`system-overview.json`)
**UID:** `themisdb-system-overview`

Monitors core system resources and health metrics:
- **CPU Usage**: Current and historical CPU utilization with thresholds
- **Memory Usage**: Memory consumption tracking
- **Disk I/O Operations**: Read and write operations per second

**Best for:** Infrastructure monitoring, capacity planning, and system health checks.

### 2. Time Series Store (`timeseries-store.json`)
**UID:** `themisdb-timeseries-store`

Tracks TSStore operations and performance:
- **Write Rate**: Time series data ingestion rates by metric
- **Query Rate**: Query operations per second
- **Write/Query Latency**: Percentile-based latency metrics (p50, p95, p99)
- **Aggregations**: Aggregation operation rates
- **Compression**: Compression ratios by type
- **Batch Sizes**: Write batch size distribution

**Best for:** Optimizing time series ingestion, monitoring query performance, and storage efficiency.

### 3. Query Performance (`query-performance.json`)
**UID:** `themisdb-query-performance`

Analyzes query engine performance and efficiency:
- **Query Rate by Type**: Breakdown of different query types
- **Query Latency Percentiles**: Performance distribution (p50, p95, p99)
- **Index Scans**: Index usage and efficiency by type
- **Full Table Scans**: Warning indicator for inefficient queries
- **Keys Scanned**: Volume of data processed
- **Result Counts**: Query result sizes

**Best for:** Query optimization, identifying slow queries, and index tuning.

### 4. Cache Performance (`cache-performance.json`)
**UID:** `themisdb-cache-performance`

Monitors caching subsystem effectiveness:
- **Overall Cache Hit Rate**: Aggregate cache effectiveness gauge
- **Hit Rate by Type**: Per-cache-type performance
- **Cache Hits/Misses**: Operation rates over time
- **Cache Evictions**: Memory pressure indicators
- **Cumulative Operations**: Historical cache usage patterns

**Best for:** Tuning cache sizes, identifying cache optimization opportunities.

### 5. Sharding & Distribution (`sharding-distribution.json`)
**UID:** `themisdb-sharding-distribution`

Tracks distributed system health and operations:
- **Cluster Size**: Number of active shards
- **Virtual Nodes**: Consistent hashing configuration
- **Topology Changes**: Cluster reconfiguration events
- **Rebalance Progress**: Data migration status
- **Shard Request Rate**: Per-shard operation distribution
- **Shard Latency**: Cross-shard operation performance
- **Routing Distribution**: Local vs. remote vs. scatter-gather routing
- **Migration Progress**: Records migrated during rebalancing

**Best for:** Distributed system monitoring, load balancing, and cluster health.

### 6. Content Processing (`content-processing.json`)
**UID:** `themisdb-content-processing`

Monitors content import and processing pipeline:
- **Import Rate by MIME Type**: Content ingestion breakdown
- **Import Throughput**: Bytes processed per second
- **Chunk Creation**: Document chunking rates
- **Embedding Generation**: Vector embedding creation performance
- **Embedding Latency**: Processing time distribution
- **Processor Invocations**: Usage by processor type (PDF, Office, Video, etc.)
- **Processor Duration**: Processing time per content type

**Best for:** Content pipeline monitoring, RAG system optimization, and processing bottleneck identification.

### 7. Security & Authentication (`security-authentication.json`)
**UID:** `themisdb-security-auth`

Tracks security-related operations and events:
- **Successful/Failed Auth Attempts**: Authentication success metrics
- **Authentication Success Rate**: Security posture indicator
- **Auth Attempt Rate**: Login activity over time
- **Policy Evaluations**: Authorization decision rates
- **Policy Evaluation Latency**: Authorization performance
- **Encryption Operations**: Cryptographic operation rates by type
- **Encryption Latency**: Encryption/decryption performance

**Best for:** Security monitoring, detecting authentication anomalies, and access control analysis.

### 8. Gossip Protocol & Network (`gossip-network.json`)
**UID:** `themisdb-gossip-network`

Monitors peer-to-peer communication and distributed coordination:
- **Active Gossip Peers**: Cluster membership size
- **Failed Peer Connections**: Network health indicators
- **Gossip Message Rate**: Inter-node communication by type
- **Gossip Round-Trip Latency**: Network performance metrics
- **Cross-Shard Join Rate**: Distributed query operations
- **Cross-Shard Join Duration**: Join performance by strategy
- **Cloud Agent Operations**: Multi-datacenter operations
- **Datacenter Latency**: Geographic distribution performance
- **Cross-DC Requests**: Inter-datacenter traffic patterns

**Best for:** Network debugging, distributed query optimization, and multi-datacenter deployment monitoring.

## Installation

### Option 1: Manual Import
1. Open Grafana web interface
2. Navigate to **Dashboards** → **Import**
3. Click **Upload JSON file**
4. Select one of the dashboard JSON files from this directory
5. Configure the Prometheus datasource
6. Click **Import**

### Option 2: Provisioning
Add to your Grafana provisioning configuration:

```yaml
apiVersion: 1

providers:
  - name: 'ThemisDB'
    orgId: 1
    folder: 'ThemisDB'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: true
    options:
      path: /path/to/ThemisDB/grafana
```

## Prerequisites

- **Grafana**: Version 9.0 or higher (tested with schema version 38)
- **Prometheus**: Configured to scrape ThemisDB metrics endpoint
- **ThemisDB**: Running with metrics collection enabled

## Prometheus Configuration

Ensure your Prometheus instance is configured to scrape ThemisDB metrics:

```yaml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:8080']  # Adjust to your ThemisDB host:port
    metrics_path: '/metrics'
    scrape_interval: 15s
```

## Dashboard Variables

All dashboards include a `DS_PROMETHEUS` variable that allows you to select the Prometheus datasource. This variable is automatically populated with available Prometheus datasources in your Grafana instance.

## Customization

Each dashboard is fully editable and can be customized to your specific needs:
- Adjust time ranges and refresh intervals
- Modify alert thresholds
- Add or remove panels
- Change visualization types
- Apply filters and transformations

## Metrics Reference

### Metric Naming Convention
ThemisDB follows Prometheus naming best practices:
- **Counters**: `*_total` suffix (monotonically increasing)
- **Gauges**: Current values that can increase or decrease
- **Histograms**: `*_ms` suffix with quantile labels for latency metrics

### Key Metric Families
- `tsstore_*`: Time series store operations
- `cache_*`: Cache subsystem metrics
- `queries_*`, `index_*`, `full_scans_*`: Query engine metrics
- `shard_*`, `themis_*`: Sharding and distributed system metrics
- `content_*`, `chunks_*`, `embeddings_*`: Content processing metrics
- `auth_*`, `policy_*`, `encryption_*`: Security metrics
- `themis_gossip_*`, `themis_cross_*`: Network and gossip protocol metrics

## Troubleshooting

### No Data Displayed
1. Verify Prometheus datasource is configured correctly
2. Check that ThemisDB is exposing metrics at `/metrics`
3. Ensure Prometheus is successfully scraping ThemisDB
4. Verify the time range matches when data was generated

### Missing Metrics
Some metrics may only appear when specific features are used:
- Content processing metrics require active content imports
- Sharding metrics require a multi-node deployment
- Gossip metrics require cluster mode operation

## Tags

Each dashboard is tagged for easy discovery:
- `themisdb`: Common tag for all ThemisDB dashboards
- Specific tags: `system`, `timeseries`, `queries`, `cache`, `sharding`, `content`, `security`, `gossip`, `network`

## Version

Dashboard Version: 1.0
Schema Version: 38
Compatible with: Grafana 9.0+

## License

These dashboards are part of the ThemisDB project and are distributed under the same license as the main ThemisDB project. See the [LICENSE](../LICENSE) file in the root directory for full license details.

## Contributing

To contribute improvements to these dashboards:
1. Make changes to the JSON files
2. Test with a live ThemisDB instance
3. Validate JSON syntax: `python3 -m json.tool dashboard.json`
4. Submit a pull request with a description of changes

## Support

For issues or questions about these dashboards, please refer to the main ThemisDB documentation or open an issue in the repository.
