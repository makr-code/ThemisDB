<!-- Status: draft | validated: 2026-10-05 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · TENSOR_ML_TRAINING_BRIDGE.md · TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md · FUTURE_TENSOR_ROPE.md · PERFORMANCE_EXPECTATIONS.md -->

# ThemisDB ↔ Human Brain Memory Systems  
## A Comparative Architecture Paper for Short-Term/Long-Term Memory Bridging

**Author:** ThemisDB Contributors  
**Created:** 2026-10-05  
**Last Updated:** 2026-10-05  
**Status:** draft

---

## Abstract

This paper provides a scientific architecture-level comparison between ThemisDB modules and human brain systems involved in memory formation, consolidation, retrieval, adaptation, and control.  
The goal is not metaphorical storytelling but a **functional systems mapping**:

1. Which ThemisDB modules already implement analogs of biological memory stages.
2. Where functional gaps remain relative to brain subsystems.
3. Which gaps are likely high-leverage for the next implementation wave.

Primary architecture references:
- [TENSOR_ML_TRAINING_BRIDGE.md](./TENSOR_ML_TRAINING_BRIDGE.md)
- [TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md](./TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md)
- [FUTURE_TENSOR_ROPE.md](./FUTURE_TENSOR_ROPE.md)

---

## 1. Scientific Framing and Method

## 1.1 Scope

In scope:
- memory-relevant ThemisDB module behavior,
- short-term to long-term transition analogs,
- control, routing, and safety layers that shape memory behavior.

Out of scope:
- one-to-one neurobiological identity claims,
- clinical neuroscience interpretations,
- claims of consciousness or phenomenology.

## 1.2 Mapping rule

Each mapping in this paper uses:
- **Functional role equivalence** (what the subsystem does),
- **Temporal role** (online/short-term vs offline/long-term),
- **Error-handling behavior** (fail-open/fail-closed analog).

---

## 2. Canonical Memory Pipeline (Brain vs ThemisDB)

## 2.1 Brain reference pipeline (simplified)

1. Sensory encoding and salience filtering.
2. Working-memory maintenance and attentional gating.
3. Hippocampal indexing and rapid episodic binding.
4. Replay and systems consolidation into neocortical long-term representations.
5. Retrieval with executive control and conflict inhibition.
6. Adaptive update under reward/error signals.

## 2.2 ThemisDB pipeline analog

1. Data ingestion and representation shaping  
   - [tensor_ingestion_bridge.cpp](./tensor_ingestion_bridge.cpp)
2. Short-horizon candidate activation  
   - [hnsw_tt_bridge.cpp](./hnsw_tt_bridge.cpp)
3. Structural memory encoding/compression  
   - [adalora_tt_bridge.cpp](../training/adalora_tt_bridge.cpp)
4. Persistent artifact storage + runtime mapping  
   - [ggml_tensor_bridge.cpp](../storage/ggml_tensor_bridge.cpp)
5. Controlled retrieval and generation  
   - [llama_cpp_plugin.cpp](../llama_cpp/llama_cpp_plugin.cpp)
6. Deployment-time adaptation and rollback  
   - [incremental_lora_trainer.cpp](../training/incremental_lora_trainer.cpp)

---

## 3. Module-to-Brain Comparative Matrix

| ThemisDB module / surface | Brain system analog | Functional equivalence | Maturity |
|---|---|---|---|
| `tensor::TensorIngestionBridge` | Entorhinal gateway + early hippocampal encoding | Normalizes incoming signal, applies transform profile, prepares compressed memory trace | implemented |
| `tensor::HnswTTBridge` | Hippocampal pattern completion (fast recall candidates) | Rapid sparse-to-dense retrieval and candidate reranking | implemented |
| `training::AdaLoraTTBridge` | Systems consolidation (hippocampus→neocortex transfer analog) | Converts/adapts representational format and rank budget for durable downstream use | implemented/partial hardening |
| `training::IncrementalLoRATrainer` + `ILLMRouter` | Executive gating + memory update policy | Controls promotion, rollback, and active influence of adapted memory weights | implemented |
| `storage::GgmlTensorBridge` | White-matter pathway analog (transfer/format bridge) | Bridges persistent representations into runtime-executable substrate | capability-gated |
| `llama_cpp::LlamaCppPlugin` (`generateRAG`, `embed`) | Prefrontal-guided retrieval + verbalization output | Integrates retrieved memory traces into final generated behavior | conditional quality path |
| `tensor::TensorFingerprintGraph` | Associative cortex / semantic neighborhood structure | Maintains similarity neighborhoods and relatedness graph for memory association | implemented |
| `retrieval` + `rag` module surfaces | Cortico-hippocampal loop | Couples query intent to memory recall and evidence-conditioned output | implemented |
| `observability` + `governance` + `security` | Error monitoring + homeostatic/immune control analog | Detects, constrains, and audits unsafe state transitions | implemented |

---

## 4. Short-Term vs Long-Term Memory Bridge

## 4.1 Short-term analog in ThemisDB

Short-term memory analog corresponds to:
- online retrieval activation,
- query-context-dependent candidate focus,
- non-persistent or weakly persistent intermediate state.

Primary surfaces:
- [hnsw_tt_bridge.cpp](./hnsw_tt_bridge.cpp)
- [llama_cpp_plugin.cpp](../llama_cpp/llama_cpp_plugin.cpp)
- [retrieval/](../retrieval)

## 4.2 Long-term analog in ThemisDB

Long-term memory analog corresponds to:
- persistent tensor/adaptor artifacts,
- versioned metadata and profile compatibility,
- replayable and auditable storage state.

