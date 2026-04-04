# Data Compression and Encoding Strategies

## Overview

ThemisDB implements a comprehensive data compression and encoding system that automatically selects optimal compression methods based on data characteristics. The system provides multiple compression algorithms with adaptive selection, performance tracking, and seamless integration with the storage layer.

## Features

- **Multiple Compression Methods**: ZSTD, RLE, Delta, Dictionary, and Sparse encoding
- **Adaptive Selection**: Automatically chooses the best method based on data type
- **Performance Metrics**: Tracks compression ratios, throughput, and timing
- **Lossless Compression**: All methods preserve data integrity 100%
- **Configurable**: Flexible configuration for different use cases
- **Efficient**: Minimal overhead for small data, excellent ratios for large data

## Compression Methods

### 1. ZSTD (Zstandard)

**Best for**: General-purpose compression, text, JSON, structured data

**Characteristics**:
- Compression levels 1-22 (3 is default)
- Excellent compression ratios (typically 2-5x)
- Fast decompression
- Industry-standard algorithm

**Use case example**:
```cpp
CompressionConfig config;
config.method = CompressionMethod::ZSTD;
config.level = 3;  // Balance between speed and ratio

CompressionStrategyManager manager(config);
auto result = manager.compress(data);
```

### 2. RLE (Run-Length Encoding)

**Best for**: Data with long runs of repeated values

**Characteristics**:
- Extremely fast encoding/decoding
- Excellent for sparse or repetitive data (10-100x compression)
- Poor for random data
- Variable-length integer encoding for counts

**Use case example**:
```cpp
// Good for sparse vectors, binary masks, homogeneous data
std::vector<uint8_t> sparse_data(10000, 0);  // Mostly zeros
sparse_data[100] = 255;
sparse_data[5000] = 128;

CompressionConfig config;
config.method = CompressionMethod::RLE;
CompressionStrategyManager manager(config);
auto result = manager.compress(sparse_data);  // ~99% compression
```

### 3. Delta Encoding

**Best for**: Sequential or slowly-changing data, time series

**Characteristics**:
- Stores first value + differences
- Excellent for monotonic sequences (3-10x compression)
- Fast encoding/decoding
- Ideal for timestamps, counters, IDs

**Use case example**:
```cpp
// Good for time series, sequential IDs, counters
std::vector<uint8_t> sequential_data;
for (int i = 0; i < 1000; ++i) {
    sequential_data.push_back(i % 256);
}

CompressionConfig config;
config.method = CompressionMethod::DELTA;
CompressionStrategyManager manager(config);
auto result = manager.compress(sequential_data);
```

### 4. Dictionary Encoding

**Best for**: Categorical data with few unique values

**Characteristics**:
- Creates dictionary of unique values + indices
- Excellent for categorical data (5-20x compression)
- Fast lookups
- Only effective with < 128 unique values

**Use case example**:
```cpp
// Good for enums, categories, tags
std::vector<uint8_t> categorical_data;
for (int i = 0; i < 1000; ++i) {
    categorical_data.push_back(i % 10);  // Only 10 unique values
}

CompressionConfig config;
config.method = CompressionMethod::DICTIONARY;
CompressionStrategyManager manager(config);
auto result = manager.compress(categorical_data);
```

### 5. Sparse CSR (Compressed Sparse Row)

**Best for**: Sparse vectors (>95% zeros)

**Characteristics**:
- Stores only non-zero values and their indices
- Excellent compression for sparse vectors (10-100x)
- 100% lossless
- Integrated with vector index systems

**Use case example**:
```cpp
#include "utils/lossless_vector_compression.h"

std::vector<float> sparse_vector(10000, 0.0f);
sparse_vector[42] = 1.5f;
sparse_vector[1337] = 2.7f;

auto compressed = themis::experimental::SparseVectorCodec::compress(sparse_vector);
auto decompressed = themis::experimental::SparseVectorCodec::decompress(compressed);
```

## Adaptive Compression

The system can automatically select the best compression method based on data characteristics:

```cpp
CompressionConfig config;
config.method = CompressionMethod::ADAPTIVE;  // Auto-select
config.data_type = DataType::GENERIC;         // Auto-detect type

CompressionStrategyManager manager(config);

// System will:
// 1. Sample the data
// 2. Detect characteristics (text? sparse? sequential?)
// 3. Select optimal method
// 4. Apply compression
auto result = manager.compress(data);

std::cout << "Selected method: " 
          << CompressionStrategyManager::method_to_string(result.method_used) 
          << std::endl;
std::cout << "Compression ratio: " << result.compression_ratio << "x" << std::endl;
```

## Data Type Hints

Provide hints for better compression selection:

```cpp
// Text data
auto result = manager.compress(text_data, DataType::TEXT);

// JSON documents
auto result = manager.compress(json_data, DataType::JSON);

// Dense vectors
auto result = manager.compress(vector_data, DataType::VECTOR_DENSE);

// Sparse vectors
auto result = manager.compress(sparse_vector, DataType::VECTOR_SPARSE);

// Integer sequences
auto result = manager.compress(int_sequence, DataType::INTEGER_SEQ);

// Categorical data
auto result = manager.compress(categories, DataType::CATEGORICAL);

// Time series
auto result = manager.compress(timeseries, DataType::TIMESERIES);
```

