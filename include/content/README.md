> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ARCHITECTURE.md · ../../src/content/README.md · ../../src/content/ARCHITECTURE.md · ../../src/content/ROADMAP.md · ../../src/content/FUTURE_ENHANCEMENTS.md · ../../src/content/SECURITY.md · ../../src/content/CHANGELOG.md -->

# Content Module — Public Headers

**Module Path:** `include/content/`
**Implementation:** `../../src/content/`
**Status:** ✅ Production Ready

This directory contains the public C++ header files (`.h`) for the `content` module.
All headers are `#pragma once` guarded and contain no implementation code.

## Header Overview

### Core Pipeline

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`content_manager.h`](content_manager.h) | `ContentManager` | Top-level content lifecycle coordinator: ingest, retrieve, search, VFS |
| [`content_processor.h`](content_processor.h) | `IContentProcessor`, `TextProcessor`, `LegacyImageProcessor`, `LegacyGeoProcessor`, `LegacyAudioProcessor`, `LegacyCADProcessor`, `StructuredProcessor`, `BinaryProcessor` | Base processor interface and legacy processor implementations |
| [`content_plugin_interface.h`](content_plugin_interface.h) | `IContentPlugin` | Plugin extension interface for embedding custom processors |
| [`ingestion_plugin.h`](ingestion_plugin.h) | `IIngestionPlugin` | Ingestion-stage plugin contract; used for format-specific processors |
| [`processor_chain_config.h`](processor_chain_config.h) | `ProcessorChainConfig` | Ordered processor pipeline configuration (per MIME type or category) |
| [`async_ingestion_worker.h`](async_ingestion_worker.h) | `AsyncIngestionWorker`, `AsyncIngestionConfig`, `IngestionJob` | Async multi-threaded ingestion worker with back-pressure and batch support |

### Content Types and Classification

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`content_type.h`](content_type.h) | `ContentType`, `ContentCategory` | Content type enum (`TEXT`, `IMAGE`, `AUDIO`, `VIDEO`, `GEO`, `CAD`, `ARCHIVE`, `STRUCTURED`, `BINARY`) and type descriptor |
| [`mime_detector.h`](mime_detector.h) | `MimeDetector` | MIME type detection via magic bytes, file extension, and libmagic; YAML-driven config |
| [`language_detector.h`](language_detector.h) | `LanguageDetector` | Multi-script text language identification and routing |
| [`content_classifier.h`](content_classifier.h) | `ContentClassifier` | ML-based content classification for tagging and routing |

### Validation, Policy, and Security

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`content_validator.h`](content_validator.h) | `ContentValidator` | Pre-ingestion format validation (JSON schema, image decode, dimensions) |
| [`content_policy.h`](content_policy.h) | `ContentPolicy` | Configurable retention, access, MIME allowlist, OCR activation, and processing policies |
| [`content_security.h`](content_security.h) | `ContentSecurity`, `ContentSecurityManager` | Archive bomb detection, decompression-ratio limits, malware signature scanning |
| [`abuse_detector.h`](abuse_detector.h) | `AbuseDetector`, `PhotoDNAAbuseDetector`, `TextAbuseDetector` | Harmful-content detection (CSAM hashes, spam fingerprints, blocklist patterns) |
| [`pii_redactor.h`](pii_redactor.h) | `PIIRedactor` | Personally identifiable information redaction before storage |

### Format-Specific Processors

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`pdf_processor.h`](pdf_processor.h) | `PDFProcessor` | PDF text and metadata extraction via poppler-cpp (layout-preserving, per-page) |
| [`html_processor.h`](html_processor.h) | `HTMLProcessor` | HTML content extraction with boilerplate removal |
| [`markdown_processor.h`](markdown_processor.h) | `MarkdownProcessor` | Markdown parsing and frontmatter extraction |
| [`office_processor.h`](office_processor.h) | `OfficeProcessor` | Office document extraction: DOCX/XLSX/PPTX (libzip+pugixml), ODF, legacy DOC/XLS/PPT (LibreOffice headless) |
| [`image_processor.h`](image_processor.h) | `ImageProcessor` | Image metadata extraction (EXIF, dimensions, colour space), pHash perceptual deduplication |
| [`ocr_processor.h`](ocr_processor.h) | `OCRProcessor` | Tesseract-based OCR (requires `THEMIS_ENABLE_OCR`); 300 DPI pre-processing via Leptonica |
| [`audio_processor.h`](audio_processor.h) | `AudioProcessor` | Audio metadata extraction (ID3, duration, codec) and optional STT transcription |
| [`video_processor.h`](video_processor.h) | `VideoProcessor` | Video metadata extraction and frame extraction (FFmpeg, requires `THEMIS_ENABLE_FFMPEG`) |
| [`stt_processor.h`](stt_processor.h) | `STTProcessor` | Speech-to-text interface (Whisper backend) |
| [`tts_processor.h`](tts_processor.h) | `TTSProcessor` | Text-to-speech generation interface (Piper backend) |
| [`cad_processor.h`](cad_processor.h) | `CADProcessor` | CAD file processing: DXF/DWG/STEP metadata and assembly hierarchy |
| [`geo_processor.h`](geo_processor.h) | `GeoProcessor` | Geospatial content processor: GeoJSON normalization, GPX, coordinate extraction |
| [`archive_processor.h`](archive_processor.h) | `ArchiveProcessor` | ZIP/TAR archive extraction with bomb protection (max 100× ratio, max 1 000 entries) |
| [`mock_clip_processor.h`](mock_clip_processor.h) | `MockCLIPProcessor` | Test/no-GPU stub for CLIP vision-language embedding (not for production use) |