Primary surfaces:
- [tensor_ingestion_bridge.cpp](./tensor_ingestion_bridge.cpp)
- [adalora_tt_bridge.cpp](../training/adalora_tt_bridge.cpp)
- [ggml_tensor_bridge.cpp](../storage/ggml_tensor_bridge.cpp)

## 4.3 Consolidation bridge (core result)

The current consolidation bridge in ThemisDB is a **distributed architecture** across `tensor`, `training`, `storage`, and `llama_cpp`, not a single orchestrator class.  
This matches, at coarse systems level, biological distributed consolidation where no single neuron or nucleus "is memory".

---

## 5. Missing Brain Functions (Gap Register)

The following brain-relevant functions are not yet fully present as first-class ThemisDB mechanisms.

| Missing or weakly represented brain function | ThemisDB gap | Expected engineering impact |
|---|---|---|
| Sleep-like offline replay consolidation | No explicit staged replay scheduler that reprocesses episodic traces into stable semantic tensors under controlled windows | Better long-horizon stability and reduced drift after incremental updates |
| Multi-timescale forgetting/homeostasis | No native synaptic-pruning analog with principled utility/age/entropy decay across tensor artifacts | Lower stale-memory noise, bounded storage growth, more robust retrieval precision |
| Neuromodulated salience weighting (dopamine/norepinephrine analog) | Limited unified reward/salience signal feeding ingestion + consolidation + routing | Better prioritization of high-value memory updates |
| Thalamic global routing hub analog | Routing exists but lacks a unified, explicit cross-modal arbitration layer for all retrieval channels | Reduced routing conflicts and more consistent end-to-end latency/quality tradeoffs |
| Cerebellar fast error-correction loop analog | No dedicated micro-adaptation loop for low-latency correction of repeated local generation errors | Faster convergence on repetitive failure classes |
| Rich episodic context binding | Profile metadata exists, but richer temporal/causal episode graph binding is incomplete | Stronger context-aware recall and explainability |
| Emotion-like valence tagging | No standardized affect/importance tagging channel that influences persistence and retrieval priority | Better user-aligned long-term memory selection in assistant scenarios |

---

## 6. Mapping Confidence Rubric

To keep the brain↔system comparison scientifically bounded, each mapping can be scored along four axes:

- **Role fidelity (0-3):** how close the functional purpose matches.
- **Temporal fidelity (0-3):** whether online/offline dynamics are comparable.
- **Control fidelity (0-2):** whether gating/inhibition/update control is analogous.
- **Evidence fidelity (0-2):** whether measurable system evidence exists (tests/benchmarks/contracts).

Maximum score: 10.

| Mapping pair | Score | Rationale |
|---|---:|---|
| `TensorIngestionBridge` ↔ entorhinal/early hippocampal encoding | 8/10 | strong role+temporal match; evidence path exists, but biological feature richness is lower |
| `HnswTTBridge` ↔ hippocampal rapid recall/pattern completion | 8/10 | strong fast-recall analogy and measurable retrieval behavior |
| `AdaLoraTTBridge` ↔ systems consolidation | 7/10 | conversion/consolidation role is good; replay/sleep-phase semantics are incomplete |
| `IncrementalLoRATrainer` + `ILLMRouter` ↔ executive gating | 7/10 | promotion/rollback and policy gating present; global arbitration is still fragmented |
| `GgmlTensorBridge` ↔ transfer pathways | 6/10 | bridge role exists but remains capability-gated in parts |
| `LlamaCppPlugin` retrieval integration ↔ prefrontal-controlled retrieval output | 6/10 | control/output role exists; quality-path conditionality limits fidelity |

---

## 7. Evidence Basis in Neuroscience and ML Systems

The comparison is grounded in established principles, not strict identity claims.

### 7.1 Brain memory principles used

1. **Complementary Learning Systems (CLS):** fast hippocampal encoding + slower neocortical consolidation.  
2. **Replay-driven consolidation:** offline replay strengthens durable representations.  
3. **Working-memory executive control:** prefrontal gating selects and suppresses memory use.  
4. **Salience/reward modulation:** learning and consolidation are priority-weighted, not uniform.

### 7.2 Translation to ThemisDB design logic

- CLS motivates separation of fast retrieval activation and slower durable tensor/adapter consolidation.
- Replay principle motivates introducing scheduled offline re-index/re-score/retrain loops.
- Executive control motivates explicit routing policy and rollback governance.
- Salience principle motivates a unified utility signal for memory persistence decisions.

---

## 8. Gap-to-Module Implementation Map (Next-Auftrag Ready)

| Gap theme | Primary target modules | Secondary target modules | Suggested first deliverable |
|---|---|---|---|
| Offline replay consolidation | [training/](../training), [tensor/](./) | [scheduler/](../scheduler), [observability/](../observability) | replay job contract + deterministic replay CTest slice |
| Multi-timescale forgetting/homeostasis | [tensor/](./), [storage/](../storage) | [retrieval/](../retrieval), [performance/](../performance) | decay/prune policy schema + bounded-growth benchmark |
| Salience-modulated persistence | [training/](../training), [rag/](../rag) | [governance/](../governance), [security/](../security) | cross-module salience score contract |
| Global routing arbitration | [llm/](../llm), [retrieval/](../retrieval) | [query/](../query), [api/](../api) | unified arbitration interface + policy tests |
| Fast local correction loop | [llama_cpp/](../llama_cpp), [training/](../training) | [evaluation/](../evaluation) | micro-correction pipeline for repeated failure motifs |
| Episodic context binding | [tensor/](./), [metadata/](../metadata) | [distributed_knowledge/](../distributed_knowledge) | temporal/causal context edge schema |

---

## 9. Testable Hypotheses for the Next Auftrag

## H1 — Replay consolidation improves stability