## Configuration Options

### Compression Level

Controls compression quality vs speed tradeoff (ZSTD only):

```cpp
CompressionConfig config;
config.level = 1;   // Fastest compression, lower ratio
config.level = 3;   // Default: balanced
config.level = 9;   // Better compression, slower
config.level = 22;  // Maximum compression, much slower
```

### Minimum Size

Don't compress data smaller than threshold:

```cpp
CompressionConfig config;
config.min_size = 128;  // Skip compression for < 128 bytes
```

### Sparse Threshold

Control sparse detection sensitivity:

```cpp
CompressionConfig config;
config.sparse_threshold = 0.95f;  // Require 95%+ zeros for sparse compression
```

### Metrics Tracking

Enable/disable performance metrics:

```cpp
CompressionConfig config;
config.enable_metrics = true;  // Track performance (default)
config.enable_metrics = false; // Disable for production workloads
```

## Performance Metrics

Track compression performance across all operations:

```cpp
CompressionStrategyManager manager;

// Perform compressions
for (auto& data : dataset) {
    auto result = manager.compress(data);
    auto decompressed = manager.decompress(result.data, result.method_used);
}

// Get metrics summary
std::string summary = manager.get_metrics();
std::cout << summary << std::endl;

// Output example:
// === Compression Metrics Summary ===
// 
// Method: zstd
//   Compression Operations: 1000
//   Decompression Operations: 1000
//   Total Bytes In: 10485760 (10.00 MB)
//   Total Bytes Out: 3145728 (3.00 MB)
//   Compression Ratio: 3.33x
//   Avg Compression Time: 1.234 ms
//   Avg Decompression Time: 0.567 ms
//   Compression Throughput: 123.45 MB/s
```

## Integration with Storage

### Direct Integration

```cpp
#include "storage/compression_strategy.h"

class MyStorage {
    CompressionStrategyManager compressor_;
    
public:
    void store(const std::string& key, const std::vector<uint8_t>& data) {
        // Compress before storing
        auto result = compressor_.compress(data);
        
        // Store compressed data + method metadata
        storage_backend_.put(key + ":data", result.data);
        storage_backend_.put(key + ":method", 
            CompressionStrategyManager::method_to_string(result.method_used));
    }
    
    std::vector<uint8_t> retrieve(const std::string& key) {
        // Load compressed data + method
        auto compressed = storage_backend_.get(key + ":data");
        auto method_str = storage_backend_.get(key + ":method");
        
        // Decompress
        auto method = CompressionStrategyManager::string_to_method(method_str).value();
        return compressor_.decompress(compressed, method);
    }
};
```

### Column-Store Integration

```cpp
// Different compression for different column types
class ColumnStore {
    std::unordered_map<std::string, CompressionConfig> column_configs_;
    
public:
    void configure_column(const std::string& column, DataType type) {
        CompressionConfig config;
        config.method = CompressionMethod::ADAPTIVE;
        config.data_type = type;
        column_configs_[column] = config;
    }
    
    void store_column(const std::string& column, const std::vector<uint8_t>& data) {
        CompressionStrategyManager manager(column_configs_[column]);
        auto result = manager.compress(data);
        // Store result...
    }
};
```

## Best Practices

### 1. Use Adaptive Compression by Default

Unless you know your data characteristics, use adaptive:

```cpp
CompressionConfig config;
config.method = CompressionMethod::ADAPTIVE;
```

### 2. Provide Data Type Hints When Possible

Help the system make better decisions:

```cpp
auto result = manager.compress(data, DataType::TEXT);
```

### 3. Set Appropriate Minimum Size

Avoid overhead for small data:

```cpp
config.min_size = 256;  // Don't compress < 256 bytes
```

### 4. Monitor Metrics in Development

Track performance during development:

```cpp
config.enable_metrics = true;
// ... run workload ...
std::cout << manager.get_metrics() << std::endl;
```

### 5. Disable Metrics in Production

Reduce overhead in production:

```cpp
config.enable_metrics = false;
```

### 6. Choose Compression Level Wisely

Higher levels = better compression but slower:

```cpp
// For write-heavy workloads
config.level = 1;  // Fast compression

// For read-heavy workloads
config.level = 9;  // Better compression, less storage, faster reads
```

## Performance Characteristics

### Compression Speed

| Method     | Speed      | Best Use Case                |
|------------|------------|------------------------------|
| NONE       | Instant    | Small data (< min_size)      |
| RLE        | Very Fast  | Repetitive data              |
| DELTA      | Very Fast  | Sequential data              |
| DICTIONARY | Fast       | Categorical data             |
| ZSTD L1    | Fast       | General purpose              |
| ZSTD L3    | Medium     | Balanced (default)           |
| ZSTD L9    | Slow       | Read-heavy workloads         |

### Compression Ratios

