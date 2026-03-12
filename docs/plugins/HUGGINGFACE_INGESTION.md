# HuggingFace Ingestion Plugin

## Overview

The HuggingFace Ingestion Plugin enables ThemisDB to fetch and ingest datasets directly from HuggingFace Hub. This plugin integrates seamlessly with ThemisDB's existing `AsyncIngestionWorker` architecture to provide background processing of large datasets.

## Features

- **HuggingFace API Integration**: Fetch datasets via REST API
- **Streaming Support**: Handle large datasets efficiently
- **Local Caching**: Cache downloaded datasets to avoid redundant API calls
- **Rate Limiting**: Respect HuggingFace API rate limits
- **Automatic Retry**: Exponential backoff for network errors
- **Progress Tracking**: Monitor ingestion progress in real-time
- **Flexible Schema Mapping**: Configure field mappings for different datasets

## Architecture

The plugin extends ThemisDB's ingestion system by adding a new `HUGGINGFACE` job type:

```
┌─────────────────────────────────────────────────┐
│  AsyncIngestionWorker                           │
├─────────────────────────────────────────────────┤
│  • SINGLE_FILE                                  │
│  • ARCHIVE                                      │
│  • BATCH_FILES                                  │
│  • HUGGINGFACE (new)                           │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│  HuggingFaceIngestionPlugin                     │
├─────────────────────────────────────────────────┤
│  • Fetch from HuggingFace Hub                   │
│  • Parse dataset schema                         │
│  • Convert to ThemisDB format                   │
│  • Cache locally                                │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│  ContentManager::importContent()                │
└─────────────────────────────────────────────────┘
```

## Installation

### Prerequisites

- libcurl (already included in ThemisDB)
- nlohmann/json (already included in ThemisDB)

### Building

The plugin is built as part of ThemisDB. No additional build steps are required.

## Configuration

### Basic Configuration

```cpp
HuggingFaceIngestionPlugin::Config config;

// Dataset selection
config.dataset_name = "lexlms/ger_legal_data";
config.split = "train";

// Performance tuning
config.streaming = true;
config.chunk_size = 1000;  // Rows per API request

// Authentication (for private datasets)
config.auth_token = "hf_...";

// Schema mapping
config.text_field = "text";
config.label_field = "label";

// Caching
config.cache_dir = "./cache/huggingface";
config.use_cache = true;

// Rate limiting
config.max_requests_per_second = 10;

// Retry behavior
config.max_retries = 3;
config.retry_delay_ms = 1000;
```

### YAML Configuration

Create `config/plugins/huggingface.yaml`:

```yaml
huggingface:
  enabled: true
  
  defaults:
    streaming: true
    chunk_size: 1000
    cache_dir: "./cache/huggingface"
    use_cache: true
    max_requests_per_second: 10
    max_retries: 3
    retry_delay_ms: 1000
  
  presets:
    legal_german:
      dataset_name: "lexlms/ger_legal_data"
      split: "train"
      text_field: "text"
      label_field: "label"
      
    legal_us:
      dataset_name: "pile-of-law/pile-of-law"
      split: "train"
      text_field: "text"
```

## Usage

### Basic Example

```cpp
#include "plugins/huggingface_ingestion_plugin.h"
#include "content/async_ingestion_worker.h"

// 1. Setup ThemisDB components
auto content_manager = std::make_shared<ContentManager>(...);

// 2. Configure plugin
HuggingFaceIngestionPlugin::Config config;
config.dataset_name = "lexlms/ger_legal_data";
config.split = "train";
config.chunk_size = 1000;

// 3. Create plugin
auto plugin = std::make_shared<HuggingFaceIngestionPlugin>(
    config, content_manager
);

// 4. Setup worker
AsyncIngestionWorker worker(content_manager);
plugin->registerWithWorker(worker);
worker.start();

// 5. Submit job (Note: requires API extension)
// auto job_id = plugin->submitDatasetJob("lexlms/ger_legal_data");

// 6. Monitor progress
// auto status = worker.getJobStatus(job_id);
// std::cout << "Progress: " << (status->progress * 100) << "%\n";

// 7. Cleanup
worker.stop(true);
```

### Fetching Dataset Metadata

```cpp
auto metadata = plugin->getDatasetMetadata("lexlms/ger_legal_data");

std::cout << "Dataset: " << metadata.dataset_id << "\n";
std::cout << "Total rows: " << metadata.total_rows << "\n";
std::cout << "Splits: ";
for (const auto& split : metadata.splits) {
    std::cout << split << " ";
}
std::cout << "\n";
```

### Custom Schema Mapping

```cpp
config.text_field = "content";  // Use "content" field as text
config.label_field = "category";  // Use "category" field as label

// Map additional fields
config.custom_fields["author"] = "metadata.author";
config.custom_fields["date"] = "metadata.created_at";
```

## API Reference

### HuggingFaceIngestionPlugin Class

#### Constructor

```cpp
HuggingFaceIngestionPlugin(
    const Config& config,
    std::shared_ptr<ContentManager> content_manager
);
```

Creates a new plugin instance with the specified configuration.

#### registerWithWorker

```cpp
void registerWithWorker(AsyncIngestionWorker& worker);
```

Registers the plugin with an `AsyncIngestionWorker` instance. This enables the worker to process `HUGGINGFACE` job types.

#### submitDatasetJob

```cpp
std::string submitDatasetJob(
    const std::string& dataset_name,
    const std::string& split = "",
    const json& config = json::object()
);
```

Submits a dataset ingestion job. Returns a job ID for tracking.

**Note**: Current implementation requires API extension to `AsyncIngestionWorker` for full functionality.

