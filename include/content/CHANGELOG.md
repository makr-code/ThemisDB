<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Content Module Public Headers

All notable changes to public headers in `include/content/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.7.0] — 2026-03-09
### Added
- `geo_processor.h`: `IGeoProcessor` for geospatial content extraction
- `cad_processor.h`: `ICADProcessor` for CAD file metadata extraction
- `stt_processor.h`: `ISTTProcessor` for speech-to-text transcription
- `tts_processor.h`: `ITTSProcessor` for text-to-speech synthesis
- `language_detector.h`: `ILanguageDetector` and `LanguageResult` for language detection
- `mock_clip_processor.h`: `MockCLIPProcessor` test mock for CLIP multimodal processing

### Changed
- `processor_chain_config.h`: `ProcessorChainConfig` now supports dynamic chain reconfiguration
- `embedding_pipeline.h`: `EmbeddingRequest` extended with `modality` field (text/image/audio)
- `content_type.h`: Added `CAD`, `GEO_VECTOR`, `GEO_RASTER` to `ContentType` enum

## [1.6.0] — 2026-02-01
### Added
- `async_ingestion_worker.h`: `IAsyncIngestionWorker` and `IngestionJob` for async ingestion
- `ingestion_plugin.h`: `IIngestionPlugin` and `PluginManifest` for custom format plugins
- `content_plugin_interface.h`: `IContentPlugin` base for all plugins
- `version_manager.h`: `IVersionManager` and `ContentVersion` for content versioning
- `archive_processor.h`: `IArchiveProcessor` for ZIP/TAR handling
- `ocr_processor.h`: `IOCRProcessor` for optical character recognition

## [1.5.0] — 2026-01-10
### Added
- Initial public header set: `content_processor.h`, `processor_chain_config.h`,
  `content_type.h`, `mime_detector.h`, `content_manager.h`
- `content_validator.h`, `content_policy.h`, `content_security.h`, `abuse_detector.h`
- `deduplication_checker.h`, `embedding_pipeline.h`
- `pdf_processor.h`, `office_processor.h`, `html_processor.h`, `markdown_processor.h`
- `image_processor.h`, `audio_processor.h`, `video_processor.h`
- `content_fs.h`, `content_logger.h`, `content_metrics.h`, `content_errors.h`