| Method     | Typical Ratio | Data Characteristics         |
|------------|---------------|------------------------------|
| RLE        | 10-100x       | Sparse/repetitive            |
| DELTA      | 3-10x         | Sequential/monotonic         |
| DICTIONARY | 5-20x         | Categorical (< 128 unique)   |
| ZSTD       | 2-5x          | Text, JSON, structured       |
| SPARSE_CSR | 10-100x       | Sparse vectors (>95% zeros)  |

## Error Handling

The system is designed to be robust:

```cpp
auto result = manager.compress(data);

if (result.success) {
    // Compression succeeded
    std::cout << "Ratio: " << result.compression_ratio << "x" << std::endl;
} else {
    // Compression failed (rare)
    // Data is stored uncompressed in result.data
    std::cout << "Compression failed, stored uncompressed" << std::endl;
}

// Decompression returns empty vector on failure
auto decompressed = manager.decompress(compressed, method);
if (decompressed.empty()) {
    // Handle decompression failure
    std::cerr << "Decompression failed!" << std::endl;
}
```

## Thread Safety

The compression metrics tracker is thread-safe:

```cpp
// Multiple threads can safely use the same metrics instance
CompressionMetrics::instance().record_compression(...);
```

Individual `CompressionStrategyManager` instances are NOT thread-safe. Use one per thread:

```cpp
// Good: One manager per thread
thread_local CompressionStrategyManager thread_manager;

// Bad: Sharing manager across threads (without locking)
static CompressionStrategyManager shared_manager;  // Not safe!
```

## Examples

### Example 1: Compressing Log Files

```cpp
CompressionConfig config;
config.method = CompressionMethod::ZSTD;
config.level = 9;  // Logs are write-once, read-maybe
config.min_size = 1024;

CompressionStrategyManager manager(config);

std::string log_data = load_log_file("app.log");
auto result = manager.compress(log_data, DataType::TEXT);

std::cout << "Original: " << log_data.size() << " bytes" << std::endl;
std::cout << "Compressed: " << result.data.size() << " bytes" << std::endl;
std::cout << "Ratio: " << result.compression_ratio << "x" << std::endl;
```

### Example 2: Vector Database Storage

```cpp
// Configure for vector storage
CompressionConfig config;
config.method = CompressionMethod::ADAPTIVE;
config.sparse_threshold = 0.95f;

CompressionStrategyManager manager(config);

// Detect sparsity and compress accordingly
std::vector<float> embedding = get_embedding();
auto sparse_compressed = themis::experimental::SparseVectorCodec::compress(embedding);

if (sparse_compressed.compressed_bytes() < embedding.size() * sizeof(float) * 0.5) {
    // Store as sparse
    store_vector_sparse(sparse_compressed);
} else {
    // Store as dense with ZSTD
    std::vector<uint8_t> dense_bytes(
        reinterpret_cast<uint8_t*>(embedding.data()),
        reinterpret_cast<uint8_t*>(embedding.data() + embedding.size())
    );
    auto result = manager.compress(dense_bytes);
    store_vector_compressed(result);
}
```

### Example 3: Time Series Data

```cpp
// Time series with Delta encoding
CompressionConfig config;
config.method = CompressionMethod::DELTA;

CompressionStrategyManager manager(config);

std::vector<uint8_t> timestamps;
// Convert timestamps to bytes
for (auto ts : get_timestamps()) {
    // Store as bytes
    uint64_t t = ts.time_since_epoch().count();
    for (int i = 0; i < 8; ++i) {
        timestamps.push_back((t >> (i * 8)) & 0xFF);
    }
}

auto result = manager.compress(timestamps);
// Delta encoding will compress sequential timestamps very well
```

## API Reference

### CompressionStrategyManager

Main class for compression operations.

**Methods**:
- `compress(data, hint)`: Compress data with optional type hint
- `decompress(data, method)`: Decompress data
- `select_method(data, size, type)`: Select optimal method
- `get_metrics()`: Get performance metrics summary
- `reset_metrics()`: Clear all metrics
- `set_config(config)`: Update configuration
- `method_to_string(method)`: Convert method enum to string
- `string_to_method(str)`: Convert string to method enum

### CompressionConfig

Configuration structure.

**Fields**:
- `method`: Compression method (default: ADAPTIVE)
- `data_type`: Data type hint (default: GENERIC)
- `level`: Compression level 1-22 (default: 3)
- `min_size`: Minimum size to compress (default: 128)
- `sparse_threshold`: Sparse detection threshold (default: 0.95)
- `enable_metrics`: Track metrics (default: true)

### CompressionResult

Result of compression operation.

**Fields**:
- `data`: Compressed data
- `method_used`: Method that was used
- `original_size`: Original data size
- `compression_ratio`: Achieved ratio
- `success`: Whether compression succeeded

## See Also

- `include/utils/zstd_codec.h` - ZSTD compression wrapper
- `include/utils/lossless_vector_compression.h` - Vector compression utilities
- `include/utils/compression_metrics.h` - Performance metrics tracking
- `tests/test_compression_strategy.cpp` - Comprehensive test suite