### Deduplication and Embeddings

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`deduplication_checker.h`](deduplication_checker.h) | `DeduplicationChecker` | SHA-256 exact dedup; pHash (images) and MinHash+band-LSH (text) near-duplicate detection |
| [`embedding_pipeline.h`](embedding_pipeline.h) | `EmbeddingPipeline` | Text/image embedding generation: batch API, model dispatch, normalised vectors |

### Observability and Utilities

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`content_metrics.h`](content_metrics.h) | `ContentMetrics` | Prometheus-style ingestion throughput, error counters, format breakdown |
| [`content_logger.h`](content_logger.h) | `ContentLogger` | Structured content audit logger |
| [`content_fs.h`](content_fs.h) | `ContentFS` | Virtual file-system abstraction for content storage path resolution |
| [`version_manager.h`](version_manager.h) | `VersionManager` | Content version history tracking and rollback |
| [`content_errors.h`](content_errors.h) | `ContentError`, `ContentException` | Typed content processing error hierarchy |

### Pipeline Utilities (`pipeline/`)

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`pipeline/content_chunker.h`](pipeline/content_chunker.h) | `ContentChunker` | Fixed-size and overlap byte-based chunking for pipeline stages |
| [`pipeline/multimodal_chunker.h`](pipeline/multimodal_chunker.h) | `MultiModalChunker` | Multi-modal chunk assembly (text + image + metadata) for RAG pipelines |
| [`pipeline/zstd_compression.h`](pipeline/zstd_compression.h) | `ZstdCompression` | zstd codec wrapper (levels 1–22, max 1 GB input, 4 GB decompressed) |
| [`pipeline/bulk_upload_interface.h`](pipeline/bulk_upload_interface.h) | `BulkUploadInterface` | Base interface for batch content upload |
| [`pipeline/async_bulk_uploader.h`](pipeline/async_bulk_uploader.h) | `AsyncBulkUploader` | Production async batch uploader wrapping `ContentManager::ingest()` |

### Adapters (`adapters/`)

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`adapters/format_extractor_adapters.h`](adapters/format_extractor_adapters.h) | Format extractor adapters | Adapter types for connecting format-specific extractors to the pipeline |

## Build Conditionals

| CMake Symbol | Headers / Components Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_OCR` | `ocr_processor.h` — `OCRProcessor` | Tesseract, Leptonica |
| `THEMIS_ENABLE_PDF` | `pdf_processor.h` — `PDFProcessor` | poppler-cpp |
| `THEMIS_ENABLE_FFMPEG` | `video_processor.h` — `VideoProcessor` frame extraction | FFmpeg |
| `THEMIS_ENABLE_LLM` | `content_manager.h` — `analyzeContent()`, `summarizeContent()`, `classifyContent()`, `generateTags()`, `extractEntities()` | LLM integration (`src/llm/`) |
| `THEMIS_ENABLE_LIBREOFFICE` | `office_processor.h` — legacy DOC/XLS/PPT via LibreOffice headless | LibreOffice runtime |
| `THEMIS_HAS_ZSTD` | `pipeline/zstd_compression.h` — `ZstdCompression` | libzstd |

## Documentation

- Module overview: [`../../src/content/README.md`](../../src/content/README.md)
- Architecture guide: [`../../src/content/ARCHITECTURE.md`](../../src/content/ARCHITECTURE.md)
- Header type hierarchy: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`../../src/content/ROADMAP.md`](../../src/content/ROADMAP.md)
- Future enhancements: [`../../src/content/FUTURE_ENHANCEMENTS.md`](../../src/content/FUTURE_ENHANCEMENTS.md)
- Security notes: [`../../src/content/SECURITY.md`](../../src/content/SECURITY.md)
- Changelog: [`../../src/content/CHANGELOG.md`](../../src/content/CHANGELOG.md)
- Pipeline README: [`../../src/content/pipeline/README.md`](../../src/content/pipeline/README.md)
- German developer docs: [`../../docs/de/content/`](../../docs/de/content/)

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

### Ingesting a File (Raw Blob)

```cpp
#include "content/content_manager.h"

