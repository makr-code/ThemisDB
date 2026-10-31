**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `include/content/`
**SOT domain:** `api-contract`
**Canonical source references:** [public headers](.), [manager implementation](../../src/content/content_manager.cpp), [feature configuration](../../cmake/CMakeLists.txt), [test registration](../../tests/content/CMakeLists.txt), [CMake presets](../../CMakePresets.json)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ARCHITECTURE.md · ../../src/content/README.md · ../../src/content/ARCHITECTURE.md · ../../src/content/ROADMAP.md · ../../src/content/FUTURE_ENHANCEMENTS.md · ../../src/content/SECURITY.md · ../../src/content/CHANGELOG.md -->

# Content Module — Public Headers

**Module Path:** `include/content/`
**Implementation:** `../../src/content/`
**Status authority:** See the [content module roadmap](../../src/content/ROADMAP.md) and root [ROADMAP.md](../../ROADMAP.md); this header inventory does not make a release-readiness claim.

This directory contains the public C++ header files (`.h`) for the `content` module.
Headers define the public interfaces and defaults; most implementation is in `src/content/`,
although some configuration, serialization, and helper methods are inline in headers.

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
| [`office_processor.h`](office_processor.h) | `OfficeProcessor` | Office document extraction: DOCX/XLSX/PPTX, ODF and RTF; legacy DOC/XLS/PPT handling can use a configured LibreOffice executable |
| [`image_processor.h`](image_processor.h) | `ImageProcessor` | Image metadata extraction (EXIF, dimensions, colour space), pHash perceptual deduplication |
| [`ocr_processor.h`](ocr_processor.h) | `OCRProcessor` | Tesseract-based OCR (requires `THEMIS_ENABLE_OCR`); 300 DPI pre-processing via Leptonica |
| [`audio_processor.h`](audio_processor.h) | `AudioProcessor` | Audio metadata extraction (ID3, duration, codec) and optional STT transcription |
| [`video_processor.h`](video_processor.h) | `VideoProcessor` (`IContentProcessorPlugin`) | Video metadata and optional FFmpeg-backed extraction when FFmpeg is detected by the build |
| [`stt_processor.h`](stt_processor.h) | `STTProcessor` | Speech-to-text interface (Whisper backend) |
| [`tts_processor.h`](tts_processor.h) | `TTSProcessor` | Text-to-speech generation interface (Piper backend) |
| [`cad_processor.h`](cad_processor.h) | `CADProcessor` | CAD file processing: DXF/DWG/STEP metadata and assembly hierarchy |
| [`geo_processor.h`](geo_processor.h) | `GeoProcessor` | Geospatial content processor: GeoJSON normalization, GPX, coordinate extraction |
| [`archive_processor.h`](archive_processor.h) | `ArchiveProcessor` | ZIP/TAR archive extraction with bomb protection (max 100× ratio, max 1 000 entries) |
| [`mock_clip_processor.h`](mock_clip_processor.h) | `MockCLIPProcessor` | Test/no-GPU stub for CLIP vision-language embedding (not for production use) |

This header inventory is not a `ContentManager` dispatch guarantee. In the current
manager implementation, raw-blob extraction paths include HTML/XHTML, Markdown, opt-in
OCR, and the separately registered archive processor. The other format-specific classes
are public components, but their end-to-end caller/server integration is not established
by their headers; see the [dispatch boundary](../../src/content/API.md#format-dispatch-boundary).

### Deduplication and Embeddings

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`deduplication_checker.h`](deduplication_checker.h) | `DeduplicationChecker` | SHA-256 exact dedup; pHash (images) and MinHash+band-LSH (text) near-duplicate detection |
| [`embedding_pipeline.h`](embedding_pipeline.h) | `EmbeddingPipeline` | LLM-backed text embedding and batch API; `model_name` enables the path but does not select a model or promise normalization |

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
| [`pipeline/async_bulk_uploader.h`](pipeline/async_bulk_uploader.h) | `AsyncBulkUploader` | Async batch-uploader wrapper over `AsyncIngestionWorker` |

### Adapters (`adapters/`)

| Header | Primary Class / Interface | Purpose |
|--------|--------------------------|---------|
| [`adapters/format_extractor_adapters.h`](adapters/format_extractor_adapters.h) | Format extractor adapters | Adapter types for connecting format-specific extractors to the pipeline |

## Build Conditionals

