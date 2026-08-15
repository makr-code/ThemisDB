# ThemisDB Content Module

<!-- Status: current | validated: 2026-08-15 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · CMT-7504-DOCUMENTATION_SYNC.md -->

## Module Purpose

The content module provides multi-format ingestion and processing runtime surfaces for ThemisDB, including extraction, validation, enrichment, deduplication, and content-pipeline orchestration.

## Module Status (Batch 5, 2026-08-15)

**Maturity:** 🟢 **PRODUCTION-CANDIDATE** with finalization in progress  
**Scope Verification:** CMT-7503 verified no dangling pointers (image/pdf adapters RAII-safe)  
**Test Coverage:** CMT-FIN-36..40 (adapter scope validation), CMT-FIN-41..46 (doc linkset validation)  
**Documentation Sync:** CMT-7504 in progress (ROADMAP/README/FUTURE_ENHANCEMENTS synchronized)

## Relevant Interfaces

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

## Sourcecode Verification (Module: content/readme)

- Verified files:
  - src/content/content_manager.cpp
  - src/content/content_manager_llm.cpp
  - src/content/content_manager_embedding.cpp
  - src/content/content_validator.cpp
  - src/content/content_policy.cpp
  - src/content/content_security.cpp
  - src/content/mime_detector.cpp
  - src/content/text_processor.cpp
  - src/content/pdf_processor.cpp
  - src/content/office_processor.cpp
  - src/content/html_processor.cpp
  - src/content/markdown_processor.cpp
  - src/content/image_processor.cpp
  - src/content/ocr_processor.cpp
  - src/content/audio_processor.cpp
  - src/content/stt_processor.cpp
  - src/content/tts_processor.cpp
  - src/content/video_processor.cpp
  - src/content/archive_processor.cpp
  - src/content/deduplication_checker.cpp
  - src/content/embedding_pipeline.cpp
  - src/content/async_ingestion_worker.cpp
  - src/content/ingestion_plugin.cpp
- Verified behavior surfaces:
  - ingestion orchestration and processor routing
  - validation/security and enrichment/deduplication behavior
  - async pipeline and plugin integration paths
- Batch 5 (v2.4.0 GA closure) tracking:
  - **CMT-7503:** Scope mismatch verification (adapters RAII-safe) ✅ VERIFIED
  - **CMT-FIN-36..40:** Adapter scope validation tests ✅ CREATED (test_adapter_scope_validation.cpp)
  - **CMT-7504:** Documentation linkset synchronization (in progress)
  - **CMT-FIN-41..46:** Documentation linkset validation tests ✅ CREATED (test_content_docs_linkset_validation.cpp)
  - **CMT-7505:** Test coverage correlation (planned)
  - **CMT-7506:** GA promotion sign-off (planned)
  - Forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - Historical entries remain in CHANGELOG.md