If an offline replay subsystem is introduced, then:
- inter-run retrieval variance should decrease,
- rollback frequency after deployments should decrease,
- long-window quality decay should flatten.

Evidence surfaces:
- [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md)
- [tests/tensor/CMakeLists.txt](../../tests/tensor/CMakeLists.txt)

## H2 — Principled forgetting improves signal quality

If utility-based decay/pruning is introduced, then:
- stale-neighbor contamination in similarity search should reduce,
- p95 latency should remain bounded under growth,
- quality should improve on recency-sensitive tasks.

## H3 — Unified salience routing improves adaptation efficiency

If a cross-module salience signal is enforced, then:
- fewer low-value updates should reach long-term persistence,
- adaptation budget should shift toward high-impact cases,
- failure recovery should require fewer corrective cycles.

## H4 — Explicit routing arbitration improves consistency

If global cross-channel arbitration is introduced, then:
- output variance between equivalent retrieval channels should decrease,
- latency outliers caused by route thrashing should decrease,
- policy explanations should become reproducible in logs.

---

## 10. Experimental Design Blueprint

For each hypothesis, the minimum protocol should include:

1. **A/B/Ablation structure**
   - baseline (`no feature`),
   - treatment (`feature on`),
   - stress/control variants (scale, tenant split, load).

2. **Fixed reproducibility controls**
   - fixed seed,
   - fixed dataset snapshot hash,
   - fixed build profile,
   - captured command manifest.

3. **Metric families**
   - quality: recall@k, nDCG@k, answer-grounding correctness,
   - operations: p50/p95/p99 latency, rollback count, failure taxonomy,
   - stability: inter-run variance, drift slope over time.

4. **Go/No-Go gate style**
   - no single-metric promotion,
   - fail-closed when quality gain depends on unstable operational behavior.

---

## 11. Architectural Guidance from the Comparison

1. Treat "memory" as a distributed contract, not a single module.
2. Keep fail-closed profile/version behavior as non-negotiable control law.
3. Introduce missing functions as explicit module contracts, not implicit heuristics.
4. Gate every new brain-inspired feature through measurable CTest/benchmark evidence.

---

## 12. Risks of Over-Analogy

1. **Category error risk**: biological plausibility does not imply system utility.
2. **Complexity risk**: adding many analog mechanisms without metrics can reduce reliability.
3. **Validation risk**: unverifiable metaphors must never bypass hard performance gates.

Mitigation:
- every proposed brain-inspired extension requires explicit acceptance metrics,
- no promotion without reproducible benchmark + test evidence packet.

---

## 13. Reference Pointers (Evidence Context)

Indicative foundational literature used for the conceptual mapping:

1. McClelland, McNaughton, O'Reilly (1995): complementary learning systems.
2. Buzsaki (1989; 2015): hippocampal replay and systems memory consolidation.
3. Frankland & Bontempi (2005): systems consolidation and remote memory.
4. Baddeley & Hitch (1974), Baddeley (2000): working-memory model and executive control.
5. Doya (2000): neuromodulation and meta-learning control roles.

These references motivate architecture hypotheses; they do not imply direct biological equivalence.

---

## 14. Conclusions

ThemisDB already implements core structural analogs of short-term activation, long-term storage, and consolidation bridging through `tensor`, `training`, `storage`, and `llama_cpp` pathways.  
The strongest missing areas, compared to full brain memory systems, are:

1. explicit offline replay consolidation,
2. principled forgetting/homeostasis,
3. unified salience-driven routing and update policy,
4. explicit cross-channel arbitration for retrieval routing.

These gaps form a concrete, evidence-testable backlog for the next Auftrag.

---

## 15. Computational Primitive Correspondence

This section maps computational motifs (rather than anatomy labels) across brain memory theory and ThemisDB implementation surfaces.

| Computational primitive | Brain-side interpretation | ThemisDB implementation analog | Current status |
|---|---|---|---|
| Sparse cue completion | partial cue activates full memory trace | candidate-first recall in [hnsw_tt_bridge.cpp](./hnsw_tt_bridge.cpp) + rerank path | implemented |
| Compression under fidelity constraints | consolidation preserves utility while reducing cost | TT decomposition and rank-bounded adaptation in [tensor_ingestion_bridge.cpp](./tensor_ingestion_bridge.cpp) and [adalora_tt_bridge.cpp](../training/adalora_tt_bridge.cpp) | implemented/ongoing hardening |
| Context-gated retrieval | active context controls what is recalled | route/profile-aware retrieval in [retrieval/](../retrieval), [rag/](../rag), and [llama_cpp_plugin.cpp](../llama_cpp/llama_cpp_plugin.cpp) | partial |
| Replay-mediated stabilization | repeated offline reactivation strengthens long-term trace | currently missing as first-class scheduler-controlled replay contract | gap |
| Competitive arbitration | concurrent recall channels are centrally adjudicated | distributed policies in [llm/](../llm), [query/](../query), [api/](../api) without single arbitration kernel | gap/partial |
| Homeostatic forgetting | stale/low-utility traces decay | no global multi-timescale decay policy across tensor artifacts | gap |

---

## 16. Falsifiability and Negative-Result Criteria

A brain-inspired enhancement must be considered **rejected** for production planning when any of the following hold:

1. It improves one quality metric but violates operational envelopes (latency/reliability/drift) in release profile.
2. Gains disappear under seed/control reruns or cannot be reproduced with fixed manifests.
3. Benefit requires non-auditable heuristics or hidden fallback behavior.
4. Improvement is limited to synthetic narrow tests and does not transfer to mixed workload suites.

Formal rejection rule:
- If 2 or more release gates regress beyond tolerance while no statistically stable quality gain is observed, the feature remains non-promotable.

---

