---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-13"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Training Module - Future Enhancements

<!-- Source review: 2026-10-31; remaining benchmark and selection-to-training items are open. -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · TRAINING_PHASE_ACCEPTANCE_CHECKLIST.md -->

## Scope

- hardening and refinement of training runtime behavior
- deterministic reliability improvements for dataset/training/adapter paths
- stronger benchmark-backed guardrails for training hot paths
- benchmark coverage for the implemented bounded trainer collection-read and persisted-label pipeline paths
- explicit, tested dataflow semantics between pipeline selection output and the trainer's configured collection
- per-source benchmark coverage for supported in-memory/custom, JSON, AQL iterator, collection, and full-pipeline routes
- bounded DB-backed sample readers and selection-aware training for IncrementalLoRATrainer, TrainingDataIterator, and TrainingPipeline
- per-source benchmark coverage for built-in, in-memory/custom, JSON, trainBatch, AQL iterator, collection, and full-pipeline paths
- keep native AdaLoRA consumption source-neutral through bounded `ITrainingSampleProvider` batches; docs.db remains one read-only provider adapter rather than a docs-specific trainer

## Design Constraints

- training contracts remain backward compatible within major release line.
- labeling and adapter lifecycle outcomes remain explicit and deterministic.
- degraded checkpoint and enrichment paths remain observable and non-silent.
- acceleration-dependent training behavior remains bounded and diagnosable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| dataset interfaces | deterministic labeling and enrichment behavior |
| training interfaces | stable LoRA/AdaLoRA lifecycle semantics |
| checkpoint interfaces | explicit save/resume/verify behavior |
| adapter interfaces | bounded merge and serving-handoff behavior |
| DB sample reader | Existing trainer/pipeline `QueryEngine` paths retain bounded reads, input validation, and explicit failure reporting |
| DB training injection | Existing non-owning `QueryEngine` injection keeps its lifetime contract; selection-to-training behavior needs explicit integration semantics |
| Training sample provider | `ITrainingSampleProvider` emits bounded normalized records with sample ID and source URI; providers own source parsing and resource lifetime |
| AdaLoRA consumer | `AdaLoRATrainingPipeline::trainFromProvider()` owns model-backed optimization, checkpointing, and adapter export independently of source format |

## Implementation Notes

- tighten parity between checkpoint integrity behavior and training diagnostics.
- standardize incident taxonomy for labeling, enrichment, and serving classes.
- expand resilience tests for prolonged adapter lifecycle workloads.
- broaden benchmark depth for training-pipeline and checkpoint scenarios.
- preserve the tested label-persistence and trainer collection-read behavior; clarify that the current selection stage does not pass its selected subset directly to the trainer.
- add separate DB read, conversion, and training measurements; never label synthetic or stub work as DB-backed training.
- persist auto-labeled samples before training and consume the configured target collection rather than an empty vector.
- pass the selected sample `_key` values from data selection into the trainer query; an empty selection must not fall through to full-collection or synthetic training.
- keep DB read/convert/train timing separate; never label deterministic synthetic fallback as DB-backed training.
- preserve the source-neutral consumer contract: AQL and docs.db providers emit identical normalized records, and provider failures never switch to a synthetic or alternate source.

### Wave B B3: Multi-Task LoRA Training Enablement
**Priority:** High
**Target:** Q1–Q2 2027

- add shared LoRA base training path with task-specific projection handling
- add domain-gating support for task-conditional adapter usage during training
- add joint multi-task loss pipeline with configurable task weighting
- add ablation workflows for shared-base vs separate-adapter comparisons
- run three-task transfer/robustness benchmarks for Wave B acceptance tracking

## Test Strategy

