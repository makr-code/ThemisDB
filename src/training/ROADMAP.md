# Training Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 | issue #5678 verification: 2026-08-07 ✓ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · TRAINING_PHASE_ACCEPTANCE_CHECKLIST.md -->

## Current Status

Production-usable training runtime exists for labeling, enrichment, LoRA/AdaLoRA training, checkpoint handling, and training pipeline orchestration.

## In Progress

- [~] hardening training and checkpoint behavior under extended adapter lifecycle pressure (Target: Q3 2026)
- [~] improving diagnostics consistency across labeling, training, and serving-handoff stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for training hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for adapter merge and rollback edge scenarios (Target: Q4 2026)
- [ ] expand stress coverage for GPU-backed and checkpoint-resume workloads (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for training and provenance incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for adapter lifecycle and training-step-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for training pipeline and enrichment workload diversity (Target: Q1 2027)
- [ ] harden long-run reliability under sustained training and deployment pressure (Target: Q1 2027)
- [~] Wave B B3: Multi-task LoRA shared-base/domain-gating training rollout (Target: Q1–Q2 2027) — core impl + ablation/benchmark tests done

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze training, checkpoint, and adapter lifecycle contracts for current major line (2026-08-09: TRAINING_CONTRACT.md created; lifecycle states, adapter invariants, checkpoint atomicity frozen)
- [x] define explicit error taxonomy for labeling, checkpoint, and serving incidents (2026-08-09: TRAINING_CONTRACT.md §4 references frozen TrainingErrorCode; summary table for 3 incident classes)

### Phase 2: Core Implementation
- [ ] complete hardening for trainer, checkpoint, and merge internals (Target: Q4 2026)
- [ ] align enrichment and serving-handoff behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for checkpoint faults, adapter merge failures, and enrichment gaps (Target: Q4 2026)
- [ ] unify diagnostics across dataset, training, and adapter incident classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for LoRA/AdaLoRA, checkpoint, and serving edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for training-lifecycle workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for training hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core training module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core training surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for trainer/checkpoint/adapter edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on training configuration, adapter size, and available acceleration.
- selected checkpoint, merge, and serving edge scenarios need continued hardening.
- benchmark depth should continue expanding for broader training workloads.
- Wave B B3 rollout depends on Wave A deployment readiness and stable LLM adapter lifecycle baselines.

## Wave B (Q1–Q2 2027) Tracking — B3 Multi-Task LoRA Training

### Scope
- [x] shared LoRA base training flow with task-specific projection support
- [x] domain-gating training signals and routing metadata integration
- [x] joint multi-task loss with configurable task weighting
- [x] benchmark orchestration for three-task transfer and robustness evaluation

### Validation
- [x] unit tests `MTL-TRAINING-01..10`
- [x] ablation study: shared-base vs separate-adapter training behavior

### Acceptance Gates
- [ ] average task performance gain ≥ +8% vs single-task baseline
- [ ] training-time increase ≤ 15% across benchmarked task sets
- [ ] robust convergence behavior across configured task-weight schedules

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM module adapter lifecycle baseline stable for multi-task integration
- [ ] benchmark harness available for repeatable three-task evaluations

### References
- AI tracker: `../ai/ROADMAP.md`
- LLM tracker: `../llm/ROADMAP.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`

## Breaking Changes

No breaking training contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.