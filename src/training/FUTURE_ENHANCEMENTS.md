---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
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
- retain focused RocksDB/QueryEngine coverage for bounded trainer reads, invalid collection names, empty data, persisted labels, and pipeline sample counts; add tests for selection-to-training semantics and remaining failure/batch boundaries.
- add benchmark smoke and comparison-report validation when per-route DB benchmarks are implemented; current `benchmarks/training/` targets do not provide those cases.

## Performance Targets

- training hot paths remain inside regression budgets.
- establish p95/p99 envelopes from reproducible release-profile measurements before treating them as targets.
- map supported input routes to benchmarks before claiming complete release-gate coverage.

## Security / Reliability

- maintain strict bounded behavior for training and checkpoint transitions.
- preserve explicit failure signaling for labeling, adapter, and serving faults.
- enforce predictable degradation under capability or runtime pressure.
- keep diagnostics actionable for production training incidents.

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
