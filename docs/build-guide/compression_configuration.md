# Compression and Encoding Configuration Guide

## Overview

This guide explains how to configure data compression and encoding strategies in ThemisDB.

## Configuration Files

ThemisDB supports configuration through JSON or YAML files. Compression settings can be specified at different levels:

### Global Compression Configuration

Configure default compression for all storage operations:

```json
{
  "storage": {
    "compression": {
      "enabled": true,
      "method": "adaptive",
      "level": 3,
      "min_size": 128,
      "sparse_threshold": 0.95,
      "enable_metrics": true
    }
  }
}
```

```yaml
storage:
  compression:
    enabled: true
    method: adaptive
    level: 3
    min_size: 128
    sparse_threshold: 0.95
    enable_metrics: true
```

### Compression Methods

Available compression methods:

- **`none`**: No compression (passthrough)
- **`zstd`**: Zstandard compression (general-purpose, excellent for text/JSON)
- **`lz4`**: LZ4 fast compression (future support)
- **`snappy`**: Snappy compression (future support)
- **`rle`**: Run-Length Encoding (best for repetitive/sparse data)
- **`delta`**: Delta encoding (best for sequential/monotonic data)
- **`dictionary`**: Dictionary encoding (best for categorical data)
- **`adaptive`**: Automatically select best method (recommended)

### Compression Levels

For ZSTD compression:

- **1**: Fastest compression, lower ratio
- **3**: Default, balanced speed/ratio
- **9**: Better compression, slower
- **22**: Maximum compression, much slower

### Column-Specific Compression

Configure different compression for different data types:

```json
{
  "storage": {
    "columns": {
      "documents": {
        "compression": {
          "method": "zstd",
          "level": 9,
          "data_type": "json"
        }
      },
      "vectors": {
        "compression": {
          "method": "adaptive",
          "sparse_threshold": 0.95,
          "data_type": "vector_sparse"
        }
      },
      "timeseries": {
        "compression": {
          "method": "delta",
          "data_type": "timeseries"
        }
      },
      "categories": {
        "compression": {
          "method": "dictionary",
          "data_type": "categorical"
        }
      }
    }
  }
}
```

### Data Type Hints

Supported data types for adaptive compression:

- **`generic`**: Unknown/generic binary data
- **`text`**: Text data (UTF-8 strings)
- **`json`**: JSON documents
- **`vector_dense`**: Dense float vectors
- **`vector_sparse`**: Sparse vectors (>95% zeros)
- **`integer_seq`**: Integer sequences
- **`categorical`**: Categorical/enum data
- **`timeseries`**: Time-series data

## Configuration Parameters

### Core Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `enabled` | bool | true | Enable/disable compression |
| `method` | string | "adaptive" | Compression method |
| `level` | int | 3 | Compression level (1-22 for ZSTD) |
| `min_size` | int | 128 | Minimum size to compress (bytes) |
| `sparse_threshold` | float | 0.95 | Sparsity threshold for sparse compression |
| `enable_metrics` | bool | true | Enable performance metrics tracking |
| `data_type` | string | "generic" | Data type hint for adaptive compression |

### Advanced Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `adaptive_ratio_threshold` | float | 1.2 | Minimum compression ratio to consider successful |
| `adaptive_sample_size` | int | 1024 | Bytes to sample for method selection |

## Programmatic Configuration

### C++ API

```cpp
#include "storage/compression_strategy.h"
#include "storage/compressed_storage.h"

using namespace themis::compression;
using namespace themis::storage;

// Configure compression
CompressionConfig config;
config.method = CompressionMethod::ADAPTIVE;
config.level = 3;
config.min_size = 256;
config.enable_metrics = true;

// Create compressed storage wrapper
auto backend = std::make_shared<MyStorageBackend>();
CompressedStorageWrapper storage(backend, config);

// Store with automatic compression
storage.put("key1", data, DataType::TEXT);
storage.put("key2", vector_data, DataType::VECTOR_SPARSE);

// Retrieve with automatic decompression
auto value = storage.get("key1");
```

### Column-Aware Storage

```cpp
ColumnCompressedStorage storage(backend);

// Configure different compression for each column
CompressionConfig text_config;
text_config.method = CompressionMethod::ZSTD;
text_config.level = 9;
storage.configure_column("documents", text_config);

CompressionConfig vector_config;
vector_config.method = CompressionMethod::ADAPTIVE;
vector_config.sparse_threshold = 0.95f;
storage.configure_column("embeddings", vector_config);

// Store in columns
storage.put("documents", "doc1", document_data, DataType::JSON);
storage.put("embeddings", "vec1", vector_data, DataType::VECTOR_SPARSE);
```

## Environment Variables

Override configuration via environment variables:

