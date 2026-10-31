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

<!-- Status: current | validated: 2026-05-31 | issue #5678 verification: 2026-08-07 ✓ -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · TRAINING_PHASE_ACCEPTANCE_CHECKLIST.md -->

## Scope

- hardening and refinement of training runtime behavior
- deterministic reliability improvements for dataset/training/adapter paths
- stronger benchmark-backed guardrails for training hot paths
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
| DB sample reader | QueryEngine-backed collection/AQL reads with stable batching, limits, and explicit error reporting |
| DB training injection | non-owning QueryEngine integration for trainer/pipeline with documented lifetime and thread-safety |

## Implementation Notes

- tighten parity between checkpoint integrity behavior and training diagnostics.
- standardize incident taxonomy for labeling, enrichment, and serving classes.
- expand resilience tests for prolonged adapter lifecycle workloads.
- broaden benchmark depth for training-pipeline and checkpoint scenarios.
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
- focused RocksDB/QueryEngine tests for empty collections, malformed samples, batch boundaries, and query failures.
- benchmark smoke tests must emit one JSON result per path and pass through the comparison-report fixture.

## Performance Targets

- training hot paths remain inside regression budgets.
- adapter lifecycle and training-step-sensitive operations remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict bounded behavior for training and checkpoint transitions.
- preserve explicit failure signaling for labeling, adapter, and serving faults.
- enforce predictable degradation under capability or runtime pressure.
- keep diagnostics actionable for production training incidents.

## DB-Backed Training Data Path Benchmarks

### Scope
- Measure sample acquisition and LoRA training for every supported input route: built-in AQL samples, custom/in-memory samples, JSON file/object import, prepared `TrainingData` batches, AQL iterator, direct collection training, and TrainingPipeline end-to-end.

### Design Constraints
- DB paths use an isolated RocksDB-backed `QueryEngine` fixture and deterministic records.
- Query/collection failure must not silently fall back to synthetic samples.
- Model-backed fine-tuning requires a real GGUF artifact and is reported separately from the CPU LoRA update-core benchmark.

### Required Interfaces
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
