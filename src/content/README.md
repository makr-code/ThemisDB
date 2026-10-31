**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [manager API](../../include/content/content_manager.h), [manager implementation](content_manager.cpp), [server manager setup](../server/http_server.cpp), [module architecture](ARCHITECTURE.md), [processor API](../../include/content/content_processor.h)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# ThemisDB Content Module

<!-- Status: current | validated: 2026-08-15 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PERFORMANCE_EXPECTATIONS.md · CMT-7504-DOCUMENTATION_SYNC.md -->
<!-- Wave Context: Wave B (Performance Consolidation Q3-Q4 2026) — Content Integrity + Versioning + Large-Content Performance -->

Related docs: [Architecture](ARCHITECTURE.md) · [Roadmap](ROADMAP.md) · [Future Enhancements](FUTURE_ENHANCEMENTS.md) · [Batch 5 Gaps](MODULE_GAPS_BATCH5.md) · [Performance Expectations](PERFORMANCE_EXPECTATIONS.md) · [CMT-7504 Sync](CMT-7504-DOCUMENTATION_SYNC.md)

## Module Purpose

Production-capable multi-format content ingestion, storage/retrieval, versioning, and metadata management for ThemisDB supporting concurrent access patterns. **Batch 5 enhancement focus: Content integrity verification, versioning correctness, large-content performance optimization, concurrent access safety**.

## Module Status (Batch 5, 2026-08-15)

**Maturity:** 🟢 **PRODUCTION-CANDIDATE** with finalization in progress
**Scope Verification:** CMT-7503 verified no dangling pointers (image/pdf adapters RAII-safe)
**Test Coverage:** CMT-FIN-36..40 (adapter scope validation), CMT-FIN-41..46 (doc linkset validation)
**Documentation Sync:** CMT-7504 in progress (ROADMAP/README/FUTURE_ENHANCEMENTS synchronized)

## Relevant Interfaces

The content inventory includes core processor families such as ContentManager, PDFProcessor, ImageProcessor, OfficeProcessor, HtmlProcessor, MarkdownProcessor, TextProcessor, ArchiveProcessor, OCRProcessor, AudioProcessor, STTProcessor, TTSProcessor, VideoProcessor, ContentValidator, ContentPolicy, ContentSecurity, MimeDetector, DeduplicationChecker, and EmbeddingPipeline.

| Interface / File | Role |
|---|---|
| content_manager.cpp | content ingestion orchestration and routing |
| content_manager_llm.cpp | LLM-augmented content analysis integration |
| content_manager_embedding.cpp | embedding pipeline integration for content paths |
| content_validator.cpp | validation and pre-ingestion safety checks |
| content_policy.cpp | processing policy and stage control behavior |
| content_security.cpp | archive/content security checks and safeguards |
| mime_detector.cpp | mime detection and routing classification |
| text_processor.cpp | text extraction and normalization paths |
| pdf_processor.cpp | PDF extraction paths |
| office_processor.cpp | Office/legacy office extraction paths |
| html_processor.cpp | HTML extraction and cleanup paths |
| markdown_processor.cpp | markdown/frontmatter extraction paths |
| image_processor.cpp | image metadata and image processing paths |
| ocr_processor.cpp | OCR extraction paths |
| audio_processor.cpp | audio metadata extraction paths |
| stt_processor.cpp | speech-to-text integration paths |
| tts_processor.cpp | text-to-speech integration paths |
| video_processor.cpp | video metadata processing paths |
| archive_processor.cpp | archive ingestion and extraction safeguards |
| deduplication_checker.cpp | hash/perceptual deduplication checks |
| embedding_pipeline.cpp | content embedding generation/runtime paths |
| async_ingestion_worker.cpp | asynchronous ingestion worker runtime |
| ingestion_plugin.cpp | plugin integration for content processing |

## Scope

In scope:
- multi-format content ingestion and extraction runtime paths
- policy, validation, and security controls for content processing
- content enrichment, deduplication, and embedding integration
- asynchronous processing and plugin-based extension surfaces

Out of scope:
- search/ranking ownership outside content ingestion boundaries
- non-content business logic in external domain modules
- low-level storage engine internals outside content integration points

## Runtime Behavior and Limits

- behavior depends on enabled processors, policies, and feature flags.
- large/complex payload behavior depends on configured limits and pipeline settings.
- degraded processor dependencies produce structured runtime failure behavior.

## Current ContentManager integration boundary (source-checked 2026-10-31)

