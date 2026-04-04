# ThemisDB Performance Profiling and Optimization Tools

This document describes the performance profiling and optimization tools available in ThemisDB.

## Overview

ThemisDB provides comprehensive performance profiling capabilities to help identify bottlenecks, optimize queries, and tune system performance. The profiling infrastructure consists of:

1. **Query Profiler** - Profiles AQL query execution with detailed timing and operator statistics
2. **Storage Profiler** - Profiles RocksDB operations and storage layer performance
3. **Performance Analyzer** - Analyzes profiles to identify issues and provide recommendations
4. **CLI Tool** - Command-line interface for profiling and analysis

## Components

### 1. Query Profiler

The Query Profiler (`include/observability/query_profiler.h`) tracks query execution phases and operator performance.

**Features:**
- Phase timing (parse, validate, optimize, plan, execute)
- Operator-level statistics (scans, filters, joins, aggregates)
- Index usage tracking
- Cache hit/miss tracking
- Automatic slow query detection
- Memory and I/O statistics
- Optimization hints and warnings

**Usage:**

```cpp
#include "observability/query_profiler.h"

using namespace themis::observability;

// Create profiler
QueryProfilerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
QueryProfiler profiler(config);

// Profile a query
{
    ScopedQueryProfile profile(profiler, "query-123", "SELECT * FROM users");
    
    // Record phases
    profile.record_phase(QueryPhase::PARSE, std::chrono::microseconds(100));
    profile.record_phase(QueryPhase::EXECUTE, std::chrono::microseconds(5000));
    
    // Record operators
    {
        ScopedOperatorProfile op(profiler, "query-123", 
                                OperatorType::INDEX_SCAN, "users_index");
        op.record_rows(1000);
        op.record_cache_hit();
    }
}

// Get results
auto profiles = profiler.get_slow_queries(std::chrono::milliseconds(500));
for (const auto& p : profiles) {
    std::cout << p->toSummary() << "\n";
}
```

### 2. Storage Profiler

The Storage Profiler (`include/observability/storage_profiler.h`) monitors RocksDB operations and storage performance.

**Features:**
- Operation timing (GET, PUT, DELETE, SCAN, etc.)
- Bytes read/written tracking
- Cache hit/miss statistics
- SST file access patterns
- Compaction metrics
- Write/read/space amplification
- Memory usage tracking

**Usage:**

```cpp
#include "observability/storage_profiler.h"

using namespace themis::observability;

// Create profiler
StorageProfiler profiler;

// Profile operations
{
    ScopedStorageOp op(profiler, StorageOpType::GET, "default");
    op.record_bytes_read(1024);
    op.set_cache_hit(true);
}

// Get statistics
auto summary = profiler.get_operation_summary();
auto cache_metrics = profiler.get_cache_metrics();
auto amp_metrics = profiler.get_amplification_metrics();

std::cout << summary.dump(2) << "\n";
```

### 3. Performance Analyzer

The Performance Analyzer (`include/observability/performance_analyzer.h`) analyzes profiling data to identify issues.

**Features:**
- Slow query detection
- Index usage analysis
- Cache efficiency analysis
- Storage amplification detection
- Automated recommendations
- HTML and JSON report generation

**Issue Categories:**
- Query optimization
- Index usage
- Cache efficiency
- Storage amplification
- Resource usage
- Slow operations

**Usage:**

```cpp
#include "observability/performance_analyzer.h"

using namespace themis::observability;

// Create analyzer
PerformanceAnalyzerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
config.cache_hit_rate_threshold = 80.0;
PerformanceAnalyzer analyzer(config);

// Run analysis
auto analysis = analyzer.analyze(query_profiler, storage_profiler);

// Export results
analyzer.export_analysis(analysis, "performance_report.json");
analyzer.export_html_report(analysis, "performance_report.html");

// Print report
std::cout << analysis.toReport() << "\n";
```

### 4. CLI Tool

The `themis_profiler.py` CLI tool provides easy access to profiling features.

**Installation:**

```bash
# Ensure Python 3 and requests library are installed
pip3 install requests

# Make executable
chmod +x tools/themis_profiler.py
```

**Commands:**

```bash
# Enable profiling
./tools/themis_profiler.py enable

# Disable profiling
./tools/themis_profiler.py disable

# View query profiles
./tools/themis_profiler.py queries --limit 10 --verbose

# View slow queries
./tools/themis_profiler.py slow-queries --threshold 1000

# View storage statistics
./tools/themis_profiler.py storage

# Run performance analysis
./tools/themis_profiler.py analyze --output analysis.json

# Export all profiles
./tools/themis_profiler.py export profiles.json

# Monitor in real-time
./tools/themis_profiler.py monitor --interval 5
```

**Options:**

- `--host HOST` - ThemisDB server host (default: localhost)
- `--port PORT` - ThemisDB server port (default: 8080)

## API Endpoints

The profiling tools can be accessed via HTTP API:

```bash
# Enable/disable profiling
POST /api/profiling/enable
POST /api/profiling/disable

# Query profiles
GET /api/profiling/queries?limit=10
GET /api/profiling/slow-queries?threshold_ms=1000

# Storage statistics
GET /api/profiling/storage

# Performance analysis
POST /api/profiling/analyze

# Export
GET /api/profiling/export
```

## Performance Tuning Guide

### Identifying Slow Queries

