**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [pipeline public headers](../../../include/content/pipeline/), [async uploader implementation](async_bulk_uploader.cpp), [async worker API](../../../include/content/async_ingestion_worker.h), [pipeline tests](../../../tests/content/test_content_pipeline.cpp), [test registration](../../../tests/content/CMakeLists.txt)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content Pipeline Module

## Overview

The Content Pipeline module provides infrastructure for efficient content processing in ThemisDB. This module integrates with existing ThemisDB infrastructure while providing a unified API for pipeline operations.

**Integration with Existing Infrastructure:**
- **ZSTD Compression**: Wraps `utils::zstd_codec` (fully functional implementation)
- **Content Chunking**: Complements `IContentProcessor::chunk()` with generic byte-based chunking
- **Bulk Upload**: Simplified interface, can integrate with `AsyncIngestionWorker` for production

## Components

### ZstdCompression

Provides compression and decompression capabilities using ThemisDB's existing ZSTD implementation.

**Integration**: Delegates to `utils::zstd_compress()` and `utils::zstd_decompress()`

**Features**:
- Uses existing production-ready ZSTD implementation
- Configurable compression levels (1-22)
- Automatic security validation (max 1GB input, 4GB decompressed)
- Conditional compilation with THEMIS_HAS_ZSTD

**Example Usage**:
```cpp
#include "content/pipeline/zstd_compression.h"

using namespace themis::content::pipeline;

ZstdCompression compressor;
compressor.set_compression_level(5);

std::vector<uint8_t> data = {...};
auto compressed = compressor.compress(data);
auto decompressed = compressor.decompress(compressed);
```

### ContentChunker

Generic byte-based chunking for pipeline operations.

**Integration**: Complements `IContentProcessor::chunk()` which provides content-aware chunking

**Use Cases**:
- Pre-chunking before content type detection
- Generic binary data streaming
- Pipeline operations requiring fixed-size chunks
- Testing and development

**For Content-Aware Chunking**:
- Text: Use `TextProcessor::chunk()` (sentence-based with overlap)
- Images: Use `ImageProcessor::chunk()` (tile or region-based)
- Audio/Video: Use respective processor chunking strategies

**Example Usage**:
```cpp
#include "content/pipeline/content_chunker.h"

using namespace themis::content::pipeline;

ContentChunker::ChunkConfig config;
config.chunk_size = 1024 * 1024;  // 1MB chunks
config.overlap = 128;              // 128 bytes overlap

ContentChunker chunker(config);
auto chunks = chunker.chunk(data);
auto reassembled = chunker.reassemble(chunks);
```

### BulkUploadInterface

Simplified base interface for batch content upload (namespace `themis::content::pipeline`).

**Integration**: `AsyncBulkUploader` (see below) implements this interface and submits
work through `AsyncIngestionWorker`; the manager's synchronous intake entry points are
`ContentManager::ingestRawBlob()` and `ContentManager::ingestStream()`.

**Use Cases**:
- Simple batch upload scenarios
- Testing and development
- Pipeline-specific upload patterns
- Integration point for custom upload strategies

**Example Usage**:
```cpp
#include "content/pipeline/bulk_upload_interface.h"

using namespace themis::content::pipeline;

BulkUploadInterface uploader;

// Set progress callback
uploader.set_progress_callback([](const std::string& id, size_t done, size_t total) {
    std::cout << "Upload " << id << ": " << done << "/" << total << std::endl;
});

// Upload single item
BulkUploadInterface::ContentMetadata metadata;
metadata.content_id = "doc-123";
metadata.content_type = "application/pdf";
auto result = uploader.upload(content, metadata);

// Bulk upload
std::vector<std::vector<uint8_t>> contents = {...};
std::vector<BulkUploadInterface::ContentMetadata> metadata_list = {...};
auto results = uploader.bulk_upload(contents, metadata_list);
```

### AsyncBulkUploader

`AsyncBulkUploader` is an asynchronous uploader implementing `BulkUploadInterface` and
submitting work through
`AsyncIngestionWorker::submitFile()` / `submitBatch()`. The wrapper does not call a
`ContentManager::ingest()` method; the manager's synchronous entry points are
`ingestRawBlob()` and `ingestStream()`. An upload result may report `IN_PROGRESS`, so
check the job status before treating it as completed.

**Use Cases**:
- High-throughput production batch ingestion
- Concurrent multi-file upload with back-pressure

### MultiModalChunker

Multi-modal chunk assembler supporting text, image, and metadata content types (`class MultiModalChunker`, namespace `themis::content::pipeline`, header `content/pipeline/multimodal_chunker.h`).

**Use Cases**:
- Mixed-modality content (text + images + structured metadata)
- RAG pipeline document assembly

## Integration with ThemisDB

The Content Pipeline module is designed to integrate with:

1. **Content Manager**: For content type detection, routing, and intake
2. **Storage Layer**: For content persistence
3. **Vector/graph/full-text indexes**: For the index paths exposed by `ContentManager`

### Integration Examples

#### Using Existing ZSTD Compression
```cpp
// Pipeline wrapper uses existing utils::zstd_codec
ZstdCompression compressor;
auto compressed = compressor.compress(data);

// Equivalent to:
auto compressed = themis::utils::zstd_compress(data, 3);
```

#### Using AsyncIngestionWorker for Production
```cpp
// For asynchronous manager ingestion, use `AsyncIngestionWorker` (or its
`AsyncBulkUploader` wrapper)
#include "content/async_ingestion_worker.h"

AsyncIngestionConfig config;
config.worker_thread_count = 4;
AsyncIngestionWorker worker(content_manager, config);
worker.start();

std::vector<std::pair<std::string, std::string>> files = {
    {"report.pdf", pdf_bytes}
};
std::string job_id = worker.submitBatch(files, "user-123");
```

## Testing

The pipeline unit tests are
[`tests/content/test_content_pipeline.cpp`](../../../tests/content/test_content_pipeline.cpp).
`tests/content/CMakeLists.txt` registers `test_*.cpp` files as focused tests. For example,
configure a system-package debug build with the module tests enabled, then build and run
this focused test:

```bash
cmake --preset community-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build --preset community-debug --target module_content_test_content_pipeline_focused
ctest --test-dir build-community-debug -R '^test_content_pipeline_content_FocusedTests$'
```

## Performance Considerations

- **Chunking**: Default chunk size is 1MB, adjustable based on workload
- **Compression**: Uses existing production ZSTD implementation with validated security bounds
- **Bulk Upload**: Simple sequential implementation; use AsyncIngestionWorker for parallel processing

## Future Development

See `docs/de/development/GAP-005-content-pipeline.md` for:
- Integration roadmap with existing components
- Multi-modality support plans
- Compaction strategies
- Performance optimization roadmap

## Contributing

When extending this module:

1. Leverage existing ThemisDB infrastructure where possible
2. Maintain compatibility with existing interfaces
3. Add comprehensive unit tests for new functionality
4. Update documentation with integration patterns
5. Follow ThemisDB coding standards

## Related Components

- **utils::zstd_codec** (`include/utils/zstd_codec.h`) - Production ZSTD implementation
- **IContentProcessor** (`include/content/content_processor.h`) - Content-aware chunking
- **AsyncIngestionWorker** (`include/content/async_ingestion_worker.h`) - Production batch ingestion
- **ContentManager** (`include/content/content_manager.h`) - Content orchestration

## License

Copyright (c) 2024 ThemisDB
SPDX-License-Identifier: MIT

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
