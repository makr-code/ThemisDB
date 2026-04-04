# LoRA Framework Testing and Metrics Guide

## Table of Contents

1. [Overview](#overview)
2. [Unit Testing with Google Test](#unit-testing-with-google-test)
3. [Performance Benchmarking with Google Benchmark](#performance-benchmarking-with-google-benchmark)
4. [Prometheus/Grafana Metrics](#prometheusgrafana-metrics)
5. [Running Tests](#running-tests)
6. [Interpreting Results](#interpreting-results)
7. [Best Practices](#best-practices)

---

## Overview

The LoRA Adapter Framework includes comprehensive testing and monitoring infrastructure:

- **Google Test (GTest)**: Unit and integration tests for all components
- **Google Benchmark (gbenchmark)**: Performance benchmarks for critical paths
- **Prometheus/Grafana**: Production metrics for monitoring and alerting

### Test Coverage

| Component | Unit Tests | Benchmarks | Metrics |
|-----------|-----------|------------|---------|
| Storage Service | ✅ | ✅ | ✅ |
| Adapter Manager | ✅ | ✅ | ✅ |
| Training Service | ✅ | ✅ | ✅ |
| Orchestrator | ✅ | ✅ | ✅ |
| Audit Logger | ✅ | ❌ | ✅ |
| themis_help_lora | ✅ | ❌ | ✅ |

---

## Unit Testing with Google Test

### Test Structure

```cpp
// tests/test_lora_framework.cpp

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_adapter_manager.h"

class LoRAFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test components
        storage_ = std::make_shared<LoRAStorageService>(config);
        manager_ = std::make_shared<LoRAAdapterManager>(config, storage_);
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    std::shared_ptr<LoRAStorageService> storage_;
    std::shared_ptr<LoRAAdapterManager> manager_;
};

TEST_F(LoRAFrameworkTest, AdapterManager_LoadAndUnload) {
    // Test adapter loading
    bool loaded = manager_->loadAdapter("test_adapter");
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(manager_->isLoaded("test_adapter"));
    
    // Test adapter unloading
    bool unloaded = manager_->unloadAdapter("test_adapter");
    EXPECT_TRUE(unloaded);
    EXPECT_FALSE(manager_->isLoaded("test_adapter"));
}
```

### Test Categories

#### 1. Storage Tests
- **Save/Load**: Verify adapter persistence
- **Versioning**: Test version creation and rollback
- **Deletion**: Ensure proper cleanup
- **Encryption**: Validate security features

#### 2. Adapter Manager Tests
- **Lifecycle**: Load, unload, hot-swap
- **Caching**: LRU cache behavior, eviction
- **Concurrency**: Thread-safe operations

#### 3. Training Tests
- **On-the-fly**: Single adapter training
- **Batch**: Multiple adapter training
- **Configuration**: Hyperparameter validation

#### 4. Orchestrator Tests
- **CRUD**: Create, read, update, delete
- **Integration**: Component coordination
- **Health**: Monitoring and diagnostics

#### 5. Integration Tests
- **End-to-end**: Complete workflows
- **Error handling**: Failure scenarios
- **Performance**: Timing constraints

### Running Unit Tests

```bash
# Build tests
cd build
cmake -DBUILD_TESTING=ON ..
make test_lora_framework

# Run all tests
./tests/test_lora_framework

# Run specific test suite
./tests/test_lora_framework --gtest_filter=*StorageService*

# Run with verbose output
./tests/test_lora_framework --gtest_output=xml:test_results.xml

# Run with specific seed for reproducibility
./tests/test_lora_framework --gtest_random_seed=12345
```

### Test Output

```
[==========] Running 35 tests from 7 test suites.
[----------] 5 tests from LoRAFrameworkTest/StorageService
[ RUN      ] LoRAFrameworkTest.StorageService_SaveAndLoadAdapter
[       OK ] LoRAFrameworkTest.StorageService_SaveAndLoadAdapter (15 ms)
[ RUN      ] LoRAFrameworkTest.StorageService_VersionManagement
[       OK ] LoRAFrameworkTest.StorageService_VersionManagement (8 ms)
...
[==========] 35 tests from 7 test suites ran. (1250 ms total)
[  PASSED  ] 35 tests.
```

---

## Performance Benchmarking with Google Benchmark

### Benchmark Structure

```cpp
// benchmarks/bench_lora_framework.cpp

#include <benchmark/benchmark.h>
#include "llm/lora_framework/lora_adapter_manager.h"

static void BM_Manager_LoadAdapter(benchmark::State& state) {
    // Setup
    auto storage = std::make_shared<LoRAStorageService>(config);
    LoRAAdapterManager manager(config, storage);
    
    // Benchmark loop
    for (auto _ : state) {
        benchmark::DoNotOptimize(manager.loadAdapter("test_adapter"));
    }
    
    // Report metrics
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_LoadAdapter);
```

### Benchmark Categories

#### 1. Storage Benchmarks
- `BM_Storage_SaveAdapter_Rank8_1K`: Save small adapter (1K weights)
- `BM_Storage_SaveAdapter_Rank16_1M`: Save large adapter (1M weights)
- `BM_Storage_LoadAdapter`: Load adapter from storage
- `BM_Storage_WithEncryption`: Save with encryption enabled

#### 2. Manager Benchmarks
- `BM_Manager_LoadAdapter`: Adapter loading performance
- `BM_Manager_HotSwap`: Hot-swap timing
- `BM_Manager_CacheHitRate`: Cache performance analysis

#### 3. Training Benchmarks
- `BM_Training_OnTheFly_SmallDataset`: Quick training (10 samples)
- `BM_Training_Batch_LargeDataset`: Batch training (100+ samples)

#### 4. Orchestrator Benchmarks
- `BM_Orchestrator_CreateAdapter`: Adapter creation overhead
- `BM_Orchestrator_CRUD_Operations`: Mixed CRUD performance

#### 5. Concurrent Benchmarks
- `BM_Concurrent_AdapterLoading`: Multi-threaded loading
- `BM_Memory_AdapterFootprint`: Memory usage tracking

### Running Benchmarks

```bash
# Build benchmarks
cd build
cmake -DBUILD_BENCHMARKS=ON ..
make bench_lora_framework

# Run all benchmarks
./benchmarks/bench_lora_framework

# Run specific benchmark
./benchmarks/bench_lora_framework --benchmark_filter=Manager_HotSwap

# Run with custom iterations
./benchmarks/bench_lora_framework --benchmark_min_time=5.0

# Output to JSON
./benchmarks/bench_lora_framework --benchmark_out=results.json --benchmark_out_format=json

# Compare results
./benchmarks/bench_lora_framework --benchmark_out=baseline.json
# ... make changes ...
./benchmarks/bench_lora_framework --benchmark_out=current.json
compare.py benchmarks baseline.json current.json
```

### Benchmark Output

```
Run on (16 X 3600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 256 KiB (x8)
  L3 Unified 16384 KiB (x2)
Load Average: 1.23, 1.45, 1.67
----------------------------------------------------------------------
Benchmark                              Time           CPU Iterations
----------------------------------------------------------------------
BM_Storage_SaveAdapter_Rank8_1K      2.15 ms       2.14 ms        327
BM_Manager_LoadAdapter/1             0.85 ms       0.84 ms        823
BM_Manager_LoadAdapter/10            0.92 ms       0.91 ms        765
BM_Manager_HotSwap                   3.21 ms       3.19 ms        219
BM_Manager_CacheHitRate/5            1.12 ms       1.11 ms        628
  CacheHitRate=0.842 Evictions=23
```

### Performance Targets

| Metric | Target | Measured | Status |
|--------|--------|----------|--------|
| Adapter load | < 500ms | 450ms | ✅ |
| Hot-swap | < 10ms | 3.2ms | ✅ |
| Cache hit rate | > 80% | 84.2% | ✅ |
| Training (on-the-fly) | < 30min | 18min | ✅ |
| Inference overhead | < 100ms | 45ms | ✅ |

---

## Prometheus/Grafana Metrics

### Metrics Categories

#### 1. Adapter Lifecycle Metrics

```prometheus
# Adapter load duration (histogram)
themis_lora_adapter_load_duration_seconds{adapter_id="themis_help_lora"} 0.45

# Adapter loads total (counter)
themis_lora_adapter_loads_total{adapter_id="themis_help_lora"} 127

# Adapter switch duration (histogram)
themis_lora_adapter_switch_duration_seconds{from="adapter1",to="adapter2"} 0.003
```

#### 2. Cache Metrics

```prometheus
# Cache hit rate
themis_lora_cache_hits_total{adapter_id="themis_help_lora"} 845
themis_lora_cache_misses_total{adapter_id="themis_help_lora"} 155
themis_lora_cache_evictions_total{adapter_id="themis_help_lora"} 23

# Cache size
themis_lora_cache_size 5
themis_lora_cache_memory_bytes 104857600  # 100 MB
```

#### 3. Training Metrics

```prometheus
# Training operations
themis_lora_training_starts_total{adapter_id="themis_help_lora",mode="on_the_fly"} 12
themis_lora_training_completes_total{adapter_id="themis_help_lora",mode="on_the_fly",status="success"} 11

# Training duration (histogram)
themis_lora_training_duration_seconds{adapter_id="themis_help_lora",mode="on_the_fly"} 1080  # 18 minutes

# Training quality
themis_lora_training_loss{adapter_id="themis_help_lora"} 0.12
themis_lora_training_accuracy{adapter_id="themis_help_lora"} 0.945
```

#### 4. Storage Metrics

```prometheus
# Storage operations
themis_lora_storage_reads_total{adapter_id="themis_help_lora"} 1523
themis_lora_storage_writes_total{adapter_id="themis_help_lora"} 87

# Storage latency (histogram)
themis_lora_storage_read_duration_seconds{adapter_id="themis_help_lora"} 0.025
themis_lora_storage_write_duration_seconds{adapter_id="themis_help_lora"} 0.035

# Storage throughput (summary)
themis_lora_storage_read_bytes 1048576000  # 1 GB
themis_lora_storage_write_bytes 52428800   # 50 MB
```

#### 5. Inference Metrics

```prometheus
# Inference operations
themis_lora_inference_total{adapter_id="themis_help_lora"} 5432
themis_lora_inference_errors_total{adapter_id="themis_help_lora",error="timeout"} 3

# Inference latency (histogram)
themis_lora_inference_duration_seconds{adapter_id="themis_help_lora"} 0.045

# Token processing
themis_lora_inference_tokens_total{adapter_id="themis_help_lora",type="input"} 123456
themis_lora_inference_tokens_total{adapter_id="themis_help_lora",type="output"} 234567

# Queue size
themis_lora_inference_queue_size 12
```

#### 6. Resource Metrics

```prometheus
# Memory usage
themis_lora_memory_usage_bytes{category="adapters"} 524288000  # 500 MB
themis_lora_memory_usage_bytes{category="cache"} 104857600    # 100 MB

# GPU VRAM usage
themis_lora_gpu_vram_bytes{adapter_id="themis_help_lora"} 33554432  # 32 MB

# CPU usage
themis_lora_cpu_usage_percent 15.7
```

### Enabling Metrics

```cpp
#include "llm/lora_framework/lora_metrics.h"

// Create Prometheus registry
auto registry = std::make_shared<prometheus::Registry>();

// Create metrics collector
LoRAMetricsCollector::Config metrics_config;
metrics_config.namespace_prefix = "themis_lora";
metrics_config.enable_detailed_metrics = true;
auto metrics = std::make_shared<LoRAMetricsCollector>(registry, metrics_config);

// Use with components
LoRAAdapterManager manager(config, storage, metrics);
LoRAOrchestrator orchestrator(storage, manager, training, audit, metrics);

// Expose metrics endpoint
// GET /metrics returns Prometheus format
```

### Grafana Dashboards

ThemisDB includes three pre-configured Grafana dashboards for comprehensive LoRA framework monitoring:

#### Dashboard 1: LoRA Framework Overview
**File**: `config/grafana/dashboards/lora-framework-overview.json`

Provides high-level system monitoring across all framework components:

- **Adapter Lifecycle Metrics**
  - P95 load duration with thresholds (yellow: 300ms, red: 500ms)
  - Load rate per adapter
  - Active adapters count (gauge)
  - Load error rate (gauge)
  - Hot-swap latency P95 (gauge)
  - Total adapters managed (stat panel)

- **Cache Performance**
  - Hit rate percentage (gauge with 80% target)
  - Memory usage with thresholds
  - Cache size (number of adapters)
  - Eviction rate trends

- **Storage I/O**
  - Read/write latency percentiles (P50, P95, P99)
  - Throughput (bytes/sec) for reads and writes

- **Inference Performance**
  - Request rate per adapter
  - Latency percentiles with thresholds
  - Queue size monitoring
  - Error rate tracking

- **Resource Utilization**
  - Memory usage by category (stacked)
  - GPU VRAM usage per adapter (stacked)
  - CPU usage percentage (gauge)

**Variables**: Adapter ID filter (multi-select)

#### Dashboard 2: LoRA Training & Performance
**File**: `config/grafana/dashboards/lora-training-performance.json`

Focused on training operations and model quality:

- **Training Operations**
  - P95 training duration by adapter and mode
  - Training throughput (samples/sec)
  - Training starts/completions counters
  - Success rate gauge (target: >95%)
  - Error rate monitoring

- **Model Quality Metrics**
  - Loss curves over time (smooth interpolation)
  - Accuracy curves with thresholds (yellow: 80%, green: 90%)
  - Current loss by adapter (bar gauge)
  - Current accuracy by adapter (bar gauge)

- **Performance Comparison**
  - Table comparing accuracy, loss, duration, and throughput across adapters
  - Color-coded cells for quick identification of issues

- **Training Bottlenecks**
  - Latency distribution heatmap
  - Operations by mode (pie chart)
  - Throughput comparison (bar chart)

**Variables**: Adapter ID filter, Training Mode filter

#### Dashboard 3: LoRA Operations & Audit
**File**: `config/grafana/dashboards/lora-operations-audit.json`

Operations monitoring and compliance tracking:

- **Orchestrator Operations**
  - P95 operation duration by type
  - Operation rate with stacking
  
- **CRUD Operations**
  - Success rates for read/write/delete (line graph with thresholds)
  - Operation distribution (donut chart)
  - Individual operation counters (stat panels)

- **Audit Logging**
  - Write duration percentiles (P50, P95, P99)
  - Query performance tracking
  - Total audit log entries
  - Audit log size monitoring

- **Error Tracking**
  - Error rate by type (adapter load, storage, inference)
  - Error type distribution (pie chart)
  - Top 10 errors by adapter (sortable table)

- **System Health**
  - Overall success rate gauge
  - Version count by adapter
  - Version rollback operations

**Variables**: Adapter ID filter, Operation type filter

### Importing Grafana Dashboards

#### Method 1: Grafana UI Import

1. **Open Grafana** (default: http://localhost:3000)
2. **Navigate to Dashboards** → **Import**
3. **Upload JSON file** or paste JSON content:
   - `config/grafana/dashboards/lora-framework-overview.json`
   - `config/grafana/dashboards/lora-training-performance.json`
   - `config/grafana/dashboards/lora-operations-audit.json`
4. **Select Prometheus datasource** from dropdown
5. **Click Import**

#### Method 2: Provisioning (Recommended for Production)

Create a provisioning configuration file:

```yaml
# /etc/grafana/provisioning/dashboards/lora-dashboards.yml
apiVersion: 1

providers:
  - name: 'LoRA Framework'
    orgId: 1
    folder: 'LoRA Monitoring'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: true
    options:
      path: /path/to/ThemisDB/config/grafana/dashboards
```

Then restart Grafana:
```bash
sudo systemctl restart grafana-server
```

#### Method 3: Docker Compose

```yaml
# docker-compose.yml
version: '3.8'

services:
  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - ./config/grafana/dashboards:/etc/grafana/provisioning/dashboards/lora:ro
      - ./grafana/provisioning:/etc/grafana/provisioning:ro
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
      - GF_USERS_ALLOW_SIGN_UP=false
```

### Configuring Prometheus Data Source

Before importing dashboards, configure Prometheus as a data source:

1. **Navigate to Configuration** → **Data Sources**
2. **Add data source** → **Prometheus**
3. **Configure settings**:
   - Name: `Prometheus`
   - URL: `http://localhost:9090` (or your Prometheus URL)
   - Access: `Server (default)`
   - Scrape interval: `15s`
4. **Save & Test**

### Variable Filters Usage

All dashboards include template variables for dynamic filtering:

#### Adapter ID Filter
- **Type**: Multi-select dropdown
- **Purpose**: Filter metrics by specific adapter(s)
- **Usage**: Select one or multiple adapters, or "All" for aggregate view
- **Query**: `label_values(themis_lora_adapter_loads_total, adapter_id)`

#### Training Mode Filter (Training Dashboard)
- **Type**: Multi-select dropdown
- **Values**: `on_the_fly`, `batch`, etc.
- **Purpose**: Filter training metrics by training mode

#### Operation Filter (Operations Dashboard)
- **Type**: Multi-select dropdown
- **Values**: `create`, `read`, `update`, `delete`, etc.
- **Purpose**: Filter orchestrator operations

**Tips**:
- Use `Ctrl+Click` (Windows/Linux) or `Cmd+Click` (Mac) for multi-select
- Variables are persistent across dashboard navigation
- Clear filters by selecting "All"

### Example Grafana Queries

```promql
# Cache hit rate (%)
100 * (
  rate(themis_lora_cache_hits_total[5m]) /
  (rate(themis_lora_cache_hits_total[5m]) + rate(themis_lora_cache_misses_total[5m]))
)

# Adapter load p95 latency
histogram_quantile(0.95, 
  rate(themis_lora_adapter_load_duration_seconds_bucket[5m])
)

# Inference requests per second
rate(themis_lora_inference_total[1m])

# Training success rate (%)
100 * (
  rate(themis_lora_training_completes_total{status="success"}[5m]) /
  rate(themis_lora_training_starts_total[5m])
)
```

### Alerting Rules

Alert rules integrate with Grafana dashboards to provide visual indicators:

```yaml
groups:
- name: lora_alerts
  rules:
  - alert: LoRAAdapterLoadTimeout
    expr: themis_lora_adapter_load_duration_seconds > 0.5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "LoRA adapter loading is slow"
      
  - alert: LoRACacheHitRateLow
    expr: |
      100 * (
        rate(themis_lora_cache_hits_total[5m]) /
        (rate(themis_lora_cache_hits_total[5m]) + rate(themis_lora_cache_misses_total[5m]))
      ) < 80
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "LoRA cache hit rate below 80%"
      
  - alert: LoRATrainingFailed
    expr: rate(themis_lora_training_completes_total{status="error"}[5m]) > 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "LoRA training failures detected"
```

## Customization

### Modifying Thresholds

To adjust alert thresholds in panels:

1. **Edit Panel** → Click panel title → **Edit**
2. **Navigate to** → **Thresholds** section
3. **Modify values**:
   - Green (healthy): Base value
   - Yellow (warning): Warning threshold
   - Red (critical): Critical threshold
4. **Save dashboard**

Example threshold configurations:
- **Cache Hit Rate**: Red < 0.6 (60%), Yellow 0.6-0.8, Green > 0.8 (80%)
- **Load Duration**: Green < 0.3s (300ms), Yellow 0.3-0.5s, Red > 0.5s (500ms)
- **Error Rate**: Green < 1%, Yellow 1-5%, Red > 5%
- **Training Accuracy**: Red < 0.80 (80%), Yellow 0.80-0.90, Green > 0.90 (90%)
- **Audit Log Size**: Green < 1GB (1073741824 bytes), Yellow 1-5GB, Red > 5GB (5368709120 bytes)

**Note**: These thresholds are examples and should be adjusted based on your specific Service Level Objectives (SLOs) and workload characteristics.

#### Adding Custom Panels

1. **Add Panel** → **Add new panel**
2. **Select Visualization Type**:
   - Time series: Trends over time
   - Stat: Single value display
   - Gauge: Percentage/threshold visualization
   - Table: Detailed comparisons
   - Bar gauge: Category comparisons
3. **Configure Query**:
   ```promql
   # Example: Custom adapter comparison
   histogram_quantile(0.95, 
     sum by (adapter_id, le) (
       rate(themis_lora_inference_duration_seconds_bucket[5m])
     )
   )
   ```
4. **Set Legend Format**: `{{adapter_id}} - P95 Latency`
5. **Apply transformations** if needed

#### Customizing Time Ranges

Default time ranges:
- **Overview Dashboard**: Last 1 hour
- **Training Dashboard**: Last 6 hours
- **Operations Dashboard**: Last 6 hours

To modify:
1. **Dashboard Settings** → **Time options**
2. **Set default**: `from: now-6h, to: now`
3. **Configure refresh intervals**: `30s`, `1m`, `5m`, `15m`, `30m`, `1h`

#### Adding Annotations

Annotations mark important events on time series:

1. **Dashboard Settings** → **Annotations**
2. **Add annotation**:
   - **Name**: Deployment
   - **Data source**: Prometheus
   - **Query**: `changes(themis_lora_total_adapters[1m]) > 0`
3. **Style**: Line, Region, or Alert
4. **Save**

### Alert Rule Integration

#### Configuring Alert Notifications

1. **Create notification channel**:
   - Alerting → Notification channels → New channel
   - Type: Email, Slack, PagerDuty, etc.
   
2. **Link to dashboard**:
   ```yaml
   # prometheus/alerts.yml
   - alert: LoRAHighLatency
     expr: histogram_quantile(0.95, rate(themis_lora_inference_duration_seconds_bucket[5m])) > 0.2
     annotations:
       dashboard: "http://grafana:3000/d/lora-framework-overview"
       panel: "inference-latency"
   ```

3. **Visual indicators**:
   - Panels show alert state colors
   - Annotations overlay on time series
   - Alert icons in dashboard list

### Troubleshooting Common Issues

#### Issue 1: No Data Displayed

**Symptoms**: Empty panels, "No data" messages

**Solutions**:
1. **Check Prometheus connection**:
   ```bash
   curl http://localhost:9090/api/v1/query?query=up
   ```
2. **Verify metrics are being collected**:
   ```bash
   curl http://localhost:9090/api/v1/label/__name__/values | grep themis_lora
   ```
3. **Check ThemisDB metrics endpoint**:
   ```bash
   curl http://localhost:9091/metrics | grep themis_lora
   ```
4. **Verify Prometheus scrape config**:
   ```yaml
   scrape_configs:
     - job_name: 'themisdb-lora'
       static_configs:
         - targets: ['localhost:9091']
   ```

#### Issue 2: Incorrect Time Range

**Symptoms**: Historical data not showing, gaps in graphs

**Solutions**:
1. **Adjust time range** in top-right corner
2. **Check Prometheus retention**:
   ```bash
   # prometheus.yml
   global:
     retention: 30d  # Increase if needed
   ```
3. **Verify scrape interval alignment**:
   - Dashboard queries use `[5m]` rate
   - Scrape interval should be ≤ 15s

#### Issue 3: Variables Not Populating

**Symptoms**: Empty dropdown for Adapter ID or other filters

**Solutions**:
1. **Check variable query**:
   ```promql
   label_values(themis_lora_adapter_loads_total, adapter_id)
   ```
2. **Verify metric has labels**:
   ```bash
   curl -g 'http://localhost:9090/api/v1/series?match[]=themis_lora_adapter_loads_total'
   ```
3. **Refresh dashboard variables**: Dashboard Settings → Variables → Refresh

#### Issue 4: Dashboard Import Fails

**Symptoms**: Error during JSON import

**Solutions**:
1. **Validate JSON syntax**:
   ```bash
   jq '.' config/grafana/dashboards/lora-framework-overview.json
   ```
2. **Check Grafana version compatibility**: Dashboards require Grafana 8.0+
3. **Remove UID conflicts**: Set `"id": null` in JSON
4. **Manual datasource selection**: Some Grafana versions require manual selection

#### Issue 5: High Cardinality Warnings

**Symptoms**: Performance issues, slow dashboard loading

**Solutions**:
1. **Limit label values**:
   ```promql
   # Instead of all adapters
   topk(10, themis_lora_adapter_loads_total)
   ```
2. **Use recording rules**:
   ```yaml
   # prometheus/rules.yml
   groups:
     - name: lora_recording_rules
       rules:
         - record: lora:cache_hit_rate:5m
           expr: rate(themis_lora_cache_hits_total[5m]) / (rate(themis_lora_cache_hits_total[5m]) + rate(themis_lora_cache_misses_total[5m]))
   ```
3. **Increase query timeout**: Grafana → Data Sources → Prometheus → Query timeout

#### Issue 6: Missing Panels or Broken Layout

**Symptoms**: Panels overlapping, missing visualizations

**Solutions**:
1. **Reset dashboard layout**: Dashboard Settings → JSON Model → Restore defaults
2. **Clear browser cache**: Shift+F5 or Ctrl+Shift+R
3. **Check panel IDs**: Ensure unique IDs in JSON
4. **Reimport dashboard**: Delete and reimport from JSON

### Dashboard Best Practices

1. **Organize by Use Case**:
   - Overview: Real-time health monitoring
   - Training: Development and optimization
   - Operations: Production troubleshooting

2. **Use Consistent Time Windows**:
   - Short-term: 1h for real-time monitoring
   - Medium-term: 6h for trend analysis
   - Long-term: 7d for capacity planning

3. **Set Meaningful Thresholds**:
   - Based on SLOs (Service Level Objectives)
   - Aligned with alert rules
   - Adjusted for your workload

4. **Document Customizations**:
   - Add panel descriptions
   - Include query explanations
   - Note threshold rationale

5. **Version Control Dashboards**:
   - Export JSON after changes
   - Store in Git repository
   - Use provisioning for deployment

6. **Regular Review**:
   - Weekly: Check alert thresholds
   - Monthly: Update based on performance trends
   - Quarterly: Add new metrics as features evolve

---

## Running Tests

### Prerequisites

```bash
# Install dependencies
sudo apt-get install -y \
    libgtest-dev \
    libbenchmark-dev \
    libprometheus-cpp-dev

# Or via vcpkg
vcpkg install gtest benchmark prometheus-cpp
```

### Build Configuration

```cmake
# CMakeLists.txt

# Enable testing
option(BUILD_TESTING "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(ENABLE_METRICS "Enable Prometheus metrics" ON)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    enable_testing()
    
    add_executable(test_lora_framework tests/test_lora_framework.cpp)
    target_link_libraries(test_lora_framework
        GTest::GTest
        GTest::Main
        lora_framework
    )
    
    add_test(NAME test_lora_framework COMMAND test_lora_framework)
endif()

if(BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
    
    add_executable(bench_lora_framework benchmarks/bench_lora_framework.cpp)
    target_link_libraries(bench_lora_framework
        benchmark::benchmark
        lora_framework
    )
endif()

if(ENABLE_METRICS)
    find_package(prometheus-cpp REQUIRED)
    target_link_libraries(lora_framework
        prometheus-cpp::core
        prometheus-cpp::pull
    )
endif()
```

### Quick Start

```bash
# 1. Build with tests enabled
mkdir build && cd build
cmake -DBUILD_TESTING=ON -DBUILD_BENCHMARKS=ON ..
make -j$(nproc)

# 2. Run unit tests
ctest --output-on-failure

# Or run directly
./tests/test_lora_framework

# 3. Run benchmarks
./benchmarks/bench_lora_framework

# 4. Start Prometheus metrics endpoint
./bin/themisdb --enable-metrics --metrics-port=9090

# 5. Query metrics
curl http://localhost:9090/metrics | grep themis_lora
```

### Continuous Integration

```yaml
# .github/workflows/tests.yml

name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y libgtest-dev libbenchmark-dev
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake -DBUILD_TESTING=ON ..
        make -j$(nproc)
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
    
    - name: Upload test results
      if: always()
      uses: actions/upload-artifact@v2
      with:
        name: test-results
        path: build/test_results.xml
```

---

## Interpreting Results

### Unit Test Results

**Success criteria:**
- All tests pass ✅
- No memory leaks (run with valgrind)
- Code coverage > 80%

**Common failures:**
- Storage errors: Check file permissions
- Timing issues: Increase timeouts
- Concurrency: Check for race conditions

### Benchmark Results

**Performance regressions:**
- Load time > 500ms: Check storage backend
- Hot-swap > 10ms: Review cache implementation
- Cache hit rate < 80%: Tune cache size

**Comparing results:**
```bash
# Generate baseline
./bench_lora_framework --benchmark_out=baseline.json --benchmark_out_format=json

# After changes
./bench_lora_framework --benchmark_out=current.json --benchmark_out_format=json

# Compare
compare.py benchmarks baseline.json current.json
```

### Metrics Analysis

**Key performance indicators:**
1. **Adapter load time**: p95 < 500ms
2. **Cache hit rate**: > 80%
3. **Training success rate**: > 95%
4. **Inference latency**: p99 < 200ms
5. **Storage throughput**: > 100 MB/s

**Troubleshooting:**
- High load times → Check storage I/O
- Low cache hit rate → Increase cache size
- Training failures → Review hyperparameters
- High inference latency → Check GPU utilization

---

## Best Practices

### Testing

1. **Test isolation**: Each test should be independent
2. **Mock external dependencies**: Use stubs for LLM calls
3. **Deterministic**: Use fixed seeds for reproducibility
4. **Fast**: Unit tests should complete in < 1 minute
5. **Comprehensive**: Cover happy path and error cases

### Benchmarking

1. **Warm-up**: Run benchmarks after warm-up period
2. **Stable environment**: Disable CPU frequency scaling
3. **Multiple runs**: Average over 3+ runs
4. **Realistic data**: Use production-like test data
5. **Document baseline**: Track performance over time

### Metrics

1. **High cardinality**: Avoid too many label combinations
2. **Aggregation**: Use histograms for latency, counters for totals
3. **Retention**: Keep metrics for 30+ days
4. **Alerting**: Set up alerts for critical metrics
5. **Dashboards**: Create actionable dashboards

---

## Further Reading

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Prometheus Best Practices](https://prometheus.io/docs/practices/naming/)
- [Grafana Dashboard Design](https://grafana.com/docs/grafana/latest/best-practices/)
- [ThemisDB Testing Guide](../docs/TESTING_AND_BENCHMARKING_GUIDE.md)
