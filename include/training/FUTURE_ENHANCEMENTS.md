# Training Module - Future Header Enhancements

## Scope

- `ITrainingPipeline` interface extensions for orchestration, async execution, and multi-modality input
- LoRA checkpoint manager API (`ILoRACheckpointManager`) with content-addressed SHA-256 integrity
- Sample provenance tracking interface (`ISampleProvenanceTracker`) with append-only lineage records
- Knowledge graph enrichment interface (`IKGEnrichmentInterface`) with query cache integration
- Confidence calibration hook (`IConfidenceCalibrator`) for threshold auto-calibration without weight mutation
- Lineage query API for read-only traversal of sample-to-model provenance chains

## Design Constraints

- `[x]` `ITrainingPipeline` is **async throughout**; no blocking calls on the public interface; all long-running operations return `std::future` or accept completion callbacks
- `[x]` LoRA checkpoints are content-addressed (SHA-256); checkpoint identifiers are derived from content and cannot be forged
- `[x]` `ISampleProvenanceTracker` is **append-only**; provenance records cannot be deleted or modified after write
- `[x]` `IConfidenceCalibrator` adjusts calibration thresholds only; it must never modify model weights or gradient state
- `[x]` Lineage query API is **read-only**; all mutation paths are absent from the public lineage interface
- `[x]` Knowledge graph enrichment queries are cached; the cache key is a deterministic hash of the query parameters

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ITrainingPipeline` | Orchestrator, trainer service | Async; exposes `submit(TrainingJob) -> std::future<TrainingResult>`, `cancel(JobId)`, `status(JobId) -> JobStatus` |
| `ILoRACheckpointManager` | Checkpoint store, model loader | Content-addressed; exposes `save(LoRAWeights) -> CheckpointId`, `load(CheckpointId) -> LoRAWeights`, `verify(CheckpointId) -> bool` |
| `ISampleProvenanceTracker` | Data pipeline, audit system | Append-only; exposes `record(SampleProvenance)`, `queryLineage(SampleId) -> LineageGraph` |
| `IKGEnrichmentInterface` | Feature extractor, training data enricher | Cached; exposes `enrich(EntityRef) -> EnrichmentResult`, `invalidateCache(EntityRef)` |
| `IConfidenceCalibrator` | Inference pipeline, evaluation harness | Read/write on thresholds only; exposes `calibrate(CalibrationDataset) -> CalibrationResult`, `currentThresholds() -> ThresholdMap` |
| `ILineageQueryAPI` | Audit tooling, compliance reporting | Read-only; exposes `getProvenance(ModelId) -> std::vector<SampleRef>`, `getSampleOrigin(SampleId) -> OriginRecord` |

## Planned Features

### LoRA Checkpoint Manager Interface

- `[x]` Define `ILoRACheckpointManager` with `save(const LoRAWeights&) -> CheckpointId` returning a SHA-256-derived content address
- `[x]` Add `load(CheckpointId) -> std::future<LoRAWeights>` — async to support large checkpoint retrieval from remote stores
- `[x]` Add `verify(CheckpointId) -> bool` to re-derive the content hash and confirm integrity; returns `false` for tampered checkpoints
- `[x]` Expose `listCheckpoints(ModelId) -> std::vector<CheckpointDescriptor>` with `id`, `createdAt`, `sizeBytes`, `signatureValid`

### Sample Provenance and Lineage Tracking API

- `[x]` Define `ISampleProvenanceTracker` with `record(const SampleProvenance&)` — append-only, no update or delete
- `[x]` `SampleProvenance` carries `sampleId`, `sourceUri` (opaque, not raw content), `collectedAt`, `preprocessingSteps`, `datasetVersion`
- `[x]` Add `queryLineage(SampleId) -> LineageGraph` returning a DAG of transformations from raw source to training-ready sample
- `[x]` Tracker exposes `totalRecords() -> size_t` and `storageEstimateBytes() -> size_t` for capacity planning; no raw content accessors

### Knowledge Graph Enrichment Query Cache

- `[x]` Define `IKGEnrichmentInterface` with `enrich(const EntityRef&) -> EnrichmentResult` — results served from cache when available
- `[x]` `EnrichmentResult` carries `entityId`, `relations` (`std::vector<KGRelation>`), `confidence`, `cacheHit`
- `[x]` Add `invalidateCache(const EntityRef&)` for targeted cache invalidation and `clearCache()` for full eviction
- `[x]` Cache key is derived deterministically from `EntityRef` fields; the interface exposes `cacheStats() -> CacheStats` with hit/miss/eviction counts

### Confidence-Threshold Auto-Calibration Interface

- `[x]` Define `IConfidenceCalibrator` with `calibrate(const CalibrationDataset&) -> std::future<CalibrationResult>`
- `[x]` `CalibrationResult` carries `updatedThresholds` (`ThresholdMap`), `calibrationError`, `samplesUsed`
- `[x]` Add `currentThresholds() -> const ThresholdMap&` and `applyThresholds(const ThresholdMap&)` — both operate on threshold state only, never on model weights
- `[x]` Calibrator exposes `resetToDefaults()` to restore factory thresholds without affecting any persisted model artefact

### Training Pipeline Orchestration API

- `[x]` Define `ITrainingPipeline` with `submit(TrainingJob) -> std::future<TrainingResult>` — fully async, non-blocking
- `[x]` `TrainingJob` carries `datasetRef`, `modelConfig`, `loraConfig` (optional), `provenanceTracker` (optional injection point)
- `[x]` Add `cancel(JobId) -> CancelResult` (best-effort, returns `Cancelled` or `AlreadyCompleted`)
- `[x]` Expose `status(JobId) -> JobStatus` with states: `Queued`, `Running`, `Completed`, `Failed`, `Cancelled`

## Test Strategy

- LoRA checkpoint round-trip tests: save weights, reload, compare byte-for-byte; also corrupt one byte and assert `verify()` returns `false`
- Provenance tracker append-only tests: attempt to overwrite a record and assert it is rejected; verify `queryLineage()` returns the correct DAG
- KG enrichment cache tests: same `EntityRef` queried twice — second call must return `cacheHit = true`; after `invalidateCache()`, next call must miss
- Confidence calibrator tests verify that `calibrate()` never alters any field of `LoRAWeights` passed in via the same pipeline context
- Lineage query API tests assert all exposed methods are read-only and that no mutation path compiles through the interface
- Training pipeline async tests: submit 10 concurrent `TrainingJob` instances, verify futures resolve independently and `cancel()` during execution returns `Cancelled`

## Performance Targets

- `ILoRACheckpointManager::save()` for a 1 GB checkpoint: **≤ 5 s**
- `ISampleProvenanceTracker::record()` per sample: **≤ 1 ms**
- `IKGEnrichmentInterface::enrich()` cache hit: **≤ 10 ms**; cache miss: **≤ 50 ms**
- `IConfidenceCalibrator::calibrate()` on a 10,000-sample dataset: **≤ 500 ms**
- `ITrainingPipeline::submit()` queuing overhead (not including training): **≤ 5 ms**
- `ILineageQueryAPI::getProvenance()` for a model with 100,000 sample refs: **≤ 200 ms**

## Security / Reliability

- Training samples are validated against PII detection before any `ISampleProvenanceTracker::record()` call; samples failing PII validation are rejected, not stored
- Model checkpoints are signed via Ed25519 at save time; `ILoRACheckpointManager::verify()` validates both content hash and Ed25519 signature
- `ILineageQueryAPI` enforces read-only access at the interface level; no write methods exist; unauthorized access throws `PermissionDeniedError`
- `ISampleProvenanceTracker` stores only opaque source URIs and metadata; raw training content is never retained in provenance records
- `IConfidenceCalibrator` is prohibited from accessing gradient state or weight tensors; calibration operates exclusively on threshold scalars
- `IKGEnrichmentInterface` cache entries have TTL enforcement; stale enrichment data is evicted before use to prevent poisoning of training features

---

## References

[1] Hu, E. J., Shen, Y., Wallis, P., Allen-Zhu, Z., Li, Y., Wang, S., … Chen, W. (2022).
    **LoRA: Low-Rank Adaptation of Large Language Models.**
    *Proceedings of the 10th International Conference on Learning Representations (ICLR).*
    https://arxiv.org/abs/2106.09685

[2] Bernstein, P. A., & Newcomer, E. (2009).
    **Principles of Transaction Processing** (2nd ed.). Morgan Kaufmann.
    *(append-only provenance store design and lineage DAG patterns)*

[3] Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023).
    **QLoRA: Efficient Finetuning of Quantized LLMs.**
    *Advances in Neural Information Processing Systems (NeurIPS)*, 36.
    https://arxiv.org/abs/2305.14314
    *(content-addressed checkpoint IDs and integrity verification)*

[4] Boneh, D., & Shoup, V. (2023).
    **A Graduate Course in Applied Cryptography** (Draft v0.6).
    https://toc.cryptobook.us
    *(Ed25519 checkpoint signing, SHA-256 content addressing, key derivation)*

[5] Sculley, D., Holt, G., Golovin, D., Davydov, E., Phillips, T., Ebner, D., … Young, M. (2015).
    **Hidden Technical Debt in Machine Learning Systems.**
    *Advances in Neural Information Processing Systems (NIPS)*, 28.
    *(IConfidenceCalibrator design rationale: avoid weight mutation via calibrator)*

[6] Zaharia, M., Chowdhury, M., Franklin, M. J., Shenker, S., & Stoica, I. (2010).
    **Spark: Cluster Computing with Working Sets.**
    *Proceedings of the 2nd USENIX Workshop on Hot Topics in Cloud Computing (HotCloud)*, 10.
    *(async pipeline orchestration patterns for ITrainingPipeline)*

---

## Paper 1 — Self-Optimising LoRA Loops (Cross-Module Vision)

> Full research paper: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`
> Master plan: `docs/issues/MASTER_IMPLEMENTATION_PLAN.md`