1. Enable profiling: `./tools/themis_profiler.py enable`
2. Run workload
3. Check slow queries: `./tools/themis_profiler.py slow-queries`
4. Analyze query plans for optimization opportunities

### Improving Cache Performance

1. Check cache metrics: `./tools/themis_profiler.py storage`
2. If hit rate is low (<80%):
   - Increase block cache size in RocksDB config
   - Review query patterns
   - Warm up cache with common queries

### Optimizing Index Usage

1. Run analysis: `./tools/themis_profiler.py analyze`
2. Review index usage percentage
3. Create missing indexes for frequently filtered columns
4. Use EXPLAIN to verify index selection

### Reducing Storage Amplification

1. Check amplification metrics: `./tools/themis_profiler.py storage`
2. If write amplification is high (>10):
   - Increase memtable size
   - Adjust compaction settings
   - Use larger SST file sizes
3. If read amplification is high (>5):
   - Increase block cache
   - Enable bloom filters
   - Optimize compaction strategy

## Configuration

### Query Profiler Configuration

```cpp
QueryProfilerConfig config;
config.enabled = true;
config.profile_all_queries = false;  // Profile only slow queries
config.collect_operator_stats = true;
config.collect_memory_stats = true;
config.collect_io_stats = true;
config.max_profiles_retained = 1000;
config.retention_duration = std::chrono::seconds(3600);
config.log_slow_queries = true;
config.slow_query_threshold = std::chrono::milliseconds(1000);
```

### Storage Profiler Configuration

```cpp
StorageProfilerConfig config;
config.enabled = true;
config.collect_op_stats = true;
config.collect_rocksdb_stats = true;
config.max_ops_retained = 10000;
config.stats_collection_interval = std::chrono::seconds(60);
config.retention_duration = std::chrono::seconds(3600);
config.log_slow_ops = true;
config.slow_op_threshold = std::chrono::milliseconds(100);
```

### Performance Analyzer Configuration

```cpp
PerformanceAnalyzerConfig config;
config.slow_query_threshold = std::chrono::milliseconds(1000);
config.slow_storage_op_threshold = std::chrono::milliseconds(100);
config.cache_hit_rate_threshold = 80.0;
config.index_usage_threshold = 50.0;
config.write_amplification_threshold = 10.0;
config.read_amplification_threshold = 5.0;
config.max_full_scan_threshold = 1000;
config.analyze_queries = true;
config.analyze_storage = true;
config.analyze_cache = true;
config.analyze_indexes = true;
config.generate_recommendations = true;
```

## Best Practices

### When to Profile

- During development to catch performance issues early
- When experiencing performance degradation
- Before and after optimization changes
- Periodically in production (with sampling)

### Profiling Overhead

- Query profiling: ~1-2% overhead
- Storage profiling: ~2-3% overhead
- Recommended to use sampling in high-load production

### Analysis Frequency

- Run analysis daily or weekly
- After significant schema/query changes
- When SLAs are not being met

### Integration with Monitoring

- Export metrics to Prometheus/Grafana
- Set up alerts for:
  - Slow query count > threshold
  - Cache hit rate < threshold
  - Write amplification > threshold
  - Storage operation latency > threshold

## Example Workflows

### Workflow 1: Optimize a Slow Query

```bash
# 1. Enable profiling
./tools/themis_profiler.py enable

# 2. Run the query
# ... execute query via your application ...

# 3. Find slow queries
./tools/themis_profiler.py slow-queries --threshold 500

# 4. Get detailed profile
./tools/themis_profiler.py queries --verbose | grep "query-id"

# 5. Analyze and get recommendations
./tools/themis_profiler.py analyze

# 6. Disable profiling
./tools/themis_profiler.py disable
```

### Workflow 2: Tune RocksDB Settings

```bash
# 1. Collect baseline metrics
./tools/themis_profiler.py storage > baseline.txt

# 2. Run workload

# 3. Check amplification
./tools/themis_profiler.py storage | grep amplification

# 4. Adjust RocksDB config based on recommendations

# 5. Compare before/after
./tools/themis_profiler.py storage > after.txt
diff baseline.txt after.txt
```

### Workflow 3: Continuous Monitoring

```bash
# Start real-time monitor
./tools/themis_profiler.py monitor --interval 5

# OR set up cron job for periodic analysis
0 */6 * * * /path/to/themis_profiler.py analyze -o /var/log/themis/analysis-$(date +\%Y\%m\%d-\%H\%M).json
```

## Troubleshooting

### Profiling Not Working

1. Check if profiling is enabled: `./tools/themis_profiler.py queries`
2. Verify API endpoint is accessible: `curl http://localhost:8080/api/profiling/queries`
3. Check server logs for errors

### High Memory Usage

1. Reduce `max_profiles_retained` in config
2. Decrease `retention_duration`
3. Disable operator stats collection if not needed

### Missing Statistics

1. Ensure profiling is enabled
2. Run queries to generate profile data
3. Wait for stats collection interval

## Future Enhancements

- [ ] Distributed tracing integration (OpenTelemetry)
- [ ] GPU profiling for vector operations
- [ ] ML-based anomaly detection
- [ ] Automated performance regression testing
- [ ] Cost-based optimizer statistics
- [ ] Real-time alerting
- [ ] Grafana dashboard templates

## References

- [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)
- [Query Optimization Best Practices](docs/de/performance/performance_optimization.md)
- [Monitoring Guide](docs/de/observability/observability_prometheus.md)