```bash
# Enable/disable compression
export THEMIS_COMPRESSION_ENABLED=true

# Set compression method
export THEMIS_COMPRESSION_METHOD=zstd

# Set compression level
export THEMIS_COMPRESSION_LEVEL=9

# Set minimum size
export THEMIS_COMPRESSION_MIN_SIZE=256

# Enable metrics
export THEMIS_COMPRESSION_METRICS=true
```

## Performance Tuning

### Write-Heavy Workloads

Optimize for write speed:

```json
{
  "compression": {
    "method": "lz4",
    "level": 1
  }
}
```

Or:

```json
{
  "compression": {
    "method": "rle"
  }
}
```

### Read-Heavy Workloads

Optimize for storage space and read performance:

```json
{
  "compression": {
    "method": "zstd",
    "level": 9
  }
}
```

### Mixed Workloads

Use adaptive compression:

```json
{
  "compression": {
    "method": "adaptive",
    "level": 3
  }
}
```

### Memory-Constrained Systems

Disable compression to save CPU:

```json
{
  "compression": {
    "enabled": false
  }
}
```

### High-Throughput Systems

Use fast compression with minimal overhead:

```json
{
  "compression": {
    "method": "lz4",
    "min_size": 4096
  }
}
```

## Monitoring and Metrics

### Enable Metrics Collection

```json
{
  "compression": {
    "enable_metrics": true
  }
}
```

### Access Metrics via API

```cpp
// Get compression statistics
std::string stats = storage.get_compression_stats();
std::cout << stats << std::endl;

// Reset metrics
storage.reset_compression_stats();
```

### Example Metrics Output

```
=== Compression Metrics Summary ===

Method: zstd
  Compression Operations: 1000
  Decompression Operations: 950
  Total Bytes In: 104857600 (100.00 MB)
  Total Bytes Out: 31457280 (30.00 MB)
  Compression Ratio: 3.33x
  Avg Compression Time: 1.234 ms
  Avg Decompression Time: 0.567 ms
  Compression Throughput: 123.45 MB/s

Method: rle
  Compression Operations: 500
  Decompression Operations: 500
  Total Bytes In: 52428800 (50.00 MB)
  Total Bytes Out: 524288 (0.50 MB)
  Compression Ratio: 100.00x
  Avg Compression Time: 0.123 ms
  Avg Decompression Time: 0.089 ms
  Compression Throughput: 987.65 MB/s
```

## Migration Guide

### Enabling Compression on Existing Data

Compression is applied only to new writes. To compress existing data:

1. Enable compression in configuration
2. Use the migration tool:

```bash
themis-admin migrate-compress \
  --source /data/themis \
  --method adaptive \
  --level 3
```

### Disabling Compression

To disable compression while preserving existing compressed data:

```json
{
  "compression": {
    "enabled": false
  }
}
```

Existing compressed data will be decompressed on read. New data will be stored uncompressed.

### Changing Compression Methods

Each value stores its compression method, so you can safely change configuration:

```json
{
  "compression": {
    "method": "zstd"  // Changed from "adaptive"
  }
}
```

- Old data: Decompressed using stored method
- New data: Compressed using new method

## Best Practices

### 1. Start with Adaptive Compression

```json
{
  "compression": {
    "method": "adaptive"
  }
}
```

### 2. Use Column-Specific Configuration

Different data types benefit from different compression:

```json
{
  "columns": {
    "logs": {"method": "zstd", "level": 9},
    "vectors": {"method": "adaptive"},
    "counters": {"method": "delta"},
    "tags": {"method": "dictionary"}
  }
}
```

### 3. Set Appropriate Minimum Size

Avoid overhead on small values:

```json
{
  "compression": {
    "min_size": 256
  }
}
```

### 4. Monitor Performance

Enable metrics during development:

```json
{
  "compression": {
    "enable_metrics": true
  }
}
```

Disable in production if not needed:

```json
{
  "compression": {
    "enable_metrics": false
  }
}
```

### 5. Tune for Your Workload

- **Analytics (OLAP)**: High compression level, large min_size
- **Transactional (OLTP)**: Low compression level or fast method
- **Time-Series**: Delta encoding
- **Document Store**: ZSTD with medium level
- **Vector Database**: Adaptive with sparse support

## Troubleshooting

### High CPU Usage

Reduce compression level or use faster method:

```json
{
  "compression": {
    "method": "lz4",
    "level": 1
  }
}
```

### Poor Compression Ratios

Increase compression level or try different method:

```json
{
  "compression": {
    "method": "zstd",
    "level": 9
  }
}
```

### Slow Queries

Check if decompression is the bottleneck. Consider:
- Reducing compression level
- Using faster method
- Increasing min_size to compress less frequently

### Out of Memory

Disable compression or increase min_size:

```json
{
  "compression": {
    "min_size": 4096
  }
}
```

## See Also

- [Compression and Encoding Strategies](../performance/compression_and_encoding_strategies.md)
- [Performance Tuning Guide](PERFORMANCE_PROFILING_GUIDE.md)
- [Storage Configuration](storage_configuration.md)