| Build definition | Headers / Components Affected | Dependency / behavior |
|---|---|---|
| `THEMIS_ENABLE_OCR` | `ocr_processor.h` — `OCRProcessor` | Explicit CMake option; requires Tesseract and Leptonica |
| `THEMIS_ENABLE_PDF` | `pdf_processor.h` — `PDFProcessor` | Explicit CMake option; requires poppler-cpp |
| `THEMIS_HAS_FFMPEG` | `video_processor.h` and FFmpeg-backed paths | Defined when the CMake dependency check detects FFmpeg |
| `THEMIS_ENABLE_LLM` | LLM-backed content-manager implementation | CMake option controlling LLM integration |
| `THEMIS_HAS_ZSTD` | `pipeline/zstd_compression.h` | Defined when the zstd dependency is available |
| Runtime `OfficeProcessor::Config::libreoffice_path` | Legacy DOC/XLS/PPT handling | Optional configured LibreOffice executable; no `THEMIS_ENABLE_LIBREOFFICE` option was found in the current CMake files |

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
    // Inspect IngestResult::stage_outcomes and metadata for this input.
    // Optional enrichment/index paths depend on configuration and dependencies.
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

The line-oriented streaming path is for selected text formats (including NDJSON); other
formats are buffered before the raw-blob path, up to the configured `max_buffered_bytes`
(default 256 MiB). See [streaming behavior](../../src/content/IMPLEMENTATION.md#stream)
before assuming every large file is processed incrementally.

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

An empty result can mean that no pipeline is attached or embedding generation failed.
The `model_name` argument on the manager API is not currently used for model selection;
see the [embedding API caveat](../../src/content/API.md#embedding-api-caveat).

### Async Batch Ingestion

```cpp
#include "content/async_ingestion_worker.h"

AsyncIngestionConfig config;
config.worker_thread_count = 8;
config.max_queue_depth = 256;

AsyncIngestionWorker worker(manager, config);
worker.start();

std::vector<std::pair<std::string, std::string>> files = {
    {"report.pdf", pdf_bytes}
};
std::string job_id = worker.submitBatch(files, "user-123");
```

### Deduplication

```cpp
#include "content/deduplication_checker.h"

auto dedup = std::make_shared<DeduplicationChecker>();
manager->setDeduplicationChecker(dedup);

// Near-duplicate checking requires an enabled deduplication policy and this checker.
// When detected, the existing ID is reported in IngestResult::primary_content_id.
```

### Configuring the Processor Chain

```cpp
#include "content/processor_chain_config.h"

ProcessorChainConfig chain;
chain.category_configs[ContentCategory::IMAGE].embedding.enabled = false;
manager->setProcessorChainConfig(chain);
```

`ProcessorChainConfig` resolves the exact MIME entry first, then the content-category
entry, then `default_config`; it does not merge partial entries. Each stage is enabled by
default. `StageConfig` also carries retry count/delay and `continue_on_error`; see
[`processor_chain_config.h`](processor_chain_config.h) for the JSON schema and defaults.

## Building and focused tests

The content tests are registered from `tests/content/test_*.cpp`; their target and CTest
names are derived from each filename stem in
[`tests/content/CMakeLists.txt`](../../tests/content/CMakeLists.txt). For example, with
the system-package `community-debug` preset:

```bash
cmake --preset community-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build --preset community-debug --target module_content_test_content_pipeline_focused
ctest --test-dir build-community-debug -R '^test_content_pipeline_content_FocusedTests$'
```

Optional processor availability depends on the configured build and detected runtime
dependencies; a successful compile of the public header alone does not establish that a
format backend is enabled.

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| Ingestion cannot route a MIME type | MIME type/category is not recognized or no processor is registered for the category | Check `ContentTypeRegistry`, the supplied MIME/filename, and processor registration; do not assume a YAML edit alone adds a processor route |
| Direct `PDFProcessor` extraction is unavailable | Built without `THEMIS_ENABLE_PDF` or poppler-cpp | Rebuild with `-DTHEMIS_ENABLE_PDF=ON`; this does not by itself add a PDF route to `ContentManager::ingestRawBlob()` |
| OCR not activated for images | `ContentPolicy::ocrEnabled()` returns false | Enable OCR in policy config or set `THEMIS_ENABLE_OCR=ON` at build time |
| Async stream submission waits under load | Queue back-pressure threshold reached | Check worker state and queue statistics; tune `AsyncIngestionConfig` or producer concurrency |
| Archive extraction rejected | Archive safety limits or processor configuration rejected the input | Check the active archive/security configuration and source error; avoid raising limits without security review |
| LLM analysis methods return empty results | Built without `THEMIS_ENABLE_LLM` | Rebuild with `-DTHEMIS_ENABLE_LLM=ON` and configure the LLM backend |
| `generateEmbedding()` returns empty vector | No `EmbeddingPipeline` attached | Call `manager->setEmbeddingPipeline(pipeline)` before ingestion |
| Legacy Office file (`.doc`/`.xls`/`.ppt`) extraction fails | Optional LibreOffice executable is unavailable or not configured | Check `OfficeProcessor::Config::libreoffice_path` and the installed runtime |