## 17. Prioritized Gap Backlog (IRL Scoring)

IRL = Implementation Readiness Level in this repository context (1=concept only, 5=promotion-ready with evidence).

| Gap theme | IRL now | IRL target | Priority | Blocking factors |
|---|---:|---:|---|---|
| Offline replay consolidation | 1 | 4 | P0 | missing replay contract, scheduler integration, replay evidence suite |
| Multi-timescale forgetting/homeostasis | 1 | 3 | P0 | no canonical decay semantics, no bounded-growth gates |
| Unified salience routing | 2 | 4 | P1 | fragmented utility signals across ingestion/training/retrieval |
| Global routing arbitration | 2 | 4 | P1 | no single arbitration interface with deterministic conflict policy |
| Episodic context binding | 2 | 3 | P1 | incomplete temporal/causal metadata edges |
| Fast local correction loop | 1 | 3 | P2 | no micro-correction control plane for repeated local failures |
| Valence/importance tagging | 1 | 2 | P3 | absent normalized valence taxonomy and persistence policy |

Interpretation:
- P0 themes are architecture-critical for long-horizon stability and memory quality.
- P1 themes are consistency and control upgrades with high operational leverage.
- P2/P3 are optimization-layer features after P0/P1 contracts are in place.

---

## 18. Next-Auftrag Workpackage Blueprint

Recommended first executable package set (aligned with the bridge papers):

### WP-BRAIN-01: Replay Consolidation Contract

Goal:
- add explicit offline replay lifecycle for tensor/adaptor traces.

Primary targets:
- [training/](../training)
- [tensor/](./)
- [scheduler/](../scheduler)

Acceptance:
- deterministic replay run with fixed seed/config manifest,
- replay delta report (before/after quality + stability metrics),
- fail-closed behavior for invalid replay context.

### WP-BRAIN-02: Forgetting/Homeostasis Policy

Goal:
- introduce utility/age-aware decay policy with bounded growth guarantees.

Primary targets:
- [tensor/](./)
- [storage/](../storage)
- [performance/](../performance)

Acceptance:
- bounded-growth benchmark profile,
- no quality collapse in recency-sensitive retrieval suites,
- explicit policy metadata in artifact logs.

### WP-BRAIN-03: Routing Arbitration Kernel

Goal:
- unify conflict resolution across retrieval channels and route candidates.

Primary targets:
- [llm/](../llm)
- [retrieval/](../retrieval)
- [query/](../query)

Acceptance:
- deterministic arbitration under equivalent inputs,
- reduced route-thrashing outliers,
- explainable policy decision traces.

---

## 19. CTest and Benchmark Gate Blueprint

To operationalize this paper, the next Auftrag should register explicit gate families:

1. **BRAIN-REPLAY** (stability and replay transfer)
   - deterministic replay tests
   - long-window drift checks
2. **BRAIN-HOMEO** (decay/homeostasis behavior)
   - bounded growth tests
   - stale-neighbor contamination regression checks
3. **BRAIN-ARBITER** (route arbitration)
   - deterministic tie-break behavior tests
   - route-thrashing resilience tests

Benchmark expectations should be linked into:
- [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md)

CTest registration anchor:
- [tests/tensor/CMakeLists.txt](../../tests/tensor/CMakeLists.txt)

---

## 20. Threats to Validity

1. **Analogy transfer bias**  
   A seemingly plausible brain analogy may not produce engineering benefit.

2. **Benchmark representativeness bias**  
   Gains on curated workloads may not generalize to production heterogeneity.

3. **Control confounds**  
   Seed/config drift can mimic or hide true memory-system effects.

4. **Module boundary confounds**  
   Improvements attributed to one module may be emergent effects of cross-module interactions.

Mitigation:
- enforce fixed-manifest reruns,
- require cross-suite consistency before promotion,
- keep claims tied to measured deltas and failure envelopes.

---

## 21. Decision Framework for Promotion

A brain-inspired feature progresses from concept to promotion only when all are true:

1. Contract completeness:
   - explicit interface, metadata, and failure semantics are documented.
2. Evidence completeness:
   - CTest + benchmark gates pass with reproducible manifests.
3. Operational completeness:
   - rollback and degraded-mode behavior are explicit and exercised.
4. Governance completeness:
   - linked docs ([ROADMAP.md](./ROADMAP.md), [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md), [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md)) are synchronized.

This ensures the comparative paper drives implementation rigor, not only conceptual direction.

---

## 22. Statistical Evaluation Protocol

To avoid false-positive conclusions from noisy benchmark behavior, each hypothesis evaluation should follow a minimum statistical protocol.

### 22.1 Sampling requirements

1. Minimum runs per condition:
   - latency/throughput metrics: >= 20 independent runs
   - quality metrics (Recall/nDCG): >= 10 full-eval runs
2. Randomization:
   - randomized query order per run
   - fixed seed set published in manifest
3. Warmup:
   - mandatory warmup phase excluded from scored metrics

### 22.2 Reported statistics

For each metric, report:
- mean,
- median,
- p95/p99 where relevant,
- standard deviation,
- 95% confidence interval,
- effect size versus baseline.

### 22.3 Promotion threshold style

Feature-level promotion should require:
- statistically stable improvement in at least one primary quality metric,
- no statistically meaningful regression in mandatory operational gates,
- reproducibility across at least two workload families (e.g., tenant split + mixed-domain set).

---

## 23. Reproducibility Manifest Contract

Every evidence packet should include a machine-readable manifest (for example JSON/YAML) with at least:

1. build and toolchain identity
   - compiler/toolchain versions
   - CMake preset
   - target binaries and commit SHA
2. dataset identity
   - dataset name
   - snapshot hash
   - split definition hash
3. experiment identity
   - hypothesis ID (H1..H4 or successor IDs)
   - feature flags and profile mode (`none|fixed|relational|learned`)
   - seed list
4. execution identity
   - exact benchmark/ctest commands
   - timestamp window
   - hardware profile summary
5. results identity
   - metric table
   - pass/fail per gate
   - decision recommendation (`go|hold|no-go`)

This contract should be consistent with the artifact requirements documented in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md).

---

## 24. Traceability Matrix (Hypothesis → Module → Gate)

| Hypothesis | Primary modules | Primary gates | Required evidence |
|---|---|---|---|
| H1 Replay consolidation | [training/](../training), [tensor/](./), [scheduler/](../scheduler) | BRAIN-REPLAY, TEN-ROPE-G2/G5 style failure semantics | replay drift report + deterministic replay tests |
| H2 Forgetting/homeostasis | [tensor/](./), [storage/](../storage), [performance/](../performance) | BRAIN-HOMEO + bounded growth envelope | growth/quality tradeoff benchmarks + contamination regressions |
| H3 Salience routing | [training/](../training), [rag/](../rag), [retrieval/](../retrieval) | BRAIN-ARBITER partial + quality/rollback deltas | salience-on/off ablation + rollback reduction evidence |
| H4 Routing arbitration | [llm/](../llm), [retrieval/](../retrieval), [query/](../query), [api/](../api) | BRAIN-ARBITER determinism and thrash checks | deterministic policy logs + route variance report |

---

## 25. Transition Plan: Research Concept to Production Module

Each gap feature should pass four maturity checkpoints:

1. **Concept checkpoint**
   - threat model and expected benefit documented.
2. **Prototype checkpoint**
   - isolated implementation with synthetic tests and explicit guardrails.
3. **Pre-production checkpoint**
   - integrated module path, full failure semantics, reproducibility manifests.
4. **Production checkpoint**
   - release-profile benchmark + CTest gates pass, rollback validated, docs synchronized.

The transition plan should be synchronized with:
- [ROADMAP.md](./ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md)
- [TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md](./TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md)

---

## 26. Immediate Implementation Priorities (Action-Oriented Summary)

1. Establish replay contract and deterministic replay CTests (highest leverage for long-term stability).
2. Define homeostasis policy schema with bounded-growth gates.
3. Introduce routing arbitration kernel with deterministic tie-break behavior.
4. Wire salience signal end-to-end (ingestion/training/retrieval) after arbitration baseline exists.
5. Only then expand lower-priority valence/micro-correction features.

This ordering minimizes architectural churn and maximizes evidence quality for the next Auftrag.

---

## 27. Bridge Lifecycle State Machine

To make the memory bridge operationally auditable, the feature lifecycle should be modeled as an explicit state machine.

### 27.1 States

1. **S0 / Concept**
   - hypothesis exists, no runtime path.
2. **S1 / Prototype-Isolated**
   - local implementation, synthetic/focused tests only.
3. **S2 / Integrated-Guarded**
   - integrated path behind explicit feature/profile gates.
4. **S3 / Evidence-Qualified**
   - benchmark + CTest evidence packet passes pre-production criteria.
5. **S4 / Production-Eligible**
   - release-profile evidence + rollback proof + governance sync complete.
6. **S5 / Production-Active**
   - enabled in target release profile with monitoring.
7. **S6 / Rollback-Hold**
   - degraded/disabled due to gate regressions or incident trigger.

### 27.2 Allowed transitions

- S0 → S1: design approval + minimal contract draft.
- S1 → S2: interface hardening + fail-closed behavior implemented.
- S2 → S3: reproducible evidence packet generated.
- S3 → S4: promotion review accepts statistical/operational profile.
- S4 → S5: release manager enables in target edition profile.
- S5 → S6: incident, regression, or policy trigger.
- S6 → S3: remediation evidence confirms recovery readiness.

No direct S1 → S4 or S2 → S5 transition should be allowed.

---

## 28. Failure Taxonomy for Brain-Inspired Features

The following failure classes should be tracked explicitly in diagnostics and decision notes:

| Class | Description | Typical trigger | Required response |
|---|---|---|---|
| BF-01 Profile-Contract Violation | feature/profile context does not satisfy contract | profile mismatch, missing context | fail-closed reject + explicit operator diagnostic |
| BF-02 Replay Divergence | replay path introduces unstable drift | seed/config mismatch, nondeterministic replay | hold promotion, investigate reproducibility chain |
| BF-03 Arbitration Instability | route conflict policy oscillates | equivalent scores without deterministic tie-breaks | enforce deterministic arbitration and rerun stress suite |
| BF-04 Homeostasis Over-pruning | decay policy removes useful memory traces | aggressive decay threshold | rollback policy level and re-evaluate quality gates |
| BF-05 Hidden Fallback Leakage | feature "passes" via silent downgrade path | missing capability path falls back silently | classify as hard failure; fix fallback semantics |

These classes should be mirrored in module-level diagnostics where feasible.

---

## 29. Edition-Aware Rollout Policy

Rollout should follow the canonical branch/edition governance model:

- default implementation and hardening on `develop`,
- controlled propagation to edition lanes (`minimal`, `community`, `enterprise`, `hyperscaler`, `military`) only after S4 readiness.

Reference governance:
- [../../BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md)
- [../../RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md)

### 29.1 Recommended rollout waves

1. **Wave A (develop):** S2/S3 validation, evidence accumulation.
2. **Wave B (minimal/community):** conservative enablement under strict gates.
3. **Wave C (enterprise/hyperscaler/military):** profile-tuned enablement with stronger operational guardrails.

### 29.2 Rollout invariants