#### getDatasetMetadata

```cpp
DatasetMetadata getDatasetMetadata(const std::string& dataset_name);
```

Fetches metadata about a dataset from HuggingFace Hub.

#### estimateDatasetSize

```cpp
size_t estimateDatasetSize(const std::string& dataset_name);
```

Estimates the total number of rows in a dataset.

### Configuration Structure

```cpp
struct Config {
    std::string dataset_name;
    std::string split = "train";
    bool streaming = true;
    size_t chunk_size = 1000;
    std::string auth_token;
    
    std::string text_field = "text";
    std::string label_field = "label";
    std::map<std::string, std::string> custom_fields;
    
    std::string cache_dir = "./cache/huggingface";
    bool use_cache = true;
    
    size_t max_requests_per_second = 10;
    size_t max_retries = 3;
    size_t retry_delay_ms = 1000;
};
```

### Dataset Metadata Structure

```cpp
struct DatasetMetadata {
    std::string dataset_id;
    std::string description;
    size_t total_rows;
    std::vector<std::string> splits;
    std::map<std::string, std::string> columns;
};
```

## Performance

### Tuning Guidelines

| Parameter | Recommended | Notes |
|-----------|-------------|-------|
| `chunk_size` | 500-2000 | Larger = fewer API calls, more memory |
| `max_requests_per_second` | 5-10 | Respect HF rate limits |
| `worker_thread_count` | 2-4 | Balance throughput and resource usage |
| `use_cache` | true | Essential for large datasets |

### Performance Targets

- **Throughput**: >1000 documents/second (with caching)
- **Memory**: <500 MB for streaming mode
- **Network**: Automatic retry with exponential backoff
- **Cache hit**: <100ms for cached datasets

## Caching

The plugin implements a local file-based cache to avoid redundant API calls.

### Cache Structure

```
cache_dir/
├── lexlms_ger_legal_data_train.json
├── lexlms_ger_legal_data_test.json
└── imdb_train.json
```

### Cache Management

```cpp
// Enable caching
config.use_cache = true;
config.cache_dir = "./cache/huggingface";

// Cache is automatically populated on first fetch
// Subsequent fetches load from cache

// To clear cache
std::filesystem::remove_all(config.cache_dir);
```

## Rate Limiting

The plugin implements token bucket rate limiting to respect HuggingFace API limits.

```cpp
// Limit to 10 requests per second
config.max_requests_per_second = 10;

// Disable rate limiting
config.max_requests_per_second = 0;
```

## Error Handling

### Automatic Retry

Network errors trigger automatic retry with exponential backoff:

```cpp
config.max_retries = 3;  // Try up to 3 times
config.retry_delay_ms = 1000;  // Start with 1s delay

// Retry delays: 1s, 2s, 4s
```

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "Failed to initialize CURL" | libcurl not available | Check dependencies |
| "HTTP error 404" | Dataset not found | Verify dataset name |
| "HTTP error 429" | Rate limited | Reduce `max_requests_per_second` |
| "dataset_name not specified" | Missing config | Set `dataset_name` in config |

## Example Datasets

### Legal Datasets

```cpp
// German legal corpus (12 GB)
config.dataset_name = "lexlms/ger_legal_data";
config.split = "train";

// US legal corpus
config.dataset_name = "pile-of-law/pile-of-law";
config.split = "train";
```

### Small Test Datasets

```cpp
// IMDB reviews (for testing)
config.dataset_name = "imdb";
config.split = "train";

// Squad QA (for testing)
config.dataset_name = "squad";
config.split = "train";
```

## Troubleshooting

### Plugin not processing jobs

**Problem**: Jobs are submitted but never processed.

**Solution**: Ensure plugin is registered with worker before starting:

```cpp
plugin->registerWithWorker(worker);
worker.start();  // Must be called AFTER registration
```

### Out of memory errors

**Problem**: Memory usage spikes with large datasets.

**Solution**: Reduce `chunk_size` and enable streaming:

```cpp
config.streaming = true;
config.chunk_size = 500;  // Reduce batch size
```

### Rate limit errors (HTTP 429)

**Problem**: Too many API requests.

**Solution**: Reduce request rate and enable caching:

```cpp
config.max_requests_per_second = 5;  // Slower
config.use_cache = true;  // Avoid redundant requests
```

### Cache corruption

**Problem**: Cached data is invalid or corrupted.

**Solution**: Clear cache and refetch:

```bash
rm -rf ./cache/huggingface
```

## Future Enhancements

Potential improvements for future versions:

1. **Direct Job Submission**: Extend `AsyncIngestionWorker` with `submitCustomJob()` API
2. **Incremental Updates**: Detect and fetch only new rows
3. **Parallel Fetching**: Fetch multiple batches concurrently
4. **Compression**: Store cached data with zstd compression
5. **Authentication**: Support for private datasets and organizations
6. **Dataset Versioning**: Track and handle dataset versions
7. **Metrics**: Prometheus metrics for monitoring
8. **Resume Support**: Resume interrupted downloads

## Contributing

When contributing to this plugin:

1. Maintain backward compatibility
2. Add tests for new features
3. Update documentation
4. Follow ThemisDB coding standards
5. Respect API rate limits in tests

## License

This plugin is part of ThemisDB and follows the same license.

## Support

For issues and questions:

- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Documentation: https://themisdb.com/docs/plugins/huggingface

## Changelog

### Version 1.0.0 (February 2026)

- Initial release
- HuggingFace API integration
- Streaming support
- Local caching
- Rate limiting
- Automatic retry
- AsyncIngestionWorker integration
