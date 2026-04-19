<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Content Module — Public Header Architecture

**Version:** 1.7.0
**Last Updated:** 2026-04-06
**Module Path:** `include/content/`
**Implementation:** `../../src/content/`

---

## 1. Overview

The `include/content/` directory exposes public C++ headers for ThemisDB's content processing
pipeline. The module handles ingestion, type detection, format-specific extraction (PDF, Office,
HTML, Markdown, audio, video, image, CAD, OCR, STT/TTS, geo), deduplication, abuse detection,
content policy enforcement, embedding generation, and plugin-based extensibility.

---

## 2. Design Principles

- **Chain Architecture** – `processor_chain_config.h` defines the processing pipeline as an
  ordered chain; each processor implements `IContentProcessor` and can be hot-configured.
- **Type-Driven Dispatch** – `mime_detector.h` and `content_type.h` determine the processor
  chain at ingestion; unknown types are routed to the `ingestion_plugin.h` extension point.
- **Safety Layers** – `content_security.h`, `content_policy.h`, and `abuse_detector.h` are
  always-present interfaces; they run before any content is persisted.
- **Embedding Pipeline** – `embedding_pipeline.h` defines the vector generation step that
  runs after content extraction; outputs feed the `cache/` and `search/` modules.
- **Plugin Extensibility** – `content_plugin_interface.h` and `ingestion_plugin.h` allow
  third-party format processors without modifying core chain code.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `content_processor.h` | `IContentProcessor`, `ProcessorResult` | Base processor interface for all format handlers |
| `processor_chain_config.h` | `ProcessorChainConfig`, `IProcessorChain` | Pipeline chain configuration and execution |
| `content_type.h` | `ContentType` enum, `ContentTypeInfo` | Content type taxonomy |
| `mime_detector.h` | `IMIMEDetector` | MIME type detection from content bytes |
| `content_manager.h` | `IContentManager`, `ContentItem` | Content lifecycle management |
| `content_validator.h` | `IContentValidator`, `ValidationResult` | Format-level content validation |
| `content_policy.h` | `IContentPolicy`, `PolicyResult` | Content policy enforcement |
| `content_security.h` | `IContentSecurity`, `ThreatResult` | Malware / threat detection |
| `abuse_detector.h` | `IAbuseDetector`, `AbuseSignal` | Abuse pattern detection |
| `deduplication_checker.h` | `IDeduplicationChecker`, `DedupResult` | Exact and fuzzy deduplication |
| `embedding_pipeline.h` | `IEmbeddingPipeline`, `EmbeddingRequest` | Vector embedding generation |
| `pdf_processor.h` | `IPDFProcessor` | PDF text and metadata extraction |
| `office_processor.h` | `IOfficeProcessor` | Office document (DOCX, XLSX, PPTX) extraction |
| `html_processor.h` | `IHTMLProcessor` | HTML content extraction |
| `markdown_processor.h` | `IMarkdownProcessor` | Markdown parsing and extraction |
| `image_processor.h` | `IImageProcessor` | Image metadata and content extraction |
| `audio_processor.h` | `IAudioProcessor` | Audio metadata extraction |
| `video_processor.h` | `IVideoProcessor` | Video frame and metadata extraction |
| `stt_processor.h` | `ISTTProcessor` | Speech-to-text transcription |
| `tts_processor.h` | `ITTSProcessor` | Text-to-speech synthesis |
| `ocr_processor.h` | `IOCRProcessor` | Optical character recognition |
| `cad_processor.h` | `ICADProcessor` | CAD file metadata extraction |
| `geo_processor.h` | `IGeoProcessor` | Geospatial content extraction |
| `archive_processor.h` | `IArchiveProcessor` | Archive (ZIP, TAR) decompression and enumeration |
| `async_ingestion_worker.h` | `IAsyncIngestionWorker`, `IngestionJob` | Async ingestion task dispatch |
| `ingestion_plugin.h` | `IIngestionPlugin`, `PluginManifest` | Custom format ingestion plugin |
| `content_plugin_interface.h` | `IContentPlugin` | Plugin base interface |
| `language_detector.h` | `ILanguageDetector`, `LanguageResult` | Language detection |
| `content_fs.h` | `IContentFS` | Content filesystem abstraction |
| `content_logger.h` | `IContentLogger` | Content operation audit logger |
| `content_metrics.h` | `ContentMetrics` | Content pipeline metric descriptors |
| `content_errors.h` | `ContentErrorCode` enum | Canonical content error taxonomy |
| `version_manager.h` | `IVersionManager`, `ContentVersion` | Content version history |
| `mock_clip_processor.h` | `MockCLIPProcessor` | Test mock for CLIP multimodal processor |

> **Implementation details:** `../../src/content/`