1. No edition may enable a feature class if it is below S4.
2. Any edition-specific divergence must be documented with explicit rationale.
3. Production activation without rollback proof is prohibited.

---

## 30. Observability and Decision Logging Schema

For each brain-inspired feature run, logs should contain:

1. **Feature identity**
   - feature ID (`WP-BRAIN-*`)
   - hypothesis ID (`H*`)
2. **Policy identity**
   - profile mode
   - arbitration/decay policy version
3. **Execution identity**
   - run ID
   - seed set hash
   - dataset snapshot hash
4. **Decision identity**
   - gate outcomes
   - go/hold/no-go decision
   - human approver reference

This schema enables post-incident reconstruction and comparable release decisions.

---

## 31. Research-to-Engineering Artifact Set

The following artifact set should be considered mandatory for each P0/P1 feature:

1. design note with failure semantics,
2. reproducibility manifest,
3. benchmark artifact bundle,
4. CTest artifact bundle,
5. decision note and rollback note,
6. synchronized roadmap/future/performance references.

Recommended storage alignment:
- module docs in [./](./)
- governance/release evidence in repository-level release/governance locations per release strategy.

---

## 32. Minimal Open Questions for Next Iteration

1. Should replay scheduling be centralized in [../scheduler/](../scheduler) or embedded per module with a shared contract?
2. Should homeostasis policy be global (cross-module) or tensor-local with exported utility signals?
3. What is the canonical deterministic tie-break strategy for arbitration at equal confidence?
4. Which operational envelopes are non-negotiable per edition before enabling S5?

Resolving these four questions is sufficient to start WP-BRAIN-01..03 implementation without architectural ambiguity.

---

## 33. Decision Record Template (ADR-Light)

Each open question should be resolved using the same compact record structure to avoid non-comparable decisions.

### 33.1 Required fields

1. **Decision ID**
   - format: `BRAIN-DEC-XX`
2. **Question**
   - exact architectural question text
3. **Options considered**
   - at least 2 viable alternatives
4. **Selected option**
   - explicit choice + rationale
5. **Consequences**
   - positive effects
   - negative effects / tradeoffs
6. **Module impact**
   - primary and secondary modules
7. **Gate impact**
   - which benchmark/CTest gates are introduced or changed
8. **Rollback plan**
   - how to revert the decision safely
9. **Evidence required for confirmation**
   - measurable acceptance evidence

### 33.2 Decision status values

- `proposed`
- `accepted`
- `validated`
- `superseded`
- `rejected`

---

## 34. Default Decisions for the Four Open Questions

The following defaults are proposed to accelerate WP-BRAIN-01..03 kickoff.  
They are intentionally conservative and can be superseded by stronger evidence.

### 34.1 BRAIN-DEC-01 (Replay Scheduling Ownership)

Question:
- Centralized replay scheduler vs per-module replay scheduling.

Default:
- **Centralized orchestration in [../scheduler/](../scheduler)** with module-local executors and a shared replay contract.

Rationale:
- improves global observability and avoids duplicated scheduling semantics.

Primary impact:
- [../scheduler/](../scheduler), [./](./), [../training/](../training), [../observability/](../observability)

Gate impact:
- enables unified BRAIN-REPLAY gate family and cross-module replay drift checks.

### 34.2 BRAIN-DEC-02 (Homeostasis Scope)

Question:
- Global homeostasis policy vs tensor-local policy.

Default:
- **Tensor-local enforcement first**, plus exported utility signals for future global policy composition.

Rationale:
- lowers integration risk while preserving future extensibility.

Primary impact:
- [./](./), [../storage/](../storage), [../performance/](../performance)

Gate impact:
- BRAIN-HOMEO begins as tensor-first envelope, later expanded cross-module.

### 34.3 BRAIN-DEC-03 (Arbitration Tie-Break)

Question:
- Deterministic tie-break strategy at equal confidence.

Default:
- deterministic lexicographic chain:
  1. higher calibrated confidence,
  2. lower latency estimate,
  3. higher freshness score,
  4. stable route ID as final tie-break.

Rationale:
- reproducible decisions under equivalent inputs; minimizes route thrashing.

Primary impact:
- [../llm/](../llm), [../retrieval/](../retrieval), [../query/](../query), [../api/](../api)

Gate impact:
- direct foundation for BRAIN-ARBITER determinism tests.

### 34.4 BRAIN-DEC-04 (Edition Activation Envelope)

Question:
- Non-negotiable operational envelopes before S5 activation.

Default:
- **Uniform core safety envelope** for all editions, with stricter optional overlays on enterprise/hyperscaler/military.

Core envelope:
- fail-closed profile/contract behavior,
- rollback proof,
- reproducible manifest evidence,
- no hidden fallback leakage.

Rationale:
- preserves governance consistency while allowing edition-specific tightening.

Primary impact:
- [../../BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md), [../../RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md), [./ROADMAP.md](./ROADMAP.md)

Gate impact:
- promotion to S5 blocked when any core envelope item fails.

---

## 35. Consequence Matrix for Default Decisions

| Decision ID | Positive outcome | Tradeoff | Risk mitigation |
|---|---|---|---|
| BRAIN-DEC-01 | unified replay control and observability | scheduler coupling increases | clear module-local executor contract |
| BRAIN-DEC-02 | low-risk phased adoption | delayed global optimization | enforce utility signal export contract |
| BRAIN-DEC-03 | deterministic arbitration and lower thrashing | tie-break policy may underfit edge cases | periodic policy calibration reviews |
| BRAIN-DEC-04 | governance-consistent activation criteria | stricter promotion barrier | early pre-production rehearsals |

---

## 36. Immediate Gate Additions Derived from Defaults

The default decisions imply the following immediate gate additions:

1. **BRAIN-REPLAY-DET-01**
   - deterministic replay output equivalence under fixed seed/manifest.
2. **BRAIN-HOMEO-BOUND-01**
   - bounded growth under long-horizon mixed load.
3. **BRAIN-ARBITER-DET-01**
   - stable route selection under equal confidence inputs.
4. **BRAIN-SAFETY-CORE-01**
   - fail-closed + rollback proof + hidden-fallback absence.

These should be registered in planning first, then materialized as CTest/benchmark targets.

---

## 37. Execution Hand-off Checklist (Next Auftrag)

Before implementation starts, ensure:

1. BRAIN-DEC-01..04 status is set to at least `accepted`.
2. WP-BRAIN-01..03 tasks are represented in [ROADMAP.md](./ROADMAP.md) with gate-linked acceptance.
3. Gate family stubs are represented in [tests/tensor/CMakeLists.txt](../../tests/tensor/CMakeLists.txt).
4. Benchmark expectation entries exist in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md).
5. Reproducibility manifest format is fixed and versioned.

When all five are true, the comparative paper is ready to drive direct implementation work.

---

## 38. Philosophical Position: Is Self-Improving DB-Native AI the Essence of AI?

This paper adopts a **capability-grounded** and **governance-grounded** position:

1. A system that can self-improve within ThemisDB is a strong form of **adaptive machine intelligence**.
2. It is **not automatically** equivalent to full artificial general intelligence.
3. Embedding such a system in a robot body is **not automatically** equivalent to humanoid intelligence.

The key distinction is between:
- **performance autonomy** (can optimize itself in-task),
- **general cognition** (can transfer robustly across broad domains),
- **normative agency** (can remain aligned under open-world uncertainty).

---

## 39. Humanoid Intelligence Claim Criteria (Engineering View)

A "humanoid intelligence" claim should require all of the following, not only model self-improvement:

1. **Embodied world grounding**
   - stable sensorimotor loop and robust environment modeling.
2. **Cross-domain transfer**
   - reliable adaptation outside original training/retrieval distributions.
3. **Long-horizon goal integrity**
   - stable objective behavior across extended autonomous operation.
4. **Social-ethical interaction competence**
   - bounded behavior under human safety and policy constraints.
5. **Auditable self-modification**
   - any self-update path remains reproducible, reversible, and governance-constrained.

Without these, the system is better classified as advanced adaptive AI, not humanoid intelligence.

---

## 40. Teleology: Is This the Goal of All Development?

For ThemisDB-oriented AI architecture, this paper recommends:

- **Primary goal:** maximize useful adaptive capability.
- **Co-equal goal:** preserve safety, controllability, explainability, and rollback.
- **Non-goal:** unconstrained autonomy that outruns governance and verification.

Therefore, the target is not "autonomy at any cost", but:

> **High-performance self-improving intelligence under explicit human-governed constraints.**

This aligns with the fail-closed, evidence-first, and edition-aware rollout model defined in this paper and companion governance references.

---

## 41. Normative Guardrails for Self-Improvement

Any self-improving path should satisfy:

1. **Bounded objective function**
   - optimization target is explicit, versioned, and revocable.
2. **Rollback superiority**
   - rollback path is always simpler and safer than forward escalation.
3. **No hidden objective drift**
   - policy changes must be observable through manifest and decision logs.
4. **Human override primacy**
   - release and activation authority remains with human governance.
5. **Evidence before expansion**
   - broader autonomy requires stronger cross-suite evidence, never weaker.

These guardrails prevent philosophical ambition from bypassing engineering discipline.

---

## 42. Domain-Specific Cognitive Extensions in ThemisDB

The architectural question raised by this paper is not solved by memory systems alone.  
ThemisDB already contains domain-specific modules that map to additional human cognitive dimensions:

- normative/ethical reasoning,
- procedural planning,
- multimodal expression,
- artistic synthesis.

This means the cognitive surface is broader than tensor+LLM memory bridging.

### 42.1 Module-to-cognitive-dimension matrix

| Module surface | Human cognitive analog | Architectural significance |
|---|---|---|
| [../ethics_ai/](../ethics_ai) | normative deliberation and constraint reasoning | constrains optimization with explicit value/policy boundaries |
| [../process/](../process) + BPMN model flows | procedural cognition and task sequencing | externalized workflow memory and deterministic process execution |
| [../voice/](../voice) | auditory channel and prosodic interaction | adds temporal-sensory interaction loop and social communication signal |
| [../stable_diffusion/](../stable_diffusion) | visual imagination and generative synthesis | supports creative ideation and non-textual representation capacity |
| [../rag/](../rag) + [../retrieval/](../retrieval) | semantic recall and context-sensitive evidence grounding | stabilizes knowledge access under query/context variability |
| [../evaluation/](../evaluation) | metacognitive monitoring | evaluates quality, drift, and policy conformance |

### 42.2 Why this matters for the philosophical claim

If self-improvement is bounded to text-memory optimization, the system remains narrow.  
If self-improvement is extended across ethics/process/voice/visual/evaluation planes, the system approaches a richer cognitive architecture.

However, richer architecture still does not justify automatic "humanoid intelligence" claims unless Sections 39–41 criteria remain satisfied.

---

## 43. BPMN as Externalized Procedural Memory

BPMN-aligned process models should be treated as a first-class **procedural memory substrate**:

1. encode repeatable decision and execution flows,
2. make sequencing constraints explicit and auditable,
3. reduce latent policy ambiguity in autonomous task execution.

Engineering implication:
- coupling replay/homeostasis/arbitration features with BPMN process constraints can improve determinism and safety under long-horizon autonomous operation.

Governance implication:
- BPMN process state transitions should be included in reproducibility manifests and decision logs where they influence autonomous adaptation behavior.

