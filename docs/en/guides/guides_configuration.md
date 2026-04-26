# Configuration Guide

For comprehensive configuration documentation, please see:

## 📘 Complete Configuration Documentation

### Configuration File Reference

**[Complete Configuration Reference](../../../config/config.yaml)**

This file contains:
- All available configuration parameters
- Detailed comments explaining each option
- Default values and recommendations
- Example configurations for different use cases

### Comprehensive Tuning Guide (German)

**[Konfigurations- und Tuning-Guide (DE)](../../de/guides/CONFIGURATION_TUNING_GUIDE.md)**

This comprehensive guide covers:
- Configuration file formats and locations
- Storage tuning (RocksDB memory, write, read, compression)
- Server tuning (threads, connections, TLS, HTTP/2)
- Vector index tuning (HNSW parameters, GPU acceleration)
- Memory management and planning
- I/O optimization strategies
- Feature configuration
- Use-case-specific tuning examples
- Hardware-specific configurations
- Monitoring and troubleshooting
- Production deployment checklist

## Quick Start

### Basic Configuration

Create a `config.yaml` file:

```yaml
storage:
  rocksdb_path: "./data/rocksdb"
  memtable_size_mb: 256
  block_cache_size_mb: 1024

server:
  host: "0.0.0.0"
  port: 8765
  worker_threads: 0  # auto-detect

vector_index:
  object_name: "documents"
  dimension: 768
  metric: "COSINE"
```

### Using Pre-configured Files

ThemisDB includes optimized configurations for common hardware:

```bash
# Raspberry Pi 5 (8GB RAM)
cp config/config.rpi5.json config/config.json

# Raspberry Pi 4 (4GB RAM)
cp config/config.rpi4.json config/config.json

# Raspberry Pi 3 (2GB RAM)
cp config/config.rpi3.json config/config.json

# QNAP NAS
cp config/config.qnap.json config/config.json
```

### Specifying Configuration File

```bash
# Default search paths:
# 1. ./config.yaml
# 2. ./config/config.yaml
# 3. /etc/vccdb/config.yaml

# Or specify custom path:
./themis_server --config /path/to/config.yaml
```

## Configuration Sections

### Storage (RocksDB)

Controls database storage engine behavior:
- Memory allocation (memtable, block cache)
- Write performance (WAL, pipelining, concurrent writes)
- Compaction settings (background jobs, levels)
- Compression (lz4, zstd, none)
- BlobDB for large values

### Server

Controls HTTP/gRPC server behavior:
- Network settings (host, port)
- Thread configuration
- Connection limits
- TLS/SSL configuration
- HTTP/2, WebSocket settings

### Vector Index

Controls vector search behavior:
- HNSW parameters (M, ef_construction, ef_search)
- GPU acceleration
- Persistence settings

### Features

Enable/disable optional features:
- Semantic cache
- LLM store
- CDC (Change Data Capture)
- Time-series support
- Data retention policies

### Tracing

OpenTelemetry tracing configuration:
- Enable/disable distributed tracing
- Service name
- OTLP endpoint

## Performance Tuning

### By Workload

**Write-Heavy:**
- Increase `memtable_size_mb`
- Enable `enable_pipelined_write`
- Increase `max_background_jobs`

**Read-Heavy:**
- Increase `block_cache_size_mb`
- Increase `bloom_bits_per_key`
- Enable `cache_index_and_filter_blocks`

**Balanced:**
- Use default settings
- Adjust based on monitoring

### By Hardware

**Low Memory (2-4 GB):**
- `memtable_size_mb: 64`
- `block_cache_size_mb: 256`
- `worker_threads: 4`

**Medium Memory (8-16 GB):**
- `memtable_size_mb: 256`
- `block_cache_size_mb: 1024`
- `worker_threads: 0` (auto)

**High Memory (32+ GB):**
- `memtable_size_mb: 512-1024`
- `block_cache_size_mb: 4096+`
- `enable_high_parallel_tuning: true`

## Related Documentation

- [German Tuning Guide](../../de/guides/CONFIGURATION_TUNING_GUIDE.md) - Comprehensive tuning guide
- [Raspberry Pi Tuning](../../de/deployment/deployment_raspberry_tuning.md) - ARM-specific optimizations
- [Performance Tuning](../../de/search/performance_tuning.md) - Search performance tuning
- [Deployment Guide](guides_deployment.md) - Deployment best practices
- [Administrator Guide](ADMINISTRATOR_GUIDE.md) - Administration tasks
- [Quick Start Guide](QUICK_START.md) - Getting started

## Examples

### Example 1: Low-Memory Device (Raspberry Pi 3)

```yaml
storage:
  memtable_size_mb: 64
  block_cache_size_mb: 256
  max_background_jobs: 2
  compression:
    default: "lz4"
    bottommost: "zstd"

server:
  worker_threads: 4
  max_connections: 50

vector_index:
  hnsw_m: 12
  max_elements: 50000

features:
  semantic_cache: false
  cdc: false
```

### Example 2: High-Performance Server

```yaml
storage:
  memtable_size_mb: 1024
  block_cache_size_mb: 8192
  max_background_jobs: 16
  enable_high_parallel_tuning: true
  enable_pipelined_write: true
  compression:
    default: "lz4"
    bottommost: "zstd"

server:
  worker_threads: 0  # auto-detect
  max_connections: 500

vector_index:
  hnsw_m: 64
  hnsw_ef_construction: 800
  ef_search: 200
  use_gpu: true
  max_elements: 10000000

features:
  semantic_cache: true
  llm_store: true
  cdc: true
```

### Example 3: Vector Search Optimized

```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 2048

server:
  worker_threads: 0
  max_connections: 100

vector_index:
  dimension: 768
  hnsw_m: 32
  hnsw_ef_construction: 400
  ef_search: 100
  use_gpu: true
  max_elements: 1000000

features:
  semantic_cache: true
  llm_store: true
```

## Support

For configuration questions or performance issues:

1. Check metrics: `curl http://localhost:8765/stats`
2. Review logs: `tail -f logs/themis_server.log`
3. Consult the comprehensive guide: [CONFIGURATION_TUNING_GUIDE.md](../../de/guides/CONFIGURATION_TUNING_GUIDE.md)
4. Open an issue: https://github.com/makr-code/ThemisDB/issues