using namespace themis::content;

// Construct ContentManager with storage and index dependencies
auto manager = std::make_shared<ContentManager>(
    storage, vector_index, graph_index, secondary_index
);

// Ingest a PDF file
std::string pdf_bytes = read_file("report.pdf");
auto result = manager->ingestRawBlob(
    pdf_bytes,
    "report.pdf",
    "application/pdf",
    "user-123"
);

if (result.success) {
    std::string content_id = result.primary_content_id;
    // content_id is now stored, chunked, and indexed
}
```

### Streaming Ingestion for Large Files

```cpp
#include "content/content_manager.h"
#include <fstream>

std::ifstream stream("large_dataset.ndjson", std::ios::binary);
nlohmann::json config = {{"chunk_size_bytes", 4 * 1024 * 1024}};  // 4 MB chunks

auto result = manager->ingestStream(stream, "large_dataset.ndjson", "", "user-123", config);
```

### Semantic and Hybrid Search

```cpp
// Vector similarity search
auto hits = manager->searchContent("machine learning introduction", 10);

// Hybrid vector + fulltext (RRF)
auto hybrid_hits = manager->searchContentHybrid(
    "machine learning introduction", 10,
    nlohmann::json::object(),
    0.5f,   // vector weight
    0.5f,   // fulltext weight
    60.0f   // RRF k constant
);
```

### Embedding Generation

```cpp
#include "content/embedding_pipeline.h"
#include "content/content_manager.h"

// Attach an embedding pipeline
auto pipeline = std::make_shared<EmbeddingPipeline>(model_config);
manager->setEmbeddingPipeline(pipeline);

// Generate an embedding for arbitrary text
std::vector<float> vec = manager->generateEmbedding("ThemisDB hybrid database");
```

### Async Batch Ingestion

```cpp
#include "content/async_ingestion_worker.h"

AsyncIngestionConfig config;
config.worker_thread_count = 8;
config.max_queue_depth = 256;

AsyncIngestionWorker worker(manager, config);
worker.start();

IngestionJob job;
job.job_id    = "batch-001";
job.type      = IngestionJobType::BATCH_FILES;
// ... populate job.files
worker.submitJob(job);
```

### Deduplication

```cpp
#include "content/deduplication_checker.h"

auto dedup = std::make_shared<DeduplicationChecker>();
manager->setDeduplicationChecker(dedup);

// ingestRawBlob() automatically rejects near-duplicates and returns
// the existing content_id in IngestResult::primary_content_id
```

### Configuring the Processor Chain

```cpp
#include "content/processor_chain_config.h"

ProcessorChainConfig chain;
chain.disableStageForCategory(ContentCategory::IMAGE, "embedding");
manager->setProcessorChainConfig(chain);
```

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `ingestRawBlob()` returns `success = false` with "Unsupported MIME type" | MIME type not in allowlist or content type undetectable | Add MIME type to `config/data_management/mime_types.yaml`; ensure magic bytes are valid |
| PDF ingestion fails with "poppler not available" | Built without `THEMIS_ENABLE_PDF` | Rebuild with `-DTHEMIS_ENABLE_PDF=ON` |
| OCR not activated for images | `ContentPolicy::ocrEnabled()` returns false | Enable OCR in policy config or set `THEMIS_ENABLE_OCR=ON` at build time |
| `ingestStream()` blocks indefinitely | Worker queue depth at `max_queue_depth` | Increase `max_queue_depth` in `AsyncIngestionConfig` or reduce ingestion rate |
| Archive extraction rejected as zip bomb | Decompression ratio > 100× or > 1 000 entries | Adjust `content.archive.max_size_mb` / `content.archive.max_depth` in config, or split the archive |
| LLM analysis methods return empty results | Built without `THEMIS_ENABLE_LLM` | Rebuild with `-DTHEMIS_ENABLE_LLM=ON` and configure the LLM backend |
| `generateEmbedding()` returns empty vector | No `EmbeddingPipeline` attached | Call `manager->setEmbeddingPipeline(pipeline)` before ingestion |
| Legacy Office file (`.doc`/`.xls`/`.ppt`) extraction fails | LibreOffice not installed or `THEMIS_ENABLE_LIBREOFFICE` not set | Install LibreOffice runtime and rebuild with `-DTHEMIS_ENABLE_LIBREOFFICE=ON` |
