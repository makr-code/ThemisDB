# HuggingFace Ingestion Plugin

A standalone plugin for ThemisDB that fetches datasets from HuggingFace Hub and ingests them into the database.

## Quick Start

```cpp
#include "plugins/huggingface_ingestion_plugin.h"
#include "content/async_ingestion_worker.h"

// Configure plugin
HuggingFaceIngestionPlugin::Config config;
config.dataset_name = "lexlms/ger_legal_data";
config.split = "train";

// Create plugin and register with worker
auto plugin = std::make_shared<HuggingFaceIngestionPlugin>(
    config, content_manager
);

AsyncIngestionWorker worker(content_manager);
plugin->registerWithWorker(worker);
worker.start();
```

## Features

- ✅ REST API integration with HuggingFace Datasets Server
- ✅ Streaming support for large datasets
- ✅ Local file-based caching
- ✅ Rate limiting (configurable requests/second)
- ✅ Automatic retry with exponential backoff
- ✅ Progress tracking
- ✅ Flexible schema mapping

## Documentation

- **Full Documentation**: [docs/plugins/HUGGINGFACE_INGESTION.md](../../docs/plugins/HUGGINGFACE_INGESTION.md)
- **Example Code**: [examples/huggingface_ingestion_example.cpp](../../examples/huggingface_ingestion_example.cpp)
- **Configuration**: [config/plugins/huggingface.yaml](../../config/plugins/huggingface.yaml)

## Configuration

See [plugin.json](plugin.json) for the full schema.

Basic configuration:

```yaml
huggingface:
  enabled: true
  defaults:
    streaming: true
    chunk_size: 1000
    cache_dir: "./cache/huggingface"
    use_cache: true
    max_requests_per_second: 10
```

## Dependencies

- libcurl (already in ThemisDB)
- nlohmann/json (already in ThemisDB)

## Testing

```bash
# Run unit tests
./build/tests/test_huggingface_plugin

# Run with network tests (requires internet)
./build/tests/test_huggingface_plugin --gtest_also_run_disabled_tests
```

## Use Cases

Primary use case: Ingest large legal datasets for legal AI training.

Example datasets:
- `lexlms/ger_legal_data` - 12 GB German legal corpus
- `pile-of-law/pile-of-law` - US legal corpus
- `imdb` - Movie reviews (for testing)

## Architecture

```
AsyncIngestionWorker
    ↓
HuggingFaceIngestionPlugin
    ↓ (registers HUGGINGFACE job type)
    ↓
ContentManager::importContent()
```

## Performance

- **Throughput**: >1000 documents/second (with caching)
- **Memory**: <500 MB (streaming mode)
- **Cache hit**: <100ms

## Status

✅ **Ready for use** - All core features implemented and tested.

## Contributing

When contributing:
1. Add tests for new features
2. Update documentation
3. Follow ThemisDB coding standards
4. Respect HuggingFace API rate limits

## License

Part of ThemisDB - Same license as main project.