### DATABASE_OPTIMIZER Domain (IMPL-A1)
- Extend `DomainType` enum with `DATABASE_OPTIMIZER` for automatic labeling of `(query, explain_plan, Δlatency_ms)` triples
- Golden dataset construction from real workload captures; minimum 1 000 labeled pairs
- Confidence scoring: `tanh(|Δlatency_ms| / 50)` — reproducible, workload-agnostic

### Loop Orchestration Integration (IMPL-A2)
- `IncrementalLoRATrainer` is Loop 4 in `ContinuousLearningOrchestrator`; it must be invokable via `triggerLoop4AdapterImprovement()`
- Loop 4 completion fires `FEDERATED_ROUND_START` event (24 h cooldown guard)

### Federation Bridges (IMPL-A3)
- `exportGradient()` → `EncryptedGradient` (AES-256-GCM; no raw sample content may appear in blob)
- `applyGlobalDelta(const GlobalAdapterDelta&)` → FedAvg aggregate applied to local adapter weights
- Consumed by `LoRAFederationCoordinator` in `distributed_knowledge` module (Layer B)

### Performance Targets
- `exportGradient()` serialisation ≤ 50 ms for rank-32 adapter
- `applyGlobalDelta()` weight update ≤ 100 ms for rank-32 adapter
- Loop 4 full training cycle ≤ 30 min on single GPU, 5 min on 4-GPU cluster
