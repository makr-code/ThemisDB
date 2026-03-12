### Context

This issue implements the roadmap item '`ProvenanceTracker`: Replace AQL Template Stubs with Live Connection' for the training domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `ProvenanceTracker`: Replace AQL Template Stubs with Live Connection

### Goal

Deliver the scoped changes for `ProvenanceTracker`: Replace AQL Template Stubs with Live Connection in src/training/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `ProvenanceTracker`: Replace AQL Template Stubs with Live Connection
**Priority:** High
**Target Version:** v1.8.0

`provenance_tracker.cpp` line 36 documents: "AQL template stubs (production: bind against live ArangoDB connection)". The provenance tracker uses in-process simulation for AQL-based lineage queries (line 141: "build a stub tree from in-process store"). In production, provenance information is not queryable via the AQL API.

**Implementation Notes:**
- `[ ]` Replace the in-process stub with `AQLRunner::execute(provenance_query, bindings)` calls; inject `AQLRunner*` into `ProvenanceTracker`.
- `[ ]` `knowledge_graph_enricher.cpp` has `vector_index_ = nullptr` guard (non-owning, "offline/stub"); inject a real `VectorIndexManager*` for production builds; fail fast (not silently degrade) when `nullptr` in non-test builds.
- `[ ]` Add integration test for provenance lineage round-trip: create training sample, verify AQL provenance query returns correct lineage.

---


**Priority:** High
**Target Version:** v0.9.0

Extend `LegalAutoLabeler` in `auto_labeler.cpp` to detect and separately process multiple content modalities within a single legal document: plain text clauses, structured tables (e.g., damages schedules), embedded citations, and scanned-image pages (via OCR). Each modality produces modality-typed `LabeledSample` records with distinct feature extractors.

**Implementation Notes:**
- Add a `ModalityDetector` class to `auto_labeler.cpp` that inspects document content type (MIME, layout heuristics) and dispatches to per-modality extractors: `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, `OCRExtractor`.
- `OCRExtractor` wraps an optional Tesseract or PaddleOCR shared library; gate this modality behind a build-time feature flag to avoid mandatory dependency.
- Each `LabeledSample` must carry a `modality` enum field so downstream filtering in `training_pipeline.cpp` can apply modality-specific confidence thresholds.
- Emit per-modality extraction statistics via `utils/logger.cpp` at INFO level including document URN, sample count, and mean confidence per modality.

**Performance Targets:**
- Text modality extraction throughput: >50 documents/s (1–10 page legal briefs) per core.
- OCR modality: >5 pages/s (TIFF, 300 DPI) on CPU; >20 pages/s with GPU acceleration.

---

### Acceptance Criteria

- [ ] Replace the in-process stub with `AQLRunner::execute(provenance_query, bindings)` calls; inject `AQLRunner*` into `ProvenanceTracker`.
- [ ] `knowledge_graph_enricher.cpp` has `vector_index_ = nullptr` guard (non-owning, "offline/stub"); inject a real `VectorIndexManager*` for production builds; fail fast (not silently degrade) when `nullptr` in non-test builds.
- [ ] Add integration test for provenance lineage round-trip: create training sample, verify AQL provenance query returns correct lineage.
- [ ] Add a `ModalityDetector` class to `auto_labeler.cpp` that inspects document content type (MIME, layout heuristics) and dispatches to per-modality extractors: `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, `OCRExtractor`.
- [ ] `OCRExtractor` wraps an optional Tesseract or PaddleOCR shared library; gate this modality behind a build-time feature flag to avoid mandatory dependency.
- [ ] Each `LabeledSample` must carry a `modality` enum field so downstream filtering in `training_pipeline.cpp` can apply modality-specific confidence thresholds.
- [ ] Emit per-modality extraction statistics via `utils/logger.cpp` at INFO level including document URN, sample count, and mean confidence per modality.
- [ ] Text modality extraction throughput: >50 documents/s (1–10 page legal briefs) per core.
- [ ] OCR modality: >5 pages/s (TIFF, 300 DPI) on CPU; >20 pages/s with GPU acceleration.

### Relationships

- Roadmap row: #120 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/training/FUTURE_ENHANCEMENTS.md#provenancetracker-replace-aql-template-stubs-with-live-connection
- Source key: roadmap:120:training:v1.8.0:provenancetracker-replace-aql-template-stubs-with-live-connection

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:120:training:v1.8.0:provenancetracker-replace-aql-template-stubs-with-live-connection -->
<!-- roadmap-ref: row=120;module=training;target=v1.8.0 -->
<!-- roadmap-detail: src/training/FUTURE_ENHANCEMENTS.md#provenancetracker-replace-aql-template-stubs-with-live-connection -->