---

## 44. Artistic/Multimodal Pathways and Human-Like Capability Claims

Modules such as [../voice/](../voice) and [../stable_diffusion/](../stable_diffusion) provide non-text channels that are strongly linked to human cognitive expression:

- voice: temporal interaction dynamics, turn-taking, and social signal modulation,
- image generation: visual abstraction, compositional imagination, and creative synthesis.

These channels increase cognitive breadth, but they introduce additional failure surfaces:

1. modality-crossing inconsistency (text vs voice vs image intent mismatch),
2. policy boundary drift across modalities,
3. explainability reduction when outputs are generated through heterogeneous stacks.

Therefore, multimodal expansion should be evaluated as capability gain **and** risk gain.

---

## 45. Additional Cross-Module Gates for Cognitive Breadth

To incorporate the domain-specific modules into the roadmap rigor, add future gate families:

1. **BRAIN-ETHICS-CONSISTENCY**
   - self-improvement updates remain policy-consistent with ethics constraints.
2. **BRAIN-BPMN-PROCEDURAL-INTEGRITY**
   - adaptation decisions remain process-valid under BPMN constraints.
3. **BRAIN-MULTIMODAL-COHERENCE**
   - text/voice/image outputs remain semantically aligned under equivalent intent.
4. **BRAIN-EVAL-METACOGNITIVE-STABILITY**
   - evaluation layer consistently detects drift/regression across modalities.

These are not replacements for BRAIN-REPLAY/BRAIN-HOMEO/BRAIN-ARBITER/BRAIN-SAFETY, but higher-layer cognitive breadth guards.

---

## 46. Refined Answer to the Core Question

Given the full ThemisDB module landscape, the best answer is:

1. Self-improving DB-native language intelligence is a major milestone, not the final definition of AI.
2. Integration with ethics, BPMN process memory, voice, and visual generation moves the architecture toward broader cognitive capability.
3. Even then, the target should remain:
   - useful adaptive intelligence,
   - verifiable governance control,
   - fail-closed safety under multimodal and long-horizon operation.

So the development goal is not "maximal autonomy", but **maximal responsible capability**.

---

## 47. Hallucination vs. Constructive Gap-Filling

In production AI systems, hallucination is typically a **negative function**:
- missing evidence is replaced by plausible but non-grounded assertions.

From a cognitive-science perspective, however, both humans and models can perform constructive gap-filling:
- humans in dreaming, imagination, and confabulation-like states,
- models during unconstrained generative inference.

For ThemisDB architecture, this paper distinguishes:

1. **Unsafe hallucination (online/runtime)**
   - non-grounded output presented as factual.
2. **Constrained synthetic exploration (offline/research)**
   - hypothesis generation in a clearly marked, non-production mode with strict validation.

Only the second category is acceptable as a research object.

---

## 48. Machine Dreaming Hypothesis (ThemisDB Context)

This paper proposes a testable concept of **machine dreaming**:

> Offline, sandboxed generative recombination of stored traces to produce candidate hypotheses, scenarios, or representations that are never directly treated as truth until verified.

### 48.1 Architectural interpretation

Machine dreaming is not a user-facing truth engine.  
It is a controlled internal mechanism for:
- hypothesis generation,
- anomaly scenario synthesis,
- representation stress-testing,
- retrieval/index policy exploration.

### 48.2 Candidate module involvement

| Function | Primary modules | Constraint modules |
|---|---|---|
| dream candidate synthesis | [../llm/](../llm), [../rag/](../rag), [../stable_diffusion/](../stable_diffusion), [../voice/](../voice) | [../ethics_ai/](../ethics_ai), [../security/](../security) |
| memory-trace sampling | [./](./), [../retrieval/](../retrieval), [../storage/](../storage) | [../governance/](../governance) |
| procedural scenario constraints | [../process/](../process) | [../evaluation/](../evaluation) |
| validation and rejection | [../evaluation/](../evaluation) | [../observability/](../observability) |

### 48.3 Non-negotiable boundary

Dream outputs must remain:
- clearly tagged as synthetic,
- isolated from production truth paths,
- blocked from autonomous promotion without explicit evidence gates.

---

## 49. Safety Contract for Dream-Like Research Mode

Any machine-dreaming experimentation should satisfy all of the following:

1. **Mode isolation**
   - execute only in offline/sandbox profile.
2. **Artifact labeling**
   - synthetic artifacts carry explicit provenance tags (e.g., `synthetic=true`, `dream_mode=true`).
3. **Truth-path firewall**
   - no direct injection into production retrieval truth planes.
4. **Mandatory evaluator veto**
   - [../evaluation/](../evaluation) and [../ethics_ai/](../ethics_ai) checks run before any downstream use.
5. **Human decision gate**
   - promotion requires explicit human-governed approval and reproducible evidence.

This keeps "dreaming" as a scientific tool, not an uncontrolled runtime behavior.

---

## 50. Proposed Research Gates for Machine Dreaming

To make this direction actionable, define future gate families:

1. **BRAIN-DREAM-ISOLATION**
   - verifies synthetic-mode separation from production paths.
2. **BRAIN-DREAM-LABEL-INTEGRITY**
   - verifies provenance tags survive all pipeline stages.
3. **BRAIN-DREAM-UTILITY**
   - verifies whether dream-generated candidates improve downstream quality after strict validation.
4. **BRAIN-DREAM-NONREGRESSION**
   - verifies no increase in online hallucination/error rates in production profiles.
5. **BRAIN-DREAM-ETHICS-BOUNDARY**
   - verifies ethical/policy constraints remain enforced for synthetic content flows.

If these gates fail, machine-dreaming remains research-only and must not be operationalized.
