> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Training Module - Future Enhancements

This document covers planned enhancements to ThemisDB's legal-domain model training subsystem, which provides `LegalAutoLabeler` (`auto_labeler.cpp`) for extracting training samples from legal documents via NLP modality detection, an incremental LoRA adapter trainer with checkpoint/resume (`incremental_lora_trainer.cpp`), `KnowledgeGraphEnricher` (`knowledge_graph_enricher.cpp`) for enriching samples via AQL graph traversal, and a confidence-threshold filtering pipeline (`training_pipeline.cpp`). The module is Alpha-stage and requires hardened data provenance tracking, production-grade checkpoint management, and expanded training modalities before Beta.

## Design Constraints

- All training samples must carry a provenance record linking them to their source document (by URN), extraction timestamp, and labeler version; samples without provenance must be rejected by the training pipeline.
- LoRA adapter training must be resumable from the last saved checkpoint without data loss; checkpoint files must be validated by checksum before use.
- The `KnowledgeGraphEnricher` AQL traversal queries must be read-only and must not modify graph state; write operations from the enricher are forbidden.
- Confidence-threshold filtering in `training_pipeline.cpp` must be configurable per legal domain category (e.g., contract law vs. case law) to account for differing label certainty across modalities.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `LegalAutoLabeler::extract(document_urn)` | `training_pipeline.cpp` | Returns `LabeledSample[]` with confidence scores |
| `IncrementalLoRATrainer::train_step(batch)` | `training_pipeline.cpp` | Stateful; persists checkpoint after each epoch |
| `IncrementalLoRATrainer::resume(checkpoint_path)` | `training_pipeline.cpp` | Validates checksum; throws on corrupt checkpoint |
| `KnowledgeGraphEnricher::enrich(sample)` | `training_pipeline.cpp` | Issues AQL `FOR … RETURN` queries; read-only |
| `TrainingPipeline::run(config)` | Admin API, scheduled job | Orchestrates label → enrich → filter → train cycle |

## Planned Features

### [x] `ProvenanceTracker`: Replace AQL Template Stubs with Live Connection
**Priority:** High
**Target Version:** v1.8.0

`provenance_tracker.cpp` line 36 documents: "AQL template stubs (production: bind against live ArangoDB connection)". The provenance tracker uses in-process simulation for AQL-based lineage queries (line 141: "build a stub tree from in-process store"). In production, provenance information is not queryable via the AQL API.

**Implementation Notes:**
- `[x]` Replace the in-process stub with `AQLRunner::execute(provenance_query, bindings)` calls; inject `AQLRunner*` into `ProvenanceTracker`.
- `[x]` `knowledge_graph_enricher.cpp` has `vector_index_ = nullptr` guard (non-owning, "offline/stub"); inject a real `VectorIndexManager*` for production builds; fail fast (not silently degrade) when `nullptr` in non-test builds.
- `[x]` Add integration test for provenance lineage round-trip: create training sample, verify AQL provenance query returns correct lineage.

---

### [x] Multi-Modality Legal Document Parser
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

### [x] LoRA Checkpoint Manager with Integrity Validation
**Priority:** High
**Target Version:** v0.9.0

Harden the checkpoint save/resume path in `incremental_lora_trainer.cpp` with SHA-256 integrity validation, atomic checkpoint rotation, and automatic rollback to the previous checkpoint on corruption detection.

**Implementation Notes:**
- Implement `LoRACheckpointManager` in a new `lora_checkpoint_manager.cpp`; writes checkpoints atomically (write to `.tmp`, compute SHA-256, rename) and maintains a rolling window of the last three checkpoints.
- On `resume()`, `IncrementalLoRATrainer` delegates checksum validation to `LoRACheckpointManager`; if the latest checkpoint is corrupt, automatically falls back to the previous checkpoint and emits a WARN log via `utils/logger.cpp`.
- Store checkpoint metadata (epoch, step, loss, SHA-256, base model hash) in a `checkpoint_manifest.json` alongside the adapter weights.
- Emit a `training_checkpoint_age_seconds` gauge to the timeseries module (`timeseries/ts_auto_buffer.h`) so operators can alert on stale checkpoints.

**Performance Targets:**
- Checkpoint write time for a 7B-parameter LoRA adapter (fp16, r=16): <30 s on NVMe.
- SHA-256 validation of a 1 GB checkpoint file: <2 s (using `utils/zstd_codec.cpp` streaming interface for chunked reads).

---

### [x] Knowledge Graph Enrichment Query Cache
**Priority:** Medium
**Target Version:** v0.9.0

Add a read-through LRU cache to `knowledge_graph_enricher.cpp` to avoid redundant AQL traversal queries for the same entity or citation. Enrichment results are cached by (entity_key, graph_version) and invalidated when the underlying AQL graph is updated.

**Implementation Notes:**
- Implement `EnrichmentCache` in `knowledge_graph_enricher.cpp` using a thread-safe LRU map (capacity configurable, default 50k entries); cache key is `hash(entity_key + graph_schema_version)`.
- Graph schema version is read from the AQL metadata API on enricher startup and refreshed every 5 minutes; version change triggers a full cache evict.
- Cache hit/miss counters must be emitted via `utils/tracing.cpp` as span attributes so cache effectiveness is visible in distributed traces.
- The enrichment cache must respect the read-only constraint; it caches query results only and never issues AQL write operations.

**Performance Targets:**
- AQL query latency (cache miss): <20 ms P99 on a 10M-node legal knowledge graph.
- Cache hit ratio after warm-up on a typical training batch: >70%.
- Memory footprint of 50k cached enrichment results: <200 MB.

---

### [x] Confidence-Threshold Auto-Calibration
**Priority:** Medium
**Target Version:** v0.10.0

Replace the static per-category confidence thresholds in `training_pipeline.cpp` with an auto-calibration mechanism that adjusts thresholds based on downstream model evaluation scores. After each training epoch, the calibrator correlates per-sample confidence with the trained model's validation accuracy to set thresholds that maximise F1 on the validation set.

**Implementation Notes:**
- Add `ConfidenceCalibrator` class to `training_pipeline.cpp` that consumes per-sample `(confidence, model_correct)` pairs from the validation loop in `incremental_lora_trainer.cpp` and computes optimal thresholds via isotonic regression.
- Calibrated thresholds are stored in a `calibration_manifest.json` alongside each LoRA checkpoint (managed by `lora_checkpoint_manager.cpp`).
- Expose per-category calibrated thresholds as observable metrics so data scientists can review them via the Grafana dashboard.
- `LegalAutoLabeler` must surface per-sample confidence as a float in `[0, 1]` for all modalities; opaque boolean labels must be prohibited.

**Performance Targets:**
- Calibration computation time after a 10k-sample validation run: <5 s.
- Post-calibration F1 improvement vs. static thresholds: ≥3% on the legal-domain benchmark dataset.

---

### [x] Training Sample Provenance and Lineage Tracking
**Priority:** High
**Target Version:** v0.9.0

Implement end-to-end provenance tracking for every training sample from source document through labeling, enrichment, filtering, and model training. Provenance records are stored in the AQL graph and queryable for compliance and audit purposes.

**Implementation Notes:**
- Extend `LabeledSample` struct with a `ProvenanceRecord` containing: source document URN, extraction timestamp, labeler version (`auto_labeler.cpp` build hash), modality type, and enrichment AQL query fingerprints.
- `training_pipeline.cpp` must write a `TrainingSample` vertex and `DerivedFrom` edges to the AQL graph for each sample accepted past the confidence threshold.
- Integrate `utils/audit_logger.cpp` to emit a structured audit event for every sample filtered out by the confidence threshold, including the sample ID, category, confidence score, and threshold applied.
- Provide a `lineage_query.aql` example that traces a trained model prediction back to its contributing training samples and source documents.

**Performance Targets:**
- Provenance write throughput: >1000 sample records/s to the AQL graph without blocking the training pipeline.
- Lineage query latency for 10-hop traversal (model → samples → documents): <500 ms on a 1M-node provenance graph.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >85% new code | Cover `ModalityDetector`, `LoRACheckpointManager`, `ConfidenceCalibrator`, `EnrichmentCache` |
| Integration | Full pipeline: label → enrich → filter → train → checkpoint | Use synthetic legal document fixture corpus |
| Reliability | Checkpoint corruption recovery | Inject SHA-256 mismatch; verify rollback to previous checkpoint |
| Performance | P99 < budgets above | Extraction throughput, AQL cache hit ratio, provenance write throughput |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Text modality extraction throughput | ~10 docs/s | >50 docs/s | Per-core microbenchmark with 5-page legal briefs |
| AQL enrichment latency (cache miss) | ~80 ms | <20 ms | Knowledge graph query benchmark |
| AQL enrichment cache hit rate | 0% (no cache) | >70% | Cache effectiveness measurement on training corpus |
| Checkpoint write (7B LoRA, fp16) | ~120 s | <30 s | NVMe sequential write benchmark |
| Provenance write throughput | N/A | >1000 records/s | AQL bulk-insert benchmark |
| Confidence calibration time (10k samples) | N/A | <5 s | Isotonic regression microbenchmark |

## Security / Reliability

- [ ] Training samples derived from client legal documents must be processed under the client's tenant key; cross-tenant sample leakage must be architecturally impossible via separate AQL graph namespaces per tenant.
- [ ] `LegalAutoLabeler` must PII-scan extracted text samples via `utils/pii_detector.cpp` before storing them as `LabeledSample` records; samples containing unredacted PII must be rejected and logged to `utils/audit_logger.cpp`.
- [ ] LoRA checkpoint files must be encrypted at rest using `utils/hkdf_helper.cpp`-derived keys; plaintext checkpoints on shared storage are prohibited.
- [?] Clarify whether trained LoRA adapters that have seen client-privileged legal documents constitute privileged work product and therefore require special deletion procedures.
- [ ] The `KnowledgeGraphEnricher` must use a read-only AQL database user credential; the training service account must not hold write permissions on the legal knowledge graph.
- [ ] `training_pipeline.cpp` must emit a SAGA log entry (`utils/saga_logger.cpp`) for each pipeline run to enable compensating transaction rollback if a downstream model deployment fails post-training.

---

## References

[1] Hu, E. J., Shen, Y., Wallis, P., Allen-Zhu, Z., Li, Y., Wang, S., … Chen, W. (2022).
    **LoRA: Low-Rank Adaptation of Large Language Models.**
    *Proceedings of the 10th International Conference on Learning Representations (ICLR).*
    https://arxiv.org/abs/2106.09685

[2] Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023).
    **QLoRA: Efficient Finetuning of Quantized LLMs.**
    *Advances in Neural Information Processing Systems (NeurIPS)*, 36.
    https://arxiv.org/abs/2305.14314

[3] Auer, S., Bizer, C., Kobilarov, G., Lehmann, J., Cyganiak, R., & Ives, Z. G. (2007).
    **DBpedia: A Nucleus for a Web of Open Data.**
    *Proceedings of the 6th International Semantic Web Conference (ISWC)*, 722–735.
    https://doi.org/10.1007/978-3-540-76298-0_52

[4] Bellman, R. (1957).
    **Dynamic Programming.**
    Princeton University Press.
    *(isotonic regression / calibration mathematical foundation)*

[5] Mitra, B., & Craswell, N. (2018).
    **An Introduction to Neural Information Retrieval.**
    *Foundations and Trends in Information Retrieval*, 13(1), 1–126.
    https://doi.org/10.1561/1500000061
    *(knowledge graph traversal and enrichment patterns)*

[6] Ribeiro, M. T., Singh, S., & Guestrin, C. (2020).
    **Beyond Accuracy: Behavioral Testing of NLP Models with CheckList.**
    *Proceedings of the 58th Annual Meeting of the Association for Computational Linguistics (ACL)*, 4902–4912.
    https://doi.org/10.18653/v1/2020.acl-main.442
    *(modality-aware confidence calibration for NLP)*

[7] Carta, S., Giuliani, A., Piano, L., Podda, A. S., Pompianu, L., & Tiddia, S. G. (2023).
    **Iterative Zero-Shot LLM Prompting for Knowledge Graph Construction.**
    arXiv preprint arXiv:2307.01128.
    *(knowledge graph enrichment for training data)*


---

## ✅ Implemented — AdaLoRA, LoRAAdapterMerger, and LoRA+ (v1.6.0)

### AdaLoRA — Adaptive Budget Allocation
- **Status**: ✅ Implemented in `include/training/ada_lora_adapter.h` + `src/training/ada_lora_adapter.cpp`
- Importance scoring: per-rank-component B/A norm product approximation
- `reallocateRanks(budget)`: proportional allocation with [1, max_rank] bounds
- Active-rank forward pass (only unpruned rank components used)
- 36 tests: `tests/test_ada_lora_adapter.cpp`; CMake target: `AdaLoRAFocusedTests`
- Reference: Zhang et al. (2023), *AdaLoRA*, arXiv:2303.10512

### LoRAAdapterMerger — Multi-Adapter Composition
- **Status**: ✅ Implemented in `include/training/lora_adapter_merger.h` + `src/training/lora_adapter_merger.cpp`
- `mergeLinear()` / `mergeLinearAll()`: weighted ΔW sum + SVD-based (B', A') factorisation
- `mergeTIES()` / `mergeTIESAll()`: Trim (threshold) → Resolve (majority-vote sign) → Merge (sign-consistent average)
- 32 tests: `tests/test_lora_adapter_merger.cpp`; CMake target: `LoRAMergerFocusedTests`
- References: Ilharco et al. (2023), *Task Arithmetic*, arXiv:2212.04089;
              Yadav et al. (2023), *TIES-Merging*, arXiv:2306.01708

### LoRA+ — Asymmetric Learning Rates
- **Status**: ✅ Implemented via `IncrementalTrainingConfig::lora_plus_lambda`
- When `lora_plus_lambda > 1.0`: B uses `lr * λ`, A uses `lr` (two separate AdamOptimizer instances)
- Backward-compatible: default `1.0` preserves original behaviour
- Reference: Hayou et al. (2024), *LoRA+*, arXiv:2402.12354

---

## Paper 1 — Self-Optimising LoRA Loops (IMPL-A1, IMPL-A3)

> Full research paper: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
> See also: `include/training/FUTURE_ENHANCEMENTS.md` §Paper 1

### DATABASE_OPTIMIZER Domain (IMPL-A1)
- `DomainType::DATABASE_OPTIMIZER` in `auto_labeler.h` / `auto_labeler.cpp`
- Confidence function: `tanh(|Δlatency_ms| / 50)` — reject if < 0.85
- Minimum golden dataset: 1 000 labeled `(query, explain_plan, Δlatency_ms)` pairs

### Federation Bridges (IMPL-A3)
- `IncrementalLoRATrainer::exportGradient()` → `EncryptedGradient` (AES-256-GCM)
- `IncrementalLoRATrainer::applyGlobalDelta(const GlobalAdapterDelta&)` → FedAvg weight update
- Privacy invariant enforced by unit test: raw query text absent from gradient blob
