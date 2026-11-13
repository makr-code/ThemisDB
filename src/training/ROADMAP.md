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

# Training Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 | issue #5678 verification: 2026-08-07 ✓ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · TRAINING_PHASE_ACCEPTANCE_CHECKLIST.md -->

## Current Status

The module implements labeling, enrichment, LoRA/AdaLoRA training, checkpoint handling, provenance, data selection, and pipeline orchestration. Its core runtime is functional and broadly covered by focused tests; deployment authorization/signing, representative performance evidence, and operational release gates remain incomplete. Treat this as a production-candidate subsystem, not an independently production-approved training service.

## In Progress

- [~] hardening training and checkpoint behavior under extended adapter lifecycle pressure (Target: Q3 2026)
- [~] improving diagnostics consistency across labeling, training, and serving-handoff stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for training hot paths (Target: Q3 2026)
- [x] implement real DB-backed sample loading and one benchmark per supported LoRA data path, including preparation, model-backed paths, AQL iterator, collection training, selection, and full pipeline (Target: Q4 2026)
- [~] unify AQL and docs.db TinyLlama AdaLoRA training behind a bounded provider/consumer contract; keep docs.db as a read-only source adapter (Target: Q4 2026)

## Planned Features

### Native Hybrid Specialization (Proposal; Target: Q2 2027)

- [ ] Qualify bounded source-neutral AdaLoRA training -> evaluated/versioned
  GGUF artifact -> explicit Multi-LoRA promotion and rollback; retain actual
  `output.weight` training scope (Target: Q2 2027 proposal).
- [ ] Bind sample/snapshot provenance, base/tokenizer, adaptive rank schedule,
  target shapes/scaling and artifact trust; test real-GGUF serving outcomes
  rather than equate export/load success with quality (Target: Q2 2027 proposal).
- [ ] Qualify optional AdaLoRA-to-TT round-trip and serving bridges with
  measured scaled-update error and held-out task quality; compare native,
  RAG and adapter/tensor variants under shared resource budgets
  (Target: Q2 2027 proposal).

Use the existing design/core/error/test/hardening/documentation phases for this
work; historical completion labels do not close these new gates. See
[the specialization plan](FUTURE_ENHANCEMENTS.md) and
[the hybrid boundary](../llm/ARCHITECTURE.md).

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for adapter merge and rollback edge scenarios (Target: Q4 2026)
- [x] expand stress coverage for GPU-backed and checkpoint-resume workloads (Wave D: test_training_highcardinality_stress.cpp — 3 cases delivered 2026-09-16)
- [x] improve operator-facing diagnostics for training and provenance incidents (Wave D: RUNBOOK_TRAINING_PIPELINE.md — 5 scenarios delivered 2026-09-16)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for adapter lifecycle and training-step-sensitive paths (Target: Q1 2027)
- [x] add dedicated in-process dispatch/checkpoint/sync benchmark cases (Wave D: `bench_training_dedicated_gates.cpp`, TR-BM-01..04; stub-only measurements, not pipeline/enrichment release evidence)
- [x] harden long-run reliability under sustained training and deployment pressure (Wave D: test_training_pipeline_soak.cpp — 3 soak cases delivered 2026-09-16)
- [~] Wave B B3: Multi-task LoRA shared-base/domain-gating training rollout (Target: Q1–Q2 2027) — core impl + ablation/benchmark tests done

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze training, checkpoint, and adapter lifecycle contracts for current major line (2026-08-09: TRAINING_CONTRACT.md created; lifecycle states, adapter invariants, checkpoint atomicity frozen)
- [x] define explicit error taxonomy for labeling, checkpoint, and serving incidents (2026-08-09: TRAINING_CONTRACT.md §4 references frozen TrainingErrorCode; summary table for 3 incident classes)