- unit and integration suites for labeling, training, and adapter lifecycle behavior.
- regressions for checkpoint corruption, merge conflicts, and rollback edge cases.
- deterministic stress runs for training and adapter-serving handoff workloads.
- release-profile benchmark runs for mapped training targets.
- test the generated docs.db import path and model-backed `trainFromDocsDatabase()` integration with a real TinyLlama GGUF; provider unit tests alone are insufficient end-to-end evidence.
- retain focused RocksDB/QueryEngine coverage for bounded trainer reads, invalid collection names, empty data, persisted labels, and pipeline sample counts; add tests for selection-to-training semantics and remaining failure/batch boundaries.
- add benchmark smoke and comparison-report validation when per-route DB benchmarks are implemented; current `benchmarks/training/` targets do not provide those cases.

## Performance Targets

- training hot paths remain inside regression budgets.
- establish p95/p99 envelopes from reproducible release-profile measurements before treating them as targets.
- map supported input routes to benchmarks before claiming complete release-gate coverage.

## Security / Reliability

- maintain strict bounded behavior for training and checkpoint transitions.
- The docs.db provider must use read-only RocksDB access and bounded record/sample limits; caller authorization and cryptographic adapter signing remain mandatory deployment controls.
- preserve explicit failure signaling for labeling, adapter, and serving faults.
- enforce predictable degradation under capability or runtime pressure.
- keep diagnostics actionable for production training incidents.

## Native Specialization to Multi-LoRA Serving (Proposal; Q2 2027)

### Scope

Qualify authorized DB-backed AdaLoRA training and explicit promotion to
Multi-LoRA serving on smaller local models, optionally via tensor artifact
representations. Reuse existing providers/trainers/managers; this proposal
does not claim full-model training, signed deployment or current quality gains.

### Design Constraints

- Keep base weights frozen and declare actual target tensors; the current
  native pipeline trains block-sharded `output.weight`, not arbitrary
  transformer layers.
- Preserve bounded source-neutral sample batches, adaptive rank and gradient/
  token/memory limits; authorization is a caller/deployment obligation until
  the required path is explicitly enforced.
- Training and generation use separate wrapper contexts. Schedule training
  under shared DB/inference resource admission, not on request threads or
  inside long-held transactions.
- Separate artifact production, signing/validation, quality approval, serving
  residency and request activation; no automatic online learning from
  unreviewed answers or retrieved instructions.

### Required Interfaces

- A manifest binding source/sample or supported snapshot fingerprint,
  split/dedup policy, base/tokenizer identity, target tensor names/shapes,
  rank schedule/budget, alpha/scaling, optimizer, seed and output digest.
- Explicit promotion/revocation/rollback linked to Multi-LoRA artifact
  identity and per-request activation; concurrent composition is a separate
  backend capability from loading multiple adapters.
- Optional AdaLoRA-to-TT export/import with dtype/layout/profile, scale/rank,
  measured reconstruction error and verified consumer bridge.

### Implementation Notes

- Preserve the current GGUF handoff and explicit load rejection, but add
  owner-approved quality/trust gates before production promotion.
- Do not generalize the signed snapshot path in the LLM module to all native
  pipeline exports; signing/authorization gaps remain path-specific.
- Define incremental retraining on source/model/profile changes and revoke
  affected artifacts when policy requires it. Deleting source rows alone
  does not demonstrate removal of memorized training information from weights.
- TT storage/fingerprint similarity does not establish semantic adapter
  interchangeability, exact equivalence or direct compressed llama.cpp execution.

### Test Strategy

- Real-GGUF training/export/load/inference with measured loss and held-out task
  results; cover provider failure, bounded batches, rank reallocation,
  interrupted checkpoints and incompatible base/target shapes.
- Verify signed/trusted promotion where implemented, unauthorized source
  rejection at the owning boundary, and rollback with in-flight requests.
- Test TT round-trip of reconstructed scaled updates and downstream outputs
  against the native artifact; cover zero/pruned ranks and non-finite data.
- Ablate base-only, RAG, adapter-only, RAG+adapter and tensor-enabled variants;
  use source-grouped train/validation splits to avoid overlapping-chunk leakage.

### Performance Targets

Report held-out task quality/forgetting, rank/parameter count, samples/target
tokens, training/checkpoint/export/load time, RAM/VRAM, inference TTFT and DB
p95/p99 under contention. Ratify numeric gates after a real model-backed
baseline; lifecycle microbenchmarks alone are not end-to-end evidence.

### Security / Reliability

Bound datasets, artifact sizes, training resources and promotion privileges.
Require explicit provenance/consent policy, artifact trust and audit retention.
Reject required-adapter failure rather than silently serve base-model output.
Support policy-driven revocation/retraining; do not promise machine unlearning.

Research anchors: [LoRA](https://arxiv.org/abs/2106.09685) and
[AdaLoRA](https://arxiv.org/abs/2303.10512), already referenced in
[the module roadmap](ROADMAP.md), motivate low-rank specialization, not
ThemisDB-specific performance or full-model-training claims.

## Future DB-Backed Training Data Path Benchmarks

### Scope
- No DB-backed training-data benchmark currently exists in `benchmarks/training/`. Add measurements for trainer collection reads, persisted-label pipeline runs, and the separate `llm` AQL iterator; include other input routes only after verifying that the current implementation supports them.

### Design Constraints
- DB paths use an isolated RocksDB-backed `QueryEngine` fixture and deterministic records.
- Query/collection failure must not silently fall back to synthetic samples.
- Model-backed fine-tuning requires a real GGUF artifact and is reported separately from the CPU LoRA update-core benchmark.

### Required Interfaces
- Preserve the existing non-owning `QueryEngine` contracts for `IncrementalLoRATrainer` and `TrainingPipeline`.
- Measure the current trainer read and pipeline persistence paths separately; document that pipeline selection results are not currently passed directly to the trainer.
- `TrainingDataIterator` is owned by `llm`; retain its bounded AQL batch contract and report it as a separate path.

### Test Strategy
- Extend the existing RocksDB tests to cover sample contents, malformed rows, limits, failed queries, and the selection/trainer boundary where applicable.
- Run benchmark executables separately from CTest; verify temporary database cleanup and machine-readable output when that reporting is implemented.
- `TrainingDataIterator` must execute a read-only AQL query and return bounded stable batches.
- `IncrementalLoRATrainer` and `TrainingPipeline` must accept a non-owning `QueryEngine` while preserving existing constructor call sites.
- Auto-labeled sample persistence and trainer reads must use the configured source/target collections.
- When data selection is enabled, training must be restricted to the returned sample IDs; no selected IDs or an unavailable DB engine is an explicit failure.

### Test Strategy
- Verify exact sample counts and content for each DB reader; cover empty, malformed, limited, and failed-query cases.
- Verify that the training sample count equals the selected ID count when data selection is enabled.
- Run each benchmark path in CTest with temporary DB cleanup and valid JSON output.

### Performance Targets
- Report database read, sample conversion, LoRA update, and end-to-end p50/p95 elapsed time, samples/s, and bytes/s separately.
- Compare the same host/profile against its stored baseline; flag throughput regression above 10 percent. Do not compare model-backed and CPU-core-only cases as equivalent workloads.

### Security / Reliability
- Validate collection identifiers before AQL construction and bind all data values.
- Bound result count and batch memory; expose skipped/invalid sample counts.
- Keep benchmark fixtures isolated; no model, prompt, or tenant data may be written outside the temporary benchmark DB.

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
## Wave B Acceptance Gates (B3)

- average task performance gain ≥ +8% vs single-task baseline
- training-time increase ≤ 15% across benchmarked multi-task workloads
- robust convergence and stability across configured task-weight schedules

## Related Documents

- AI wave tracker: `../ai/ROADMAP.md`
- LLM wave tracker: `../llm/ROADMAP.md`
- bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`
