> **Build:** `cmake --preset release && cmake --build build/release`

# Content Module — Public Headers

**Module Path:** `include/content/`
**Implementation:** `../../src/content/`

## Purpose

Public interfaces for ThemisDB's multi-modal content ingestion, processing, and management pipeline.

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `content_manager.h` | `ContentManager` — top-level content lifecycle coordinator |
| `content_processor.h` | `IContentProcessor` — base processor interface |
| `content_plugin_interface.h` | `IContentPlugin` — plugin extension interface |
| `ingestion_plugin.h` | `IIngestionPlugin` — ingestion-stage plugin contract |
| `processor_chain_config.h` | `ProcessorChainConfig` — ordered processor pipeline config |
| `content_type.h` | `ContentType`, `MimeType` — content type enumeration |
| `content_validator.h` | `ContentValidator` — pre-ingestion validation |
| `content_policy.h` | `ContentPolicy` — retention, access, and processing policies |
| `content_security.h` | `ContentSecurity` — content-level security controls |
| `content_classifier.h` | `ContentClassifier` — ML-based content classification |
| `content_errors.h` | `ContentError`, `ContentException` — structured error types |
| `content_metrics.h` | `ContentMetrics` — ingestion throughput and error counters |
| `content_logger.h` | `ContentLogger` — structured content audit logger |
| `content_fs.h` | `ContentFS` — virtual file-system abstraction for content storage |
| `version_manager.h` | `VersionManager` — content version history and rollback |
| `deduplication_checker.h` | `DeduplicationChecker` — hash-based duplicate detection |
| `abuse_detector.h` | `AbuseDetector` — harmful-content detection interface |
| `pii_redactor.h` | `PIIRedactor` — personally identifiable information redaction |
| `language_detector.h` | `LanguageDetector` — text language identification |
| `mime_detector.h` | `MimeDetector` — MIME type detection from content |
| `embedding_pipeline.h` | `EmbeddingPipeline` — text/image embedding generation |
| `async_ingestion_worker.h` | `AsyncIngestionWorker` — async multi-threaded ingestion worker |
| `pdf_processor.h` | `PDFProcessor` — PDF text and metadata extraction |
| `html_processor.h` | `HTMLProcessor` — HTML content extraction and sanitization |
| `markdown_processor.h` | `MarkdownProcessor` — Markdown parsing and rendering |
| `office_processor.h` | `OfficeProcessor` — Office document (DOCX/XLSX/PPTX) processor |
| `image_processor.h` | `ImageProcessor` — image resize, OCR pipeline |
| `ocr_processor.h` | `OCRProcessor` — optical character recognition |
| `audio_processor.h` | `AudioProcessor` — audio transcription pipeline |
| `video_processor.h` | `VideoProcessor` — video frame extraction and processing |
| `stt_processor.h` | `STTProcessor` — speech-to-text interface |
| `tts_processor.h` | `TTSProcessor` — text-to-speech interface |
| `cad_processor.h` | `CADProcessor` — CAD file processing (DXF/DWG/STEP) |
| `geo_processor.h` | `GeoProcessor` — geospatial content processor |
| `archive_processor.h` | `ArchiveProcessor` — ZIP/TAR archive extraction |
| `mock_clip_processor.h` | `MockCLIPProcessor` <!-- TODO: verify --> — test stub for CLIP vision-language model |

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-content
```

## See Also

- [`../../src/content/README.md`](../../src/content/README.md) — implementation details

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
