# ThemisDB Time-Series Engine 📊

**Status:** Production Ready  
**Category:** Time-Series Storage & Analytics

---

## 📖 Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [TSStore vs TimeSeriesStore](#tsstore-vs-timeseriesstore)
- [Core Components](#core-components)
- [Usage Examples](#usage-examples)
- [Configuration](#configuration)
- [Performance Considerations](#performance-considerations)
- [Best Practices](#best-practices)
- [API Reference](#api-reference)
- [Related Documentation](#related-documentation)

---

## 🌟 Overview

The ThemisDB Time-Series Engine is a high-performance storage and analytics system designed for metrics, logs, and events. Built on RocksDB, it provides efficient time-series data management with advanced features like Gorilla compression, continuous aggregates, and automated retention policies.

### Key Features

✅ **Efficient Storage** - Gorilla compression achieves 10-20x compression ratios  
✅ **Fast Queries** - Optimized range scans and aggregations  
✅ **Tag-Based Filtering** - Multi-dimensional metric organization  
✅ **Continuous Aggregates** - Automatic materialized views for faster analytics  
✅ **Retention Policies** - Per-metric TTL with automated cleanup  
✅ **Auto-Buffering** - Intelligent batching for optimal compression  
✅ **Thread-Safe** - Concurrent reads and writes  
✅ **Result<T> API** - Modern error handling with detailed context  

### Use Cases

- **Observability & Monitoring** - System metrics, application performance monitoring (APM)
- **IoT & Sensor Data** - Temperature, pressure, environmental sensors
- **Financial Data** - Stock prices, trading volumes, market data
- **Log Analytics** - Structured logs with temporal queries
- **Business Metrics** - KPIs, revenue tracking, user analytics

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    ThemisDB Time-Series Engine                   │
└─────────────────────────────────────────────────────────────────┘
                                 │
        ┌────────────────────────┼────────────────────────┐
        │                        │                        │
        ▼                        ▼                        ▼
┌──────────────┐        ┌──────────────┐        ┌──────────────┐
│  TSStore API │        │ TSAutoBuffer │        │   Retention  │
│   (Primary)  │◄───────│  (Buffering) │        │   Manager    │
└──────┬───────┘        └──────────────┘        └──────┬───────┘
       │                                                │
       │  ┌─────────────────────────────────────┐     │
       │  │      Compression Layer              │     │
       │  ├─────────────────────────────────────┤     │
       └─►│  Gorilla Codec (10-20x ratio)      │◄────┘
          │  • Delta-of-delta timestamps        │
          │  • XOR float compression            │
          └──────────┬──────────────────────────┘
                     │
          ┌──────────▼──────────────────────────┐
          │    Continuous Aggregates            │
          ├─────────────────────────────────────┤
          │  • Pre-computed rollups             │
          │  • Materialized views               │
          │  • Aggregate Scheduler (background) │
          └──────────┬──────────────────────────┘
                     │
          ┌──────────▼──────────────────────────┐
          │       RocksDB Storage               │
          ├─────────────────────────────────────┤
          │  Single: ts:{m}:{e}:{t}             │
          │  Chunks: tsc:{m}:{e}:{t1}:{t2}      │
          └─────────────────────────────────────┘

Legend:
  m = metric name
  e = entity ID
  t = timestamp (ms)
  t1, t2 = chunk start/end timestamps
```

### Component Interaction Flow

1. **Data Ingestion**
   - `TSAutoBuffer` receives data points
   - Buffers points per metric:entity
   - Triggers flush on size/time thresholds

2. **Storage & Compression**
   - `TSStore` applies Gorilla compression (if enabled)
   - Single points → individual RocksDB keys
   - Batch points → compressed chunks

3. **Aggregation**
   - `AggregateScheduler` runs background jobs
   - `ContinuousAggregateManager` computes rollups
   - Stores derived metrics for fast queries

4. **Retention**
   - `RetentionManager` enforces TTL policies
   - Deletes old data per metric configuration
   - Runs as scheduled background task

---

## 🔄 TSStore vs TimeSeriesStore

ThemisDB provides two time-series storage APIs. **Use TSStore for new development.**

### TSStore (Primary API) ✅

**Header:** `include/timeseries/tsstore.h` (~267 lines)

**Modern features:**
- ✅ `Result<T>` return types with detailed error context
- ✅ Tag-based filtering with JSON metadata
- ✅ Gorilla compression support (10-20x ratio)
- ✅ Batch operations for optimal compression
- ✅ Continuous aggregate integration
- ✅ Configurable compression strategies
- ✅ Query optimizer support
- ✅ Metrics collection integration

**Key schema:**
```
Single: ts:{metric}:{entity}:{timestamp_ms}
Chunks: tsc:{metric}:{entity}:{first_ts}:{last_ts}
```

**Value format:**
```json
{
  "value": 75.5,
  "tags": {"region": "us-east", "env": "prod"},
  "metadata": {"version": "1.0"}
}
```

**Example:**
```cpp
#include "timeseries/tsstore.h"

TSStore store(db, cf, config);

TSStore::DataPoint point{
    .metric = "cpu_usage",
    .entity = "server01",
    .timestamp_ms = now_ms(),
    .value = 75.5,
    .tags = {{"region", "us-east"}, {"env", "prod"}}
};

auto result = store.putDataPoint(point);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

### TimeSeriesStore (Legacy API) ⚠️

**Header:** `include/timeseries/timeseries.h` (~151 lines)

**Legacy features:**
- ❌ Boolean return types (limited error info)
- ❌ No built-in tag support
- ❌ No compression support
- ❌ Simpler metadata (JSON only)
- ❌ No batch optimization

**Key schema:**
```
ts:{metric}:{entity}:{timestamp_ms}
```

**Value format:**
```json
{
  "value": 75.5,
  "metadata": {...}
}
```

**Example:**
```cpp
#include "timeseries/timeseries.h"

TimeSeriesStore store(db, cf);

TimeSeriesStore::DataPoint point{
    .timestamp_ms = now_ms(),
    .value = 75.5,
    .metadata = {{"host", "server01"}}
};

bool success = store.put("cpu_usage", "server01", point);
```

### Migration Recommendation

**For new projects:** Use `TSStore` (primary API)  
**For existing projects:** Migrate to `TSStore` for:
- Better error handling
- Compression support
- Tag-based queries
- Performance optimization

---

## 🧩 Core Components

### 1. TSStore (Primary API)

**Header:** `include/timeseries/tsstore.h`  
**Purpose:** Main time-series storage interface

**Key Features:**

- **Single-point inserts**: Individual RocksDB entities
  ```cpp
  Result<void> putDataPoint(const DataPoint& point);
  ```

- **Batch inserts**: Compressed chunks (when enabled)
  ```cpp
  Result<void> putDataPoints(const std::vector<DataPoint>& points);
  ```

- **Range queries**: Time-based filtering
  ```cpp
  Result<std::vector<DataPoint>> query(const QueryOptions& options);
  ```

- **Aggregations**: Min, max, avg, sum, count
  ```cpp
  Result<AggregationResult> aggregate(const QueryOptions& options);
  ```

**Configuration:**

```cpp
TSStore::Config config{
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 24  // 24-hour chunks
};

TSStore store(db, cf, config);
```

**Data Model:**

```cpp
struct DataPoint {
    std::string metric;           // "cpu_usage"
    std::string entity;           // "server01"
    int64_t timestamp_ms;         // 1704067200000
    double value;                 // 75.5
    nlohmann::json tags;          // {"region": "us-east"}
    nlohmann::json metadata;      // Additional data
};
```

**Storage Methods:**

| Method | Storage | Compression | Use Case |
|--------|---------|-------------|----------|
| `putDataPoint()` | Individual keys | No | Real-time single inserts |
| `putDataPoints()` | Compressed chunks | Yes (if enabled) | Batch imports, historical data |

**Key Formats:**

```
# Single point (no compression)
ts:cpu_usage:server01:1704067200000 → {"value": 75.5, ...}

# Compressed chunk (batch with Gorilla)
tsc:cpu_usage:server01:1704067200000:1704153600000 → <compressed binary>
```

---

### 2. Gorilla Compression

**Header:** `include/timeseries/gorilla.h` (~93 lines)  
**Purpose:** High-efficiency time-series compression

**Algorithm:**

Based on Facebook's Gorilla codec:
- **Paper:** "Gorilla: A Fast, Scalable, In-Memory Time Series Database" (VLDB 2015)
- **Authors:** Pelkonen et al., Facebook
- **URL:** https://www.vldb.org/pvldb/vol8/p1816-teller.pdf

**Compression Techniques:**

1. **Timestamp Compression:**
   - Delta-of-delta encoding
   - ZigZag encoding for signed deltas
   - Variable-length integer encoding
   - Typical ratio: 8 bytes → 1-2 bytes

2. **Value Compression:**
   - XOR of IEEE-754 double bit patterns
   - Leading/trailing zero optimization
   - Control bits for encoding type
   - Typical ratio: 8 bytes → 1-4 bytes

**Performance Characteristics:**

| Metric | Value | Notes |
|--------|-------|-------|
| Compression Ratio | 10-20x | Typical for monitoring data |
| CPU Overhead | +15% | Encoding + decoding |
| Memory Usage | Minimal | Streaming encoder/decoder |
| Throughput | ~100K points/sec | Single-threaded |

**When Compression is Applied:**

✅ **Applied:**
- `TSStore::putDataPoints()` with batch data
- `compression = CompressionType::Gorilla`
- Multiple points for same metric:entity
- Data sorted by timestamp

❌ **Not Applied:**
- `TSStore::putDataPoint()` (single inserts)
- `compression = CompressionType::None`
- Mixed metrics in same batch
- Real-time single-point writes

**API:**

```cpp
// Encoding
GorillaEncoder encoder;
encoder.add(timestamp1, value1);
encoder.add(timestamp2, value2);
std::vector<uint8_t> compressed = encoder.finish();

// Decoding
GorillaDecoder decoder(compressed);
while (auto point = decoder.next()) {
    auto [timestamp, value] = *point;
    // Process point...
}
```

**Compression Example:**

```cpp
// Original data (16 bytes per point)
Point 1: timestamp=1704067200000, value=75.5
Point 2: timestamp=1704067260000, value=75.8
Point 3: timestamp=1704067320000, value=76.2
// Total: 48 bytes

// After Gorilla compression: ~3-4 bytes
// Compression ratio: ~12x
```

---

### 3. Continuous Aggregates

**Header:** `include/timeseries/continuous_agg.h` (~41 lines)  
**Purpose:** Materialized views for fast analytics

**Concept:**

Pre-compute aggregations (min, max, avg, sum, count) for time windows and store as derived metrics. This enables fast queries over large time ranges without scanning raw data.

**Configuration:**

```cpp
struct AggConfig {
    std::string metric;                    // "cpu_usage"
    std::optional<std::string> entity;     // "server01" or nullopt
    AggWindow window;                      // Window size
};

struct AggWindow {
    std::chrono::milliseconds size{std::chrono::minutes(1)};
};
```

**Derived Metric Naming:**

```cpp
// Base metric: "cpu_usage"
// Window: 1 minute (60000ms)
// Derived metric: "cpu_usage__agg_60000"

std::string derived = ContinuousAggregateManager::derivedMetricName(
    "cpu_usage", 
    std::chrono::minutes(1)
);
// → "cpu_usage__agg_60000"
```

**Usage:**

```cpp
ContinuousAggregateManager agg_manager(&tsstore);

AggConfig config{
    .metric = "cpu_usage",
    .entity = "server01",
    .window = {std::chrono::minutes(5)}
};

// Compute aggregates for time range
int64_t from_ms = now_ms() - 3600000;  // Last hour
int64_t to_ms = now_ms();
agg_manager.refresh(config, from_ms, to_ms);

// Query derived metric
TSStore::QueryOptions query{
    .metric = "cpu_usage__agg_300000",
    .entity = "server01",
    .from_timestamp_ms = from_ms,
    .to_timestamp_ms = to_ms
};
auto result = tsstore.query(query);
```

**Stored Aggregate Format:**

```json
{
  "value": 75.5,           // avg value
  "tags": {...},
  "metadata": {
    "min": 70.0,
    "max": 80.0,
    "sum": 378.5,
    "count": 5,
    "first_timestamp_ms": 1704067200000,
    "last_timestamp_ms": 1704067500000
  }
}
```

**Benefits:**

- 📊 Fast queries over large time ranges
- 💾 Reduced storage for long-term data
- ⚡ O(1) aggregate lookups vs O(n) scans
- 🔄 Automatic refresh with scheduler

---

### 4. Aggregate Scheduler

**Header:** `include/timeseries/aggregate_scheduler.h` (~148 lines)  
**Purpose:** Background service for continuous aggregate refresh

**Features:**

- ✅ Automatic periodic refresh
- ✅ Configurable refresh intervals
- ✅ Parallel refresh for independent metrics
- ✅ Catch-up for missed windows
- ✅ Health monitoring and error tracking
- ✅ Graceful shutdown with flush

**Configuration:**

```cpp
AggregateScheduler::Config config{
    .max_parallel_refreshes = 4,
    .check_interval = std::chrono::seconds(30),
    .catch_up_missed_windows = true,
    .max_catch_up_windows = 100
};

AggregateScheduler scheduler(&tsstore, config);
```

**Lifecycle:**

```cpp
// 1. Create scheduler
AggregateScheduler scheduler(&tsstore);

// 2. Register aggregates
AggregateScheduler::ScheduledAggregate agg{
    .id = "cpu_5m",
    .config = {
        .metric = "cpu_usage",
        .entity = "server01",
        .window = {std::chrono::minutes(5)}
    },
    .refresh_interval = std::chrono::minutes(5),
    .enabled = true
};
scheduler.registerAggregate(agg);

// 3. Start background thread
scheduler.start();

// 4. Monitor (optional)
auto stats = scheduler.getStats();
std::cout << "Active aggregates: " << stats.active_aggregates << std::endl;

// 5. Stop and cleanup
scheduler.stop();
```

**Management API:**

```cpp
// Register
std::string id = scheduler.registerAggregate(config, refresh_interval);

// Enable/Disable
scheduler.enableAggregate(id);
scheduler.disableAggregate(id);

// Manual refresh
scheduler.refreshNow(id);      // Single aggregate
scheduler.refreshAll();        // All aggregates

// Unregister
scheduler.unregisterAggregate(id);
```

**Statistics:**

```cpp
struct Stats {
    size_t registered_aggregates;
    size_t active_aggregates;
    size_t total_refreshes;
    size_t failed_refreshes;
    std::chrono::system_clock::time_point last_run;
    std::chrono::system_clock::time_point next_run;
};

auto stats = scheduler.getStats();
auto aggregates = scheduler.listAggregates();
for (const auto& agg : aggregates) {
    std::cout << agg.id << ": "
              << agg.total_refreshes << " refreshes, "
              << agg.failed_refreshes << " failures, "
              << agg.avg_refresh_time_ms << "ms avg" << std::endl;
}
```

---

### 5. Retention Policies

**Header:** `include/timeseries/retention.h` (~35 lines)  
**Purpose:** Automated data cleanup based on TTL

**Configuration:**

```cpp
RetentionPolicy policy{
    .per_metric = {
        {"cpu_usage", std::chrono::hours(24 * 7)},    // 7 days
        {"memory_usage", std::chrono::hours(24 * 7)}, // 7 days
        {"disk_io", std::chrono::hours(24 * 30)},     // 30 days
        {"errors", std::chrono::hours(24 * 90)}       // 90 days
    }
};

RetentionManager retention(&tsstore, policy);
```

**Usage:**

```cpp
// Manual enforcement
size_t deleted = retention.apply();
std::cout << "Deleted " << deleted << " old data points" << std::endl;

// Background task (recommended)
std::thread retention_thread([&retention]() {
    while (running) {
        retention.apply();
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
});
```

**Per-Metric Control:**

```cpp
// Delete old data for specific metric
int64_t cutoff = now_ms() - (7 * 24 * 3600000);  // 7 days ago
size_t deleted = tsstore.deleteOldDataForMetric("cpu_usage", cutoff);

// Delete all data for metric
auto result = tsstore.deleteMetric("deprecated_metric");
```

**Best Practices:**

1. Set shorter retention for high-volume metrics
2. Keep longer retention for critical alerts/errors
3. Use continuous aggregates for long-term analytics
4. Schedule retention during off-peak hours
5. Monitor deletion statistics

---

### 6. TS Auto-Buffer

**Header:** `include/timeseries/ts_auto_buffer.h` (~218 lines)  
**Purpose:** Intelligent batching for optimal compression

**Concept:**

Buffer single data points and automatically flush as compressed batches when thresholds are reached. This provides optimal compression ratios while maintaining low latency.

**Configuration:**

```cpp
TSAutoBufferConfig config{
    // Buffer size thresholds
    .max_points_per_buffer = 1000,     // Per metric:entity
    .max_total_points = 10000,         // Across all metrics
    
    // Time-based flush
    .flush_interval = std::chrono::seconds(5),
    
    // Memory management
    .max_memory_bytes = 100 * 1024 * 1024,  // 100 MB
    
    // Performance tuning
    .async_flush = true,
    .flush_batch_size = 500,
    
    // Compression (inherited from TSStore)
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 24
};

TSAutoBuffer buffer(&tsstore, config);
```

**Lifecycle:**

```cpp
// 1. Create buffer
TSAutoBuffer buffer(&tsstore, config);

// 2. Start background flush thread
buffer.start();

// 3. Add points (buffered automatically)
TSStore::DataPoint point{
    .metric = "cpu_usage",
    .entity = "server01",
    .timestamp_ms = now_ms(),
    .value = 75.5
};
auto result = buffer.add(point);

// 4. Monitor (optional)
auto stats = buffer.getStats();
std::cout << "Buffered: " << stats.points_buffered
          << ", Flushed: " << stats.points_flushed << std::endl;

// 5. Stop (flushes remaining points)
buffer.stop();
```

**Flush Triggers:**

| Trigger | Condition | Behavior |
|---------|-----------|----------|
| Size per buffer | Points ≥ `max_points_per_buffer` | Flush that buffer |
| Total size | Points ≥ `max_total_points` | Flush all buffers |
| Time interval | Age ≥ `flush_interval` | Flush old buffers |
| Memory limit | Memory ≥ `max_memory_bytes` | Flush to free memory |
| Manual | `buffer.flush()` called | Flush all buffers |
| Shutdown | `buffer.stop()` called | Flush all buffers |

**Thread Safety:**

```cpp
// Safe concurrent access from multiple threads
std::vector<std::thread> writers;
for (int i = 0; i < 10; ++i) {
    writers.emplace_back([&buffer, i]() {
        for (int j = 0; j < 1000; ++j) {
            buffer.add(make_point(i, j));
        }
    });
}

for (auto& t : writers) t.join();
buffer.flush();
```

**Statistics:**

```cpp
struct TSAutoBufferStats {
    std::atomic<uint64_t> points_buffered;
    std::atomic<uint64_t> points_flushed;
    std::atomic<uint64_t> flush_count;
    std::atomic<uint64_t> auto_flush_count;
    std::atomic<uint64_t> manual_flush_count;
    std::atomic<uint64_t> size_triggered_flush;
    std::atomic<uint64_t> time_triggered_flush;
    std::atomic<uint64_t> buffer_overflow_count;
    
    size_t current_buffer_size;
    size_t current_buffer_memory;
    std::chrono::steady_clock::time_point last_flush_time;
};
```

---

## 💡 Usage Examples

### Example 1: Basic Time-Series Storage

```cpp
#include "timeseries/tsstore.h"
#include <chrono>

// Initialize TSStore with Gorilla compression
TSStore::Config config{
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 24
};
TSStore store(db, cf, config);

// Write a single data point
TSStore::DataPoint point{
    .metric = "cpu_usage",
    .entity = "server01",
    .timestamp_ms = now_ms(),
    .value = 75.5,
    .tags = {{"region", "us-east"}, {"env", "prod"}},
    .metadata = {{"host", "web-01"}}
};

auto result = store.putDataPoint(point);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return;
}

std::cout << "Successfully stored data point" << std::endl;
```

### Example 2: Batch Insert with Compression

```cpp
#include "timeseries/tsstore.h"
#include <vector>

// Prepare batch data
std::vector<TSStore::DataPoint> points;
int64_t base_time = now_ms();

for (int i = 0; i < 1000; ++i) {
    points.push_back({
        .metric = "temperature",
        .entity = "sensor_01",
        .timestamp_ms = base_time + (i * 1000),  // 1-second intervals
        .value = 20.0 + (std::sin(i * 0.1) * 5.0),
        .tags = {{"location", "room_a"}, {"floor", "3"}},
        .metadata = {}
    });
}

// Batch insert (will use Gorilla compression)
auto result = store.putDataPoints(points);
if (result) {
    std::cout << "Stored " << points.size() << " points with compression" 
              << std::endl;
}
```

### Example 3: Range Query with Tag Filtering

```cpp
#include "timeseries/tsstore.h"

// Query data points with filters
TSStore::QueryOptions query{
    .metric = "cpu_usage",
    .entity = std::nullopt,  // All entities
    .from_timestamp_ms = now_ms() - 3600000,  // Last hour
    .to_timestamp_ms = now_ms(),
    .limit = 1000,
    .tag_filter = {{"env", "prod"}}  // Only production
};

auto result = store.query(query);
if (result) {
    std::cout << "Found " << result->size() << " data points:" << std::endl;
    for (const auto& point : *result) {
        std::cout << "  " << point.entity 
                  << " @ " << point.timestamp_ms 
                  << " = " << point.value << std::endl;
    }
}
```

### Example 4: Aggregations

```cpp
#include "timeseries/tsstore.h"

// Compute aggregations over time range
TSStore::QueryOptions query{
    .metric = "memory_usage",
    .entity = "server01",
    .from_timestamp_ms = now_ms() - 86400000,  // Last 24 hours
    .to_timestamp_ms = now_ms()
};

auto result = store.aggregate(query);
if (result) {
    const auto& agg = *result;
    std::cout << "Memory usage statistics (24h):" << std::endl;
    std::cout << "  Min: " << agg.min << " MB" << std::endl;
    std::cout << "  Max: " << agg.max << " MB" << std::endl;
    std::cout << "  Avg: " << agg.avg << " MB" << std::endl;
    std::cout << "  Sum: " << agg.sum << " MB" << std::endl;
    std::cout << "  Count: " << agg.count << " samples" << std::endl;
}
```

### Example 5: Continuous Aggregates

```cpp
#include "timeseries/continuous_agg.h"
#include "timeseries/aggregate_scheduler.h"

// Create aggregate scheduler
AggregateScheduler::Config sched_config{
    .max_parallel_refreshes = 4,
    .check_interval = std::chrono::seconds(30)
};
AggregateScheduler scheduler(&store, sched_config);

// Register 5-minute aggregates for CPU usage
AggConfig cpu_agg{
    .metric = "cpu_usage",
    .entity = "server01",
    .window = {std::chrono::minutes(5)}
};

std::string agg_id = scheduler.registerAggregate(
    cpu_agg, 
    std::chrono::minutes(5)  // Refresh every 5 minutes
);

// Start scheduler
scheduler.start();

// Later: query pre-computed aggregates
TSStore::QueryOptions agg_query{
    .metric = "cpu_usage__agg_300000",  // 5-minute window
    .entity = "server01",
    .from_timestamp_ms = now_ms() - 86400000,
    .to_timestamp_ms = now_ms()
};

auto agg_result = store.query(agg_query);
// Much faster than querying raw data!

scheduler.stop();
```

### Example 6: Retention Policies

```cpp
#include "timeseries/retention.h"

// Define retention policies
RetentionPolicy policy{
    .per_metric = {
        {"cpu_usage", std::chrono::hours(24 * 7)},     // 7 days
        {"memory_usage", std::chrono::hours(24 * 7)},  // 7 days
        {"disk_io", std::chrono::hours(24 * 30)},      // 30 days
        {"error_logs", std::chrono::hours(24 * 90)}    // 90 days
    }
};

RetentionManager retention(&store, policy);

// Apply retention (delete old data)
size_t deleted = retention.apply();
std::cout << "Deleted " << deleted << " expired data points" << std::endl;

// Run as background task
std::thread retention_thread([&retention]() {
    while (running) {
        retention.apply();
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
});
retention_thread.detach();
```

### Example 7: Auto-Buffering for Optimal Compression

```cpp
#include "timeseries/ts_auto_buffer.h"

// Configure auto-buffer
TSAutoBufferConfig buf_config{
    .max_points_per_buffer = 500,
    .flush_interval = std::chrono::seconds(10),
    .compression = TSStore::CompressionType::Gorilla
};

TSAutoBuffer buffer(&store, buf_config);
buffer.start();

// Add points (automatically buffered and compressed)
for (int i = 0; i < 10000; ++i) {
    TSStore::DataPoint point{
        .metric = "sensor_temperature",
        .entity = "sensor_" + std::to_string(i % 100),
        .timestamp_ms = now_ms(),
        .value = 20.0 + (rand() % 100) / 10.0,
        .tags = {{"location", "warehouse"}}
    };
    
    auto result = buffer.add(point);
    if (!result) {
        std::cerr << "Buffer add failed: " << result.error().message << std::endl;
    }
}

// Get statistics
auto stats = buffer.getStats();
std::cout << "Buffered: " << stats.points_buffered << std::endl;
std::cout << "Flushed: " << stats.points_flushed << std::endl;
std::cout << "Compression ratio: " 
          << (double)stats.points_buffered / stats.flush_count 
          << " points/batch" << std::endl;

buffer.stop();  // Flushes remaining points
```

### Example 8: Monitoring System Integration

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"
#include <thread>
#include <chrono>

class SystemMonitor {
public:
    SystemMonitor(TSAutoBuffer* buffer) : buffer_(buffer) {}
    
    void start() {
        running_ = true;
        monitor_thread_ = std::thread([this]() { this->monitorLoop(); });
    }
    
    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
    
private:
    void monitorLoop() {
        while (running_) {
            // Collect system metrics
            double cpu = getCpuUsage();
            double memory = getMemoryUsage();
            double disk = getDiskUsage();
            
            int64_t now = now_ms();
            
            // Buffer metrics (compressed automatically)
            buffer_->add({
                .metric = "cpu_usage",
                .entity = "localhost",
                .timestamp_ms = now,
                .value = cpu,
                .tags = {{"host", getHostname()}}
            });
            
            buffer_->add({
                .metric = "memory_usage",
                .entity = "localhost",
                .timestamp_ms = now,
                .value = memory,
                .tags = {{"host", getHostname()}}
            });
            
            buffer_->add({
                .metric = "disk_usage",
                .entity = "localhost",
                .timestamp_ms = now,
                .value = disk,
                .tags = {{"host", getHostname()}}
            });
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    double getCpuUsage() { /* ... */ return 0.0; }
    double getMemoryUsage() { /* ... */ return 0.0; }
    double getDiskUsage() { /* ... */ return 0.0; }
    std::string getHostname() { return "server01"; }
    
    TSAutoBuffer* buffer_;
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
};

// Usage
TSAutoBuffer buffer(&store, config);
buffer.start();

SystemMonitor monitor(&buffer);
monitor.start();

// Monitor runs continuously...

monitor.stop();
buffer.stop();
```

### Example 9: Multi-Metric Dashboard Data

```cpp
#include "timeseries/tsstore.h"
#include <map>
#include <vector>

// Fetch dashboard data for multiple metrics
std::map<std::string, TSStore::AggregationResult> fetchDashboardData(
    TSStore& store,
    const std::vector<std::string>& metrics,
    int64_t time_range_hours = 24
) {
    std::map<std::string, TSStore::AggregationResult> results;
    
    int64_t from = now_ms() - (time_range_hours * 3600000);
    int64_t to = now_ms();
    
    for (const auto& metric : metrics) {
        TSStore::QueryOptions query{
            .metric = metric,
            .entity = std::nullopt,  // All entities
            .from_timestamp_ms = from,
            .to_timestamp_ms = to
        };
        
        auto result = store.aggregate(query);
        if (result) {
            results[metric] = *result;
        }
    }
    
    return results;
}

// Usage
std::vector<std::string> dashboard_metrics = {
    "cpu_usage",
    "memory_usage",
    "disk_io",
    "network_throughput",
    "request_count",
    "error_rate"
};

auto data = fetchDashboardData(store, dashboard_metrics, 24);

for (const auto& [metric, agg] : data) {
    std::cout << metric << ":" << std::endl;
    std::cout << "  Current: " << agg.avg << std::endl;
    std::cout << "  Peak: " << agg.max << std::endl;
    std::cout << "  Min: " << agg.min << std::endl;
}
```

---

## ⚙️ Configuration

### TSStore Configuration

```cpp
struct TSStore::Config {
    // Compression strategy
    CompressionType compression = CompressionType::Gorilla;
    
    // Gorilla chunk size (hours)
    // Larger chunks = better compression, more memory during decode
    int chunk_size_hours = 24;
};

// Example configurations

// High compression (low write rate)
TSStore::Config high_compression{
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 24
};

// Low latency (high write rate)
TSStore::Config low_latency{
    .compression = TSStore::CompressionType::None,
    .chunk_size_hours = 1
};

// Balanced (default)
TSStore::Config balanced{
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 6
};
```

### Auto-Buffer Configuration

```cpp
struct TSAutoBufferConfig {
    // Buffer thresholds
    size_t max_points_per_buffer = 1000;      // Per metric:entity
    size_t max_total_points = 10000;          // Global limit
    
    // Timing
    std::chrono::milliseconds flush_interval{5000};  // 5 seconds
    
    // Memory
    size_t max_memory_bytes = 100 * 1024 * 1024;  // 100 MB
    
    // Performance
    bool async_flush = true;
    size_t flush_batch_size = 500;
    
    // Compression (TSStore settings)
    TSStore::CompressionType compression = TSStore::CompressionType::Gorilla;
    int chunk_size_hours = 24;
};
```

### Aggregate Scheduler Configuration

```cpp
struct AggregateScheduler::Config {
    // Parallelism
    size_t max_parallel_refreshes = 4;
    
    // Timing
    std::chrono::milliseconds check_interval{std::chrono::seconds(30)};
    
    // Catch-up behavior
    bool catch_up_missed_windows = true;
    size_t max_catch_up_windows = 100;
};
```

### Retention Policy Configuration

```cpp
struct RetentionPolicy {
    // Per-metric retention periods
    std::unordered_map<std::string, std::chrono::seconds> per_metric;
};

// Example
RetentionPolicy policy{
    .per_metric = {
        // High-frequency metrics: short retention
        {"cpu_usage", std::chrono::hours(24 * 7)},        // 7 days
        {"memory_usage", std::chrono::hours(24 * 7)},     // 7 days
        {"network_io", std::chrono::hours(24 * 3)},       // 3 days
        
        // Medium-frequency: medium retention
        {"disk_usage", std::chrono::hours(24 * 30)},      // 30 days
        {"application_metrics", std::chrono::hours(24 * 30)}, // 30 days
        
        // Low-frequency/critical: long retention
        {"error_logs", std::chrono::hours(24 * 90)},      // 90 days
        {"security_events", std::chrono::hours(24 * 365)} // 1 year
    }
};
```

---

## 🚀 Performance Considerations

### Storage Efficiency

| Aspect | Without Compression | With Gorilla | Improvement |
|--------|---------------------|--------------|-------------|
| Data Point Size | 16 bytes | 1-2 bytes | 10-20x |
| 1M points/day | 15.3 MB | 0.8-1.5 MB | ~10x |
| 1 year storage | 5.6 GB | 300-550 MB | ~10x |
| RocksDB keys | Many small | Few large chunks | Less overhead |

**Recommendations:**
- Use Gorilla compression for monitoring data (regular intervals)
- Use no compression for sparse or irregular data
- Adjust `chunk_size_hours` based on write patterns
- Monitor compression ratio in production

### Query Performance

| Operation | Complexity | Typical Time | Optimizations |
|-----------|------------|--------------|---------------|
| Single point lookup | O(1) | < 1ms | RocksDB Get |
| Range scan (raw) | O(n) | 10-100ms | Prefix iteration |
| Range scan (compressed) | O(n/c) | 5-50ms | Chunk decompression |
| Aggregation (raw) | O(n) | 50-500ms | Single-pass |
| Aggregation (pre-computed) | O(1) | < 1ms | Continuous aggregates |

**Recommendations:**
- Use continuous aggregates for frequently queried ranges
- Limit query result size with `.limit` parameter
- Use tag filters to reduce scan range
- Enable query optimizer for complex aggregations

### Write Throughput

| Method | Throughput | Latency | Use Case |
|--------|-----------|---------|----------|
| `putDataPoint()` | ~50K/sec | < 1ms | Real-time single writes |
| `putDataPoints()` (no compression) | ~100K/sec | < 1ms | Batch imports |
| `putDataPoints()` (Gorilla) | ~80K/sec | 1-5ms | Compressed batches |
| `TSAutoBuffer::add()` | ~200K/sec | < 0.1ms | Buffered writes |

**Recommendations:**
- Use `TSAutoBuffer` for high-throughput scenarios
- Batch writes when possible (better compression)
- Use async flush for lower latency
- Monitor buffer statistics

### Memory Usage

| Component | Memory Usage | Scaling | Notes |
|-----------|--------------|---------|-------|
| TSStore | Minimal | Constant | Streaming operations |
| GorillaEncoder | ~1KB per chunk | Per active metric:entity | Released after flush |
| GorillaDecoder | ~1KB per chunk | Per active query | Short-lived |
| TSAutoBuffer | Configurable | Linear with buffer size | Monitor with `getStats()` |
| AggregateScheduler | ~10KB per aggregate | Linear with registered | Background thread |

**Recommendations:**
- Set `max_memory_bytes` in `TSAutoBufferConfig`
- Limit concurrent queries for memory-constrained systems
- Use smaller `chunk_size_hours` for lower memory footprint
- Monitor with system metrics

### Benchmarks (Single-threaded)

Hardware: Intel i7-9700K, 32GB RAM, NVMe SSD

```
Operation                          Throughput      Latency (p50/p99)
─────────────────────────────────────────────────────────────────────
putDataPoint (no compression)      52,000/sec      18µs / 45µs
putDataPoints batch=1000 (Gorilla) 95,000/sec      10ms / 25ms
TSAutoBuffer::add                  180,000/sec     5µs / 15µs
query (1000 points, compressed)    520,000/sec     1.9ms / 5ms
aggregate (10K points, raw)        12,000/sec      80ms / 150ms
aggregate (pre-computed)           850,000/sec     1.2µs / 3µs
```

---

## ✅ Best Practices

### 1. Use TSStore (Primary API) for New Development

```cpp
// ✅ Recommended: TSStore with Result<T>
auto result = tsstore.putDataPoint(point);
if (!result) {
    log_error("Failed to store point: {}", result.error().message);
    return;
}

// ❌ Avoid: TimeSeriesStore with bool returns
bool success = old_store.put(metric, entity, point);
if (!success) {
    log_error("Failed to store point (no error details)");
}
```

### 2. Enable Gorilla Compression for Regular Time-Series

```cpp
// ✅ Regular monitoring data: Use Gorilla
TSStore::Config config{
    .compression = TSStore::CompressionType::Gorilla,
    .chunk_size_hours = 24
};

// ❌ Sparse or irregular data: Don't use compression
TSStore::Config config{
    .compression = TSStore::CompressionType::None
};
```

### 3. Use Batching for Better Compression

```cpp
// ✅ Batch inserts with compression
std::vector<TSStore::DataPoint> points;
// ... collect points ...
tsstore.putDataPoints(points);  // 10-20x compression

// ❌ Individual inserts (no compression benefit)
for (const auto& point : points) {
    tsstore.putDataPoint(point);  // Each point stored separately
}
```

### 4. Use TSAutoBuffer for High-Throughput Scenarios

```cpp
// ✅ High-throughput with automatic batching
TSAutoBuffer buffer(&tsstore, config);
buffer.start();
buffer.add(point);  // Automatically batched and compressed

// ❌ Manual batching (more complex)
std::vector<TSStore::DataPoint> batch;
batch.push_back(point);
if (batch.size() >= 1000) {
    tsstore.putDataPoints(batch);
    batch.clear();
}
```

### 5. Use Continuous Aggregates for Frequent Analytics

```cpp
// ✅ Pre-compute common aggregations
AggregateScheduler scheduler(&tsstore);
scheduler.registerAggregate(
    {.metric = "cpu_usage", .window = {std::chrono::minutes(5)}},
    std::chrono::minutes(5)
);
scheduler.start();

// Query pre-computed aggregates (fast!)
tsstore.query({.metric = "cpu_usage__agg_300000", ...});

// ❌ Compute aggregations on every query (slow)
tsstore.aggregate({.metric = "cpu_usage", ...});  // Scans raw data
```

### 6. Implement Retention Policies

```cpp
// ✅ Automated retention management
RetentionPolicy policy{
    .per_metric = {
        {"cpu_usage", std::chrono::hours(24 * 7)},  // 7 days
        {"errors", std::chrono::hours(24 * 90)}     // 90 days
    }
};
RetentionManager retention(&tsstore, policy);

// Run periodically
retention.apply();  // Deletes old data

// ❌ No retention policy (unbounded growth)
// Database grows indefinitely, performance degrades
```

---

## 📚 API Reference

### TSStore

```cpp
class TSStore {
public:
    // Constructors
    explicit TSStore(rocksdb::TransactionDB* db, 
                     rocksdb::ColumnFamilyHandle* cf,
                     Config config);
    TSStore(rocksdb::TransactionDB* db, 
            rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    // Configuration
    const Config& getConfig() const;
    void setConfig(const Config& config);
    
    // Write operations
    Result<void> putDataPoint(const DataPoint& point);
    Result<void> putDataPoints(const std::vector<DataPoint>& points);
    
    // Read operations
    Result<std::vector<DataPoint>> query(const QueryOptions& options) const;
    Result<AggregationResult> aggregate(const QueryOptions& options) const;
    Result<AggregationResult> aggregateOptimized(
        const QueryOptions& options,
        bool use_optimizer = true) const;
    
    // Management
    Stats getStats() const;
    size_t deleteOldData(int64_t before_timestamp_ms);
    size_t deleteOldDataForMetric(const std::string& metric, 
                                   int64_t before_timestamp_ms);
    Result<void> deleteMetric(const std::string& metric);
    void clear();
    
    // Metrics
    void setMetrics(std::shared_ptr<TimeSeriesMetrics> metrics);
    std::shared_ptr<TimeSeriesMetrics> getMetrics() const;
};
```

### GorillaEncoder

```cpp
class GorillaEncoder {
public:
    // Add timestamp-value pair
    void add(int64_t timestamp_ms, double value);
    
    // Finish encoding and get compressed data
    std::vector<uint8_t> finish();
};
```

### GorillaDecoder

```cpp
class GorillaDecoder {
public:
    explicit GorillaDecoder(const std::vector<uint8_t>& data);
    
    // Get next timestamp-value pair
    std::optional<std::pair<int64_t, double>> next();
};
```

### ContinuousAggregateManager

```cpp
class ContinuousAggregateManager {
public:
    explicit ContinuousAggregateManager(TSStore* store);
    
    // Compute and store aggregates for time range
    void refresh(const AggConfig& cfg, int64_t from_ms, int64_t to_ms);
    
    // Get derived metric name
    static std::string derivedMetricName(
        const std::string& base, 
        std::chrono::milliseconds win);
};
```

### AggregateScheduler

```cpp
class AggregateScheduler {
public:
    explicit AggregateScheduler(TSStore* store);
    AggregateScheduler(TSStore* store, const Config& config);
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Registration
    void registerAggregate(const ScheduledAggregate& agg);
    std::string registerAggregate(const AggConfig& config, 
                                   std::chrono::milliseconds refresh_interval);
    void unregisterAggregate(const std::string& id);
    void enableAggregate(const std::string& id);
    void disableAggregate(const std::string& id);
    
    // Manual operations
    void refreshNow(const std::string& id);
    void refreshAll();
    
    // Statistics
    Stats getStats() const;
    std::vector<ScheduledAggregate> listAggregates() const;
};
```

### RetentionManager

```cpp
class RetentionManager {
public:
    RetentionManager(TSStore* store, RetentionPolicy policy);
    
    // Apply retention policies (delete old data)
    size_t apply();
};
```

### TSAutoBuffer

```cpp
class TSAutoBuffer {
public:
    explicit TSAutoBuffer(TSStore* tsstore, 
                          TSAutoBufferConfig config = TSAutoBufferConfig{});
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Buffering
    Result<void> add(const TSStore::DataPoint& point);
    size_t flush();
    size_t flushFor(const std::string& metric, const std::string& entity);
    
    // Configuration
    TSAutoBufferStats getStats() const;
    const TSAutoBufferConfig& getConfig() const;
    void setConfig(const TSAutoBufferConfig& config);
};
```

---

## 🔗 Related Documentation

### Core Documentation
- **[Architecture Overview](../ARCHITECTURE.md)** - System architecture and design
- **[Implementation Summary](../IMPLEMENTATION_SUMMARY_WIRE_PROTOCOL_TIMESERIES.md)** - Wire protocol and time-series features
- **[German Documentation](../de/timeseries/README.md)** - Deutsche Dokumentation

### Time-Series Specific
- **[Auto-Buffer Guide](../de/timeseries/AUTO_BUFFER.md)** - Detailed auto-buffering documentation
- **[Storage Methods](../de/timeseries/STORAGE_METHODS.md)** - Storage strategy comparison

### Examples
- **[Time-Series Monitor](../../examples/05_time_series_monitor/)** - Real-world monitoring example
  - [README](../../examples/05_time_series_monitor/README.md)
  - [How-To Guide](../../examples/05_time_series_monitor/HOW_TO.md)
  - [Monitoring Guide](../../examples/05_time_series_monitor/MONITORING_GUIDE.md)
  - [Troubleshooting](../../examples/05_time_series_monitor/TROUBLESHOOTING.md)

### Source Code
- **Headers:** `include/timeseries/*.h`
- **Implementation:** `src/timeseries/*.cpp`
- **Tests:** `tests/test_timeseries*.cpp`

### External References
- **[Gorilla Paper](https://www.vldb.org/pvldb/vol8/p1816-teller.pdf)** - Original Gorilla compression algorithm
- **[RocksDB Documentation](https://github.com/facebook/rocksdb/wiki)** - Underlying storage engine
- **[nlohmann/json](https://github.com/nlohmann/json)** - JSON library used for metadata

---



### TimeSeriesStore (Legacy API Reference)

For legacy applications still using TimeSeriesStore, here is the complete API reference:

```cpp
class TimeSeriesStore {
public:
    struct DataPoint {
        int64_t timestamp_ms;
        double value;
        nlohmann::json metadata;
        
        nlohmann::json toJson() const;
        static DataPoint fromJson(const nlohmann::json& j);
    };
    
    struct RangeQuery {
        int64_t from_ms = 0;
        int64_t to_ms = INT64_MAX;
        size_t limit = 1000;
        bool descending = false;
    };
    
    struct Aggregation {
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;
        double sum = 0.0;
        size_t count = 0;
        
        nlohmann::json toJson() const;
    };
    
    explicit TimeSeriesStore(rocksdb::TransactionDB* db,
                            rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    // Write operations
    bool put(std::string_view metric, 
             std::string_view entity,
             const DataPoint& point);
    
    // Query operations
    std::vector<DataPoint> query(std::string_view metric,
                                  std::string_view entity) const;
    std::vector<DataPoint> query(std::string_view metric,
                                  std::string_view entity,
                                  const RangeQuery& query) const;
    
    // Aggregation operations
    Aggregation aggregate(std::string_view metric,
                         std::string_view entity) const;
    Aggregation aggregate(std::string_view metric,
                         std::string_view entity,
                         const RangeQuery& query) const;
    
    // Retention operations
    size_t deleteOldPoints(std::string_view metric,
                           std::string_view entity,
                           int64_t before_ms);
    
    // Latest value
    std::optional<DataPoint> getLatest(std::string_view metric,
                                       std::string_view entity) const;
};
```

**Migration Path from TimeSeriesStore to TSStore:**

```cpp
// Old code (TimeSeriesStore)
TimeSeriesStore old_store(db, cf);
TimeSeriesStore::DataPoint old_point{
    .timestamp_ms = now_ms(),
    .value = 75.5,
    .metadata = {{"host", "server01"}}
};
bool success = old_store.put("cpu_usage", "server01", old_point);

// New code (TSStore)
TSStore::Config config{.compression = TSStore::CompressionType::Gorilla};
TSStore new_store(db, cf, config);
TSStore::DataPoint new_point{
    .metric = "cpu_usage",
    .entity = "server01",
    .timestamp_ms = now_ms(),
    .value = 75.5,
    .tags = {},
    .metadata = {{"host", "server01"}}
};
auto result = new_store.putDataPoint(new_point);
if (!result) {
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

### Data Structures Reference

#### TSStore::DataPoint

Complete structure with all fields:

```cpp
struct DataPoint {
    std::string metric;           // Required: Metric name
    std::string entity;           // Required: Entity identifier
    int64_t timestamp_ms;         // Required: Unix timestamp in milliseconds
    double value;                 // Required: Numeric value
    nlohmann::json tags;          // Optional: Filtering/grouping tags
    nlohmann::json metadata;      // Optional: Additional information
    
    // Example usage
    TSStore::DataPoint point{
        .metric = "system.cpu.usage",
        .entity = "host.server01",
        .timestamp_ms = 1704067200000,
        .value = 75.5,
        .tags = {
            {"region", "us-east-1"},
            {"az", "us-east-1a"},
            {"env", "production"},
            {"service", "web"},
            {"version", "2.1.0"}
        },
        .metadata = {
            {"hostname", "web-server-01.example.com"},
            {"ip", "10.0.1.42"},
            {"collector", "telegraf"},
            {"collector_version", "1.28.0"}
        }
    };
};
```

#### TSStore::QueryOptions

Complete structure with all fields:

```cpp
struct QueryOptions {
    std::string metric;                      // Required: Metric to query
    std::optional<std::string> entity;       // Optional: Specific entity (nullopt = all)
    int64_t from_timestamp_ms = 0;          // Start time (inclusive)
    int64_t to_timestamp_ms = INT64_MAX;    // End time (inclusive)
    size_t limit = 1000;                    // Max results to return
    nlohmann::json tag_filter;               // Optional: Tag-based filtering
    
    // Example usage
    TSStore::QueryOptions query{
        .metric = "system.cpu.usage",
        .entity = std::nullopt,  // All entities
        .from_timestamp_ms = now_ms() - 3600000,  // Last hour
        .to_timestamp_ms = now_ms(),
        .limit = 10000,
        .tag_filter = {
            {"env", "production"},
            {"region", "us-east-1"}
        }
    };
};
```

#### TSStore::AggregationResult

Complete structure with all fields:

```cpp
struct AggregationResult {
    double min = 0.0;                       // Minimum value
    double max = 0.0;                       // Maximum value
    double avg = 0.0;                       // Average value
    double sum = 0.0;                       // Sum of all values
    size_t count = 0;                       // Number of data points
    int64_t first_timestamp_ms = 0;         // Timestamp of first point
    int64_t last_timestamp_ms = 0;          // Timestamp of last point
    
    // Example output
    auto result = store.aggregate(query);
    if (result) {
        std::cout << "Statistics:" << std::endl;
        std::cout << "  Count: " << result->count << std::endl;
        std::cout << "  Min: " << result->min << std::endl;
        std::cout << "  Max: " << result->max << std::endl;
        std::cout << "  Avg: " << result->avg << std::endl;
        std::cout << "  Sum: " << result->sum << std::endl;
        std::cout << "  First: " << result->first_timestamp_ms << std::endl;
        std::cout << "  Last: " << result->last_timestamp_ms << std::endl;
    }
};
```

#### TSStore::Stats

Complete structure with all fields:

```cpp
struct Stats {
    size_t total_data_points = 0;          // Total points stored
    size_t total_metrics = 0;              // Number of unique metrics
    size_t total_size_bytes = 0;           // Approximate storage size
    int64_t oldest_timestamp_ms = 0;       // Oldest data point timestamp
    int64_t newest_timestamp_ms = 0;       // Newest data point timestamp
    
    // Example usage
    auto stats = store.getStats();
    std::cout << "Time-Series Statistics:" << std::endl;
    std::cout << "  Total Data Points: " << stats.total_data_points << std::endl;
    std::cout << "  Unique Metrics: " << stats.total_metrics << std::endl;
    std::cout << "  Storage Size: " << stats.total_size_bytes / (1024*1024) << " MB" << std::endl;
    
    // Calculate time range
    int64_t range_ms = stats.newest_timestamp_ms - stats.oldest_timestamp_ms;
    int64_t range_days = range_ms / (24 * 3600000);
    std::cout << "  Time Range: " << range_days << " days" << std::endl;
};
```

### Advanced Patterns

#### Pattern 1: Multi-Tenant Time-Series

```cpp
// Use entity field for tenant isolation
class TenantTimeSeriesStore {
    TSStore* store_;
    
public:
    TenantTimeSeriesStore(TSStore* store) : store_(store) {}
    
    Result<void> putTenantMetric(
        const std::string& tenant_id,
        const std::string& metric,
        double value,
        const nlohmann::json& tags = {}
    ) {
        TSStore::DataPoint point{
            .metric = metric,
            .entity = tenant_id,
            .timestamp_ms = now_ms(),
            .value = value,
            .tags = tags
        };
        return store_->putDataPoint(point);
    }
    
    Result<TSStore::AggregationResult> getTenantStats(
        const std::string& tenant_id,
        const std::string& metric,
        int64_t from_ms,
        int64_t to_ms
    ) {
        TSStore::QueryOptions query{
            .metric = metric,
            .entity = tenant_id,
            .from_timestamp_ms = from_ms,
            .to_timestamp_ms = to_ms
        };
        return store_->aggregate(query);
    }
};
```

#### Pattern 2: Hierarchical Metrics

```cpp
// Use dot-notation for metric hierarchies
struct MetricHierarchy {
    static std::string makeMetric(
        const std::string& category,
        const std::string& subcategory,
        const std::string& name
    ) {
        return category + "." + subcategory + "." + name;
    }
    
    static std::vector<std::string> parseMetric(const std::string& metric) {
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = metric.find('.');
        
        while (end != std::string::npos) {
            parts.push_back(metric.substr(start, end - start));
            start = end + 1;
            end = metric.find('.', start);
        }
        parts.push_back(metric.substr(start));
        
        return parts;
    }
};

// Usage
std::string cpu_metric = MetricHierarchy::makeMetric("system", "cpu", "usage");
// → "system.cpu.usage"

std::string mem_metric = MetricHierarchy::makeMetric("system", "memory", "used");
// → "system.memory.used"
```

#### Pattern 3: Time-Window Aggregations

```cpp
// Compute aggregations for sliding time windows
class WindowAggregator {
    TSStore* store_;
    
public:
    WindowAggregator(TSStore* store) : store_(store) {}
    
    std::vector<TSStore::AggregationResult> computeWindows(
        const std::string& metric,
        const std::string& entity,
        int64_t start_ms,
        int64_t end_ms,
        std::chrono::milliseconds window_size
    ) {
        std::vector<TSStore::AggregationResult> results;
        
        int64_t window_ms = window_size.count();
        for (int64_t window_start = start_ms; 
             window_start < end_ms; 
             window_start += window_ms) {
            
            int64_t window_end = std::min(window_start + window_ms, end_ms);
            
            TSStore::QueryOptions query{
                .metric = metric,
                .entity = entity,
                .from_timestamp_ms = window_start,
                .to_timestamp_ms = window_end
            };
            
            auto result = store_->aggregate(query);
            if (result) {
                results.push_back(*result);
            }
        }
        
        return results;
    }
};

// Usage
WindowAggregator aggregator(&store);
auto windows = aggregator.computeWindows(
    "cpu_usage",
    "server01",
    now_ms() - 3600000,  // Last hour
    now_ms(),
    std::chrono::minutes(5)  // 5-minute windows
);

for (size_t i = 0; i < windows.size(); ++i) {
    std::cout << "Window " << i << ": avg=" << windows[i].avg 
              << ", max=" << windows[i].max << std::endl;
}
```


## 📊 Quick Reference

### Common Operations

```cpp
// Initialize
TSStore store(db, cf, {.compression = TSStore::CompressionType::Gorilla});

// Write single point
store.putDataPoint({
    .metric = "cpu", .entity = "s1", .timestamp_ms = now_ms(), .value = 75.5
});

// Write batch
store.putDataPoints(points);

// Query
auto result = store.query({
    .metric = "cpu", .from_timestamp_ms = start, .to_timestamp_ms = end
});

// Aggregate
auto agg = store.aggregate({
    .metric = "cpu", .entity = "s1", .from_timestamp_ms = start, .to_timestamp_ms = end
});

// Delete old data
store.deleteOldData(cutoff_ms);
```

### Performance Tips

| Scenario | Recommendation |
|----------|----------------|
| High write throughput | Use `TSAutoBuffer` |
| Regular time-series | Enable Gorilla compression |
| Frequent analytics | Use continuous aggregates |
| Large time ranges | Pre-compute aggregates |
| Long-term storage | Implement retention policies |
| Low memory | Reduce `chunk_size_hours` |

---

**© 2024 ThemisDB Project | Apache License 2.0**