The public processor inventory is not a map of direct `ContentManager` dispatch. In the
current `content_manager.cpp`, `ingestRawBlob()` directly uses `HtmlProcessor` for
HTML/XHTML, `MarkdownProcessor` for Markdown, and the OCR helper when requested by
configuration. Its archive branch consults a registered `ArchiveProcessor`; without one,
it stores the archive with metadata rather than extracting it. The stream path handles
selected line-oriented text formats separately and sends other formats through bounded
buffering to the raw-blob path.

The manager implementation has no direct calls to the PDF, Office, image, audio, video,
CAD, or geospatial processor classes. `src/server/http_server.cpp` registers a
`TextProcessor`, but the raw-blob path's category registry does not establish that this
processor handles every text MIME type. Public headers and standalone implementations do
not by themselves establish complete manager/server integration.
**TODO:** trace the caller/server/plugin paths for those processors before making
end-to-end extraction claims. Until then, treat that integration as unverified.

## Sourcecode Verification (Module: content/readme)

This module contains 47 processor, management, and support files:

**Core Manager (3):**
- src/content/content_manager.cpp
- src/content/content_manager_llm.cpp
- src/content/content_manager_embedding.cpp

**Validation & Security (4):**
- src/content/content_validator.cpp
- src/content/content_policy.cpp
- src/content/content_security.cpp
- src/content/content_logger.cpp

**MIME Detection & Content Classification (2):**
- src/content/mime_detector.cpp
- src/content/content_type.cpp

**Text & Format Processors (6):**
- src/content/text_processor.cpp
- src/content/markdown_processor.cpp
- src/content/html_processor.cpp
- src/content/pdf_processor.cpp
- src/content/office_processor.cpp
- src/content/language_detector.cpp

**Media Processors (5):**
- src/content/image_processor.cpp
- src/content/audio_processor.cpp
- src/content/video_processor.cpp
- src/content/ocr_processor.cpp
- src/content/cad_processor.cpp

**Speech Processing (2):**
- src/content/stt_processor.cpp
- src/content/tts_processor.cpp

**Extraction & Format Adapters (7):**
- src/content/adapters/image_extractor_adapter.cpp
- src/content/adapters/pdf_extractor_adapter.cpp
- src/content/adapters/audio_extractor_adapter.cpp
- src/content/adapters/office_extractor_adapter.cpp
- src/content/adapters/text_extractor_adapter.cpp
- src/content/adapters/archive_extractor_adapter.cpp
- src/content/adapters/format_extractor_factory.cpp

**Enrichment & Embedding (2):**
- src/content/embedding_pipeline.cpp
- src/content/content_manager_embedding.cpp

**Ingestion & Async Processing (4):**
- src/content/async_ingestion_worker.cpp
- src/content/ingestion_plugin.cpp
- src/content/pipeline/async_bulk_uploader.cpp
- src/content/pipeline/bulk_upload_interface.cpp

**Deduplication & Utilities (5):**
- src/content/deduplication_checker.cpp
- src/content/archive_processor.cpp
- src/content/archive_processor_enhancements.cpp
- src/content/version_manager.cpp
- src/content/content_metrics.cpp

**Chunking & Compression (3):**
- src/content/pipeline/content_chunker.cpp
- src/content/pipeline/multimodal_chunker.cpp
- src/content/pipeline/zstd_compression.cpp

**Support & Error Handling (2):**
- src/content/content_errors.cpp
- src/content/content_fs.cpp

**Mock / Test Helpers (1):**
- src/content/mock_clip_processor.cpp

- Verified behavior surfaces:
  - ingestion orchestration and processor routing
  - validation/security and enrichment/deduplication behavior
  - async pipeline and plugin integration paths
  - multi-format extraction and chunking
- Batch 5 (v2.4.0 GA closure) tracking:
  - **CMT-7503:** Scope mismatch verification (adapters RAII-safe) ✅ VERIFIED
  - **CMT-FIN-36..40:** Adapter scope validation tests ✅ CREATED (`tests/content/test_adapter_scope_validation.cpp`)
  - **CMT-7504:** Documentation linkset synchronization (in progress)
  - **CMT-FIN-41..46:** Documentation linkset validation tests ✅ CREATED (`tests/content/test_content_docs_linkset_validation.cpp`)
  - **CMT-7505:** Test coverage correlation (planned)
  - **CMT-7506:** GA promotion sign-off (planned)
  - forward planning is tracked in [ROADMAP.md](ROADMAP.md) and [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
  - historical entries remain in [CHANGELOG.md](CHANGELOG.md)
