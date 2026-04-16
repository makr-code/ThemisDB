<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Content Module Public Headers

**Module Path:** `include/content/`  
**Implementation Roadmap:** `../../src/content/ROADMAP.md`

---

## Current Status

Public headers at v1.7.0. All 13 format processors (PDF, Office, HTML, Markdown, Image,
Audio, Video, STT, TTS, OCR, CAD, Geo, Archive) have stable headers. Safety, deduplication,
embedding pipeline, plugin extensibility, and version management headers are complete.

---

## Completed Features

- [x] `IContentProcessor` base interface and `IProcessorChain` pipeline
- [x] `IMIMEDetector` and `ContentType` taxonomy
- [x] `IContentManager` for content lifecycle
- [x] Safety headers: `IContentSecurity`, `IContentPolicy`, `IAbuseDetector`
- [x] `IDeduplicationChecker` and `IEmbeddingPipeline`
- [x] 13 format processor headers (PDF, Office, HTML, Markdown, Image, Audio, Video, STT, TTS, OCR, CAD, Geo, Archive)
- [x] `IAsyncIngestionWorker` and `IIngestionPlugin` for async and custom format ingestion
- [x] `IVersionManager` for content version history
- [x] `ILanguageDetector` for language identification

---

## Planned Features

- [x] `IContentClassifier` for automated category tagging (Target: Q3 2026)
- [x] `IPIIRedactor` header for content-level PII redaction before storage (Target: Q3 2026)
- [ ] `IContentDiffProcessor` for structured content diff (Target: Q4 2026)
- [ ] `IFederatedIngestPipeline` for multi-source content federation (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Pipeline Interfaces
- [x] `IContentProcessor`, chain config, type taxonomy, MIME detection

### Phase 2: Safety Headers
- [x] Security, policy, abuse detection, validation headers

### Phase 3: Format Processor Headers
- [x] All 13 format processors (PDF through Archive)

### Phase 4: Embedding & Plugin Headers
- [x] Embedding pipeline, ingestion plugins, content plugin interface

### Phase 5: Advanced Content Headers
- [x] `IContentClassifier` (Q3 2026)
- [x] `IPIIRedactor` (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [x] `mock_clip_processor.h` excluded from production install targets
- [ ] Doxygen fully annotated on all 34 headers

---

## Production Readiness Checklist

- [x] All major content formats covered by processor headers
- [x] Safety pipeline headers always-present
- [x] Plugin extensibility headers present
- [x] Embedding pipeline stable
- [x] `mock_clip_processor.h` excluded from production install
- [x] `IPIIRedactor` header published