### Phase 2: Core Implementation
- [ ] complete hardening for trainer, checkpoint, and merge internals (Target: Q4 2026)
- [ ] align enrichment and serving-handoff behavior to bounded runtime contracts (Target: Q4 2026)
- [x] load bounded, validated samples from the configured ThemisDB collection into IncrementalLoRATrainer (Target: Q4 2026)
- [x] implement TrainingDataIterator AQL execution and connect TrainingPipeline QueryEngine injection, sample persistence, selected IDs, and training reads (Target: Q4 2026)
- [~] add the source-neutral `ITrainingSampleProvider` contract, read-only docs.db provider, and shared native AdaLoRA consumer with TinyLlama GGUF export (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for checkpoint faults, adapter merge failures, and enrichment gaps (Target: Q4 2026)
- [x] fail closed on DB query errors and invalid collection names; count and skip malformed/empty samples deterministically (Target: Q4 2026)
- [x] unify diagnostics across dataset, training, and adapter incident classes (Wave D: RUNBOOK_TRAINING_PIPELINE.md — 5 log patterns delivered 2026-09-16)

### Phase 4: Tests
- [x] AdaLoRAAdapter::saveToFile()/loadFromFile() checkpoint round-trip — LORA-04 updated (2026-08-24)
- [x] AdaLoRA rebuild-gate contract — LORA-08 (cache HIT / MISS / force-rebuild, 4-phase test) (2026-08-24)
- [x] expand focused regressions for LoRA/AdaLoRA, checkpoint, and serving edge scenarios (Wave D: test_training_highcardinality_stress.cpp delivered 2026-09-16)
- [x] extend deterministic stress fixtures for training-lifecycle workloads (Wave D: test_training_highcardinality_stress.cpp — HighCardinalityBatchDispatch, ConcurrentCheckpointStress, GradientAccumulationStress 2026-09-16)
- [x] add isolated RocksDB/QueryEngine tests for iterator batches, trainer sample counts, persisted labels, selection IDs, and DB-backed pipeline training (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for training hot paths (Wave D: bench_training_dedicated_gates.cpp — TR-BM-01..04 delivered 2026-09-16)
- [x] benchmark built-in, custom, JSON, trainBatch, AQL iterator, collection, selection, and pipeline paths with separate read/preprocess/train/end-to-end counters (Target: Q4 2026)
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
- model-backed source cases require a real GGUF artifact; without one, their benchmark preflight is reported as not run.
- docs.db training requires a generated RocksDB with valid `chunk_` records or legacy `doc_` previews and a real TinyLlama-compatible GGUF model; provider-only unit coverage is not end-to-end training evidence.
- training source authorization and cryptographic signing/verification of exported AdaLoRA artifacts remain unimplemented production gates.
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

## Evidence-Based Assessment (2026-11-07)

This assessment is based on the implementation, registered tests, and benchmark sources in `src/training/`, `include/training/`, `tests/training/`, and `benchmarks/training/`. It does not claim measured model-quality or production-SLO results.

| Area | Source-backed state | Gap and consequence |
|---|---|---|
| Performance and scale | LoRA/AdaLoRA, multi-task training, bounded DB reads, selection, and pipeline paths exist. `bench_training_lifecycle_gates` measures adapter construction/update/merge/import/export and a 1,000-update lifecycle; `bench_lora_data_paths` includes isolated RocksDB and model-backed cases. | Lifecycle thresholds are diagnostic, the dedicated queue/checkpoint/sync benchmark uses stubs, and release p95/p99 baselines are open. Model-backed cases need a real GGUF artifact. No distributed-training performance claim is supported yet. |
| Use and quality | The pipeline supports labeling, enrichment, quality/drift checks, selection, training, and explicit serving handoff. Selection uses filters, MinHash, hash-derived character-statistic clustering, and heuristic scoring. | `perplexity_model`/`embedding_model` settings do not prove model-backed scoring; OCR remains a placeholder. Multi-task acceptance tests use synthetic samples and do not establish gains on representative workloads. |
| Security and resilience | The module has explicit error codes/failsafes, checkpoint path and digest checks, provenance write timeouts, prompt-safety filtering, and checkpoint/adapter recovery tests. | `QueryEngine::setCollectionAccessChecker()` is optional and not required by training; trained adapter signing is not wired into persistence/deployment. A digest is not a signature or proof of signer identity. |
| Governance and operations | Pipeline/trainer metrics, provenance, benchmark guidance, runbooks, and focused DB tests are present. | Dataset/model evaluation records, enforceable release benchmarks, and a complete integration-owned authorization/key-management contract are not established. Serving handoff is not equivalent to deployment approval. |

### Source and evidence map

- Pipeline and selected-sample flow: [`training_pipeline.cpp`](./training_pipeline.cpp) passes selected IDs to `trainSelected()` when selection is enabled; [`test_training_pipeline_db.cpp`](../../tests/training/test_training_pipeline_db.cpp) covers DB-backed persistence/training. Older statements in [`TRAINING_CONTRACT.md`](../../include/training/TRAINING_CONTRACT.md), [`ARCHITECTURE.md`](./ARCHITECTURE.md), and this module's [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) say those IDs are not passed. Reconcile those secondary-doc claims against current code and tests before using them as acceptance evidence.
- Trainer bounds and result metrics: [`incremental_lora_trainer.h`](../../include/training/incremental_lora_trainer.h); DB and lifecycle coverage includes [`test_incremental_lora_db_training.cpp`](../../tests/training/test_incremental_lora_db_training.cpp), [`test_checkpoint_recovery.cpp`](../../tests/training/test_checkpoint_recovery.cpp), and [`test_deterministic_training_lifecycle.cpp`](../../tests/training/test_deterministic_training_lifecycle.cpp).
- Benchmark limits: [`benchmarks/training/README.md`](../../benchmarks/training/README.md), [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md), and the targets in [`benchmarks/training/CMakeLists.txt`](../../benchmarks/training/CMakeLists.txt). The `training` tests are glob-registered as focused unit tests in [`tests/training/CMakeLists.txt`](../../tests/training/CMakeLists.txt); the benchmarks are separately built and are not CTest tests.
- Security and operating boundaries: [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](./SECURITY.md), and [`OPERATIONS.md`](./OPERATIONS.md).

### Research findings and design implications

1. **Parameter-efficient fine-tuning:** [LoRA (Hu et al., ICLR 2022)](https://arxiv.org/abs/2106.09685) freezes base weights and learns low-rank updates; [AdaLoRA (Zhang et al., ICLR 2023)](https://arxiv.org/abs/2303.10512) allocates update rank by estimated importance. These motivate measuring quality, trainable parameters, memory, and latency together across rank/quantization settings; paper results do not establish ThemisDB workload gains.
2. **Benchmark validity:** [MLPerf Training](https://mlcommons.org/benchmarks/training/) evaluates time to a target quality and uses repeated runs. Adopt repeatable workload/configuration capture and quality-aware reporting; do not compare stub operations, preparation-only cases, or missing-model runs as equivalent training measurements.
3. **Dataset governance and risk:** [Datasheets for Datasets](https://cacm.acm.org/research/datasheets-for-datasets/) advocates documenting dataset origin, composition, collection, and intended use. [NIST AI RMF 1.0](https://doi.org/10.6028/NIST.AI.100-1) organizes risk work as Govern/Map/Measure/Manage; [OWASP ML02:2023](https://owasp.github.io/www-project-machine-learning-security-top-10/docs/ML02_2023-Data_Poisoning_Attack.html) identifies training-data poisoning and recommends validation, access controls, monitoring, and audit. These support explicit dataset lineage, authorization, holdout evaluation, and fail-closed artifact verification before release.

The measurements needed to close the gaps are not yet present as release evidence. Thresholds below are proposed acceptance gates for maintainers to confirm against representative workloads before implementation; they are not claims of current performance.

## v2.5.0 Plan — Compatibility-Preserving Hardening

**Intent:** close the security, integrity, evaluation, and release-evidence gaps without silently changing the existing training contract. Estimated effort is engineering person-weeks for one experienced contributor; external key-management, representative hardware, and dataset availability can extend elapsed time.

### Wave 2.5-1 — Data Authorization and Artifact Trust (P0)

- **Goal:** production DB training and serving handoff reject unauthorized reads and untrusted adapter artifacts.
- **Effort:** 3–5 person-weeks.
- **Tasks:**
  - [ ] Define a caller/policy context for DB-backed training and require an authorization decision before sample reads; preserve offline/test behavior only behind explicit non-production configuration. (Target: v2.5.0)
  - [ ] Specify and wire cryptographic signing and verification for completed adapter artifacts, binding the signature to the artifact digest, base-model identity, training configuration, and provenance manifest. (Target: v2.5.0)
  - [ ] Add focused tests for missing/denied policy, unauthorized collections, tampered artifacts, unknown signing keys, and failed verification before serving. (Target: v2.5.0)
- **Dependencies:** integration-owner identity/authorization contract, key custody/rotation service, and an agreed artifact envelope.
- **Risk / trade-off:** stricter fail-closed behavior may reject deployments that currently rely on an unconfigured optional checker; signing adds key availability and rotation operations.
- **Success metrics:** 100% of DB-backed reads have an explicit allow decision; 100% of tampered, unsigned, or untrusted-key artifacts are rejected before serving; no deploy/rollback state changes on failed verification.
- **Validation:** security review; unit/integration tests with allow/deny and key-rotation fixtures; tamper and interrupted-write fault injection; deployment handoff test proving router state is unchanged on rejection.

### Wave 2.5-2 — Checkpoint Format and Recovery (P1)

- **Goal:** checkpoint compatibility, corruption, and recovery outcomes are explicit and repeatable.
- **Effort:** 3–4 person-weeks.
- **Tasks:**
  - [ ] Add explicit checkpoint/manifest schema versioning and document the version-1 migration decision, supported inputs, and sunset; require maintainer approval before introducing any compatibility fallback. (Target: v2.5.0)
  - [ ] Verify checkpoint payload integrity and manifest-to-payload binding before resume, deployment, or rollback; preserve the last known-good checkpoint until replacement verification succeeds. (Target: v2.5.0)
  - [ ] Extend recovery tests for truncated payloads, stale/mismatched manifests, interrupted replacement, corrupt newest checkpoint, and all-candidates-invalid outcomes. (Target: v2.5.0)
- **Dependencies:** Wave 2.5-1 artifact identity/signature contract; checkpoint-format review.
- **Risk / trade-off:** rejecting an unsupported old format protects integrity but may require an explicit migration utility and operator downtime.
- **Success metrics:** every malformed or unsupported checkpoint fails with a stable diagnostic; 100% of fault-injection cases preserve the prior valid checkpoint or report no resumable state; accepted checkpoint versions are listed and tested.
- **Validation:** focused checkpoint-recovery and failsafe suites; deterministic restart/corruption tests; migration fixtures for each explicitly supported schema version.

### Wave 2.5-3 — Representative Performance Baselines (P1)

- **Goal:** make training performance comparisons reproducible and release-relevant.
- **Effort:** 4–6 person-weeks, excluding dedicated hardware provisioning.
- **Tasks:**
  - [ ] Define workload profiles for DB read, conversion/selection, CPU LoRA update, model-backed training, checkpoint I/O, and end-to-end pipeline; record hardware, build flags, model/data identifiers, and warm-up policy. (Target: v2.5.0)
  - [ ] Add repeated, machine-readable measurements with per-step latency distribution (p50/p95/p99), end-to-end time, samples/s, peak memory, and model quality; separately report unavailable GGUF/GPU cases as not run. (Target: v2.5.0)
  - [ ] Convert agreed same-host/profile regression limits into an automated release gate; retain stub benchmarks as microbenchmarks only. (Target: v2.5.0)
- **Dependencies:** stable representative test data/model artifacts and CI hardware for every claimed backend.
- **Risk / trade-off:** CI runtime and hardware variance can produce noisy gates; use repeated runs, same-host comparisons, and explicit baseline refresh review rather than weakening a failed gate.
- **Success metrics:** every claimed production path has a captured baseline and workload metadata; p95/p99 are reported for per-step samples; a >10% same-profile throughput regression is detected and blocks the designated release gate.
- **Validation:** run benchmarks in release profile on CPU and available supported accelerators; compare repeated-run spread and stored baseline; verify gate failure with an intentionally perturbed baseline and confirm stub cases cannot satisfy production gates.

### Wave 2.5-4 — Data Quality and Multi-Task Evidence (P1)

- **Goal:** replace proxy-only confidence in selection and multi-task training with held-out task-quality evidence.
- **Effort:** 4–6 person-weeks, plus domain-owner evaluation time.
- **Tasks:**
  - [ ] Record dataset origin, permitted use, task/domain, composition, selection configuration, and evaluation split in a reviewable dataset/evaluation manifest; avoid raw sensitive training content in logs. (Target: v2.5.0)
  - [ ] Compare heuristic selection against random and full-data baselines on at least three representative tasks, with held-out data and three seeds; report per-task quality and sample/compute cost. (Target: v2.5.0)
  - [ ] Re-run the existing multi-task LoRA acceptance gates on representative task datasets; retain current synthetic tests as unit-level checks, not quality evidence. (Target: v2.5.0)
- **Dependencies:** approved datasets/licenses, task owners, and repeatable evaluation metrics.
- **Risk / trade-off:** quality changes may favor common domains and regress minority tasks; per-task floors and dataset review are required alongside average scores.
- **Success metrics:** selection reduces training examples by at least 20% while keeping every task's held-out primary metric within 2 percentage points of the full-data baseline across three seeds; publish per-task results and dataset metadata.
- **Validation:** reproducible selection ablation; leakage checks between training and holdout sets; multi-task convergence and per-task regression tests; reviewer sign-off on data-use records.

### Wave 2.5-5 — Release Readiness and Operations (P0 exit)

- **Goal:** close the v2.5.0 evidence package and ensure operators can diagnose, stop, and recover training safely.
- **Effort:** 2–3 person-weeks after Waves 2.5-1 through 2.5-4.
- **Tasks:**
  - [ ] Integrate focused correctness/security tests and approved benchmark gates into the release validation workflow; archive machine/configuration/baseline outputs. (Target: v2.5.0)
  - [ ] Verify cancellation, timeout, resource-limit, rollback, and recovery behavior in a sustained pipeline test; update runbook only for verified behavior. (Target: v2.5.0)
  - [ ] Close the source-vs-secondary-doc selection-flow discrepancy identified above and refresh the developer wiki before marking the evidence bundle complete. (Target: v2.5.0)
- **Dependencies:** all previous v2.5.0 waves and human review of security, dataset-use, and benchmark evidence.
- **Risk / trade-off:** integration-level dependencies are owned outside this module; unresolved key, data, or hardware availability blocks readiness rather than being replaced with simulated evidence.
- **Success metrics:** all P0 security/recovery checks pass; release benchmark gate is green on named profiles; every unresolved risk has an owner and explicit non-claim; zero unexplained failures in the agreed soak duration.
- **Validation:** registered training tests plus security, benchmark, fault-injection, and soak evidence; operator walkthrough using [`RUNBOOK.md`](./RUNBOOK.md); human release sign-off.

## v3.0.0 Strategic Plan — Versioned Training Service

**Intent:** use the major-version boundary for an explicit, policy-aware job contract and scalable execution; retain only capabilities proven valuable by v2.5.0 evidence. Proposed effort ranges are engineering person-weeks and exclude hardware procurement and compliance review.

### Wave 3-1 — Job, Policy, and Artifact Contract (P0)

- **Goal:** define a versioned training-job API with explicit tenant/data policy, resource budgets, cancellation, status, and immutable artifact identity.
- **Effort:** 5–8 person-weeks.
- **Tasks:**
  - [ ] Design the v3 job lifecycle and versioned interfaces for submit, cancel, progress, result, provenance, authorization, quotas, and idempotent retry; publish v2-to-v3 migration notes. (Target: v3.0.0)
  - [ ] Separate policy decisions from trainer internals and make deny-by-default authorization and signed artifact verification mandatory for production jobs. (Target: v3.0.0)
  - [ ] Set bounded limits for queued jobs, input bytes/samples, memory, wall-clock time, checkpoint storage, and concurrent jobs, with stable failure codes. (Target: v3.0.0)
- **Dependencies:** v2.5.0 security/artifact evidence, API owners, and operator/tenant requirements.
- **Risk / trade-off:** stronger API guarantees create migration and integration costs; freeze the contract only after representative deployment review.
- **Success metrics:** all v3 production jobs carry policy, resource, and artifact-version metadata; denied jobs perform zero training reads; cancellation and idempotent retry pass lifecycle contract tests with no orphaned active jobs.
- **Validation:** public API compatibility review; contract/property tests for lifecycle transitions, authorization, quota boundaries, cancellation races, and retry idempotency; migration tests against supported v2 artifacts.

### Wave 3-2 — Bounded Streaming and Distributed Execution (P1)

- **Goal:** scale beyond single-process buffered runs without making multi-GPU or multi-node support a configuration-only claim.
- **Effort:** 8–12 person-weeks for a first supported distributed profile.
- **Tasks:**
  - [ ] Stream bounded DB batches through selection/training with backpressure and resumable cursor/checkpoint semantics; peak input memory must be independent of total dataset size. (Target: v3.0.0)
  - [ ] Implement and benchmark one explicitly supported multi-GPU profile, then add multi-node execution only after failure, synchronization, and recovery semantics are defined. (Target: v3.0.0)
  - [ ] Add worker loss, slow-worker, communication failure, and checkpoint-resume tests; document hardware/backend support matrix. (Target: v3.0.0)
- **Dependencies:** v3 job/resource contract, GPU/LLM backend support, and representative hardware.
- **Risk / trade-off:** synchronization and retry overhead may erase throughput gains; unsupported backends must fail clearly rather than silently fall back.
- **Success metrics:** streaming 1,000,000 samples stays within configured batch/prefetch memory bounds; a supported two-GPU workload reaches at least 1.6× single-GPU throughput with equivalent quality; worker-loss recovery resumes from a verified checkpoint without duplicate committed progress.
- **Validation:** backend-specific release benchmarks, peak-memory profiling, scaling-efficiency reports, deterministic CPU/GPU parity tolerances, and distributed fault injection.

### Wave 3-3 — Quality-Calibrated Selection and Adaptation (P1)

- **Goal:** make selection/adaptation strategy evidence-driven and robust across tasks, domains, and changing data.
- **Effort:** 6–10 person-weeks, excluding domain dataset creation.
- **Tasks:**
  - [ ] Compare current heuristics with embedding/learned selection and random/full-data baselines; adopt a more complex method only when quality-per-cost improves on held-out workloads. (Target: v3.0.0)
  - [ ] Add task/domain-level quality floors, drift-triggered evaluation, calibration, and explicit human approval for policy-sensitive data or model changes. (Target: v3.0.0)
  - [ ] Extend multi-task evaluation to disjoint, imbalanced, and shifting task distributions with ablations and confidence intervals. (Target: v3.0.0)
- **Dependencies:** v2.5.0 dataset manifests and quality baselines; representative task owners and evaluation data.
- **Risk / trade-off:** learned selection adds inference cost and can encode dataset bias; retain simpler selection when quality-per-cost does not justify added complexity.
- **Success metrics:** quality-per-training-cost improves at least 10% over the best v2.5.0 baseline across three task domains and three seeds, with no task's primary metric regressing more than 2 percentage points; report confidence intervals and data provenance.
- **Validation:** held-out, leakage-checked ablations; domain-stratified regression tests; drift-triggered retraining simulation; reproducible evaluation bundle.

### Wave 3-4 — Privacy-Preserving Cross-Site Training (Conditional P2)

- **Goal:** support cross-site adaptation only when data residency, privacy threat model, and utility requirements justify it.
- **Effort:** 8–12 person-weeks after privacy/security design approval; substantial external integration work may be additional.
- **Tasks:**
  - [ ] Complete a threat model and data-residency review before choosing federated aggregation, secure aggregation, or differential privacy; federation alone is not a privacy guarantee. (Target: v3.0.0)
  - [ ] Implement one approved protocol with authenticated participants, replay protection, bounded update validation, and auditable membership/round metadata; raw records must remain local. (Target: v3.0.0)
  - [ ] Define the privacy budget and utility floor with the accountable privacy owner before implementation; if no approved budget/protocol exists, defer this wave rather than claiming privacy-preserving training. (Target: v3.0.0)
- **Dependencies:** approved privacy/legal threat model, key management, network transport, and participating test sites.
- **Risk / trade-off:** privacy noise and communication can reduce quality or increase cost; cross-site support expands the trust boundary and attack surface.
- **Success metrics:** zero raw training records cross the site boundary; all rounds authenticate and audit; the approved privacy budget is met where differential privacy is selected, while every site's agreed quality floor is maintained.
- **Validation:** red-team protocol review; malicious/failed participant and replay tests; privacy accountant review if applicable; multi-site utility, communication, and failure-recovery benchmarks.

## Open Assumptions and Knowledge Gaps

- The v2.5.0 selection-quality gate assumes three permitted, representative datasets with stable held-out evaluations are available. If not, maintain the quality claim as unverified and block promotion rather than substituting synthetic data.
- Authorization, signing keys, benchmark hardware, and production dataset ownership are integration-level dependencies; this module does not independently supply them today.
- The current source implements selected-ID training, while the cited contract/architecture/future-enhancement text still describes the former collection-wide behavior. The source and a focused DB test take precedence for planning; reconcile the documentation before release.
- `ai_context/developer_llm_wiki/WIKI_STATUS.json` was generated 2026-11-02 and reports a large source delta. It is stale for this assessment; use module sources and refresh the wiki before implementation decisions.
- Existing multi-task thresholds and benchmark limits are not accepted as release evidence until reproduced on the proposed representative datasets/profiles.

## Breaking Changes

No breaking training contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.