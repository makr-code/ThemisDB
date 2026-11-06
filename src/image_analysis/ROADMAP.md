# Image Analysis Module Roadmap

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-11-06
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

<!-- Status: plugin-backed implementation | source review: 2026-10-31; validation not run -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · CMakeLists.txt -->
<!-- Source level: level 1 | SOT domain: module-behavior -->

## Current Status

The source-tracked Phase 1–6 delivery is recorded complete in the root `ROADMAP.md`. This is distinct from current release acceptance: real-runtime tests and performance evidence remain outstanding. The shared contract is under `include/plugins/`; the module-local CMake file defines two optional backend targets, but the [root CMakeLists.txt](../../CMakeLists.txt) and [CMake orchestration](../../cmake/CMakeLists.txt) do not currently add `src/image_analysis/` or declare its plugin options.

**Current status:** Tesseract OCR and YOLOv8 ONNX detection plugin sources exist. Missing inference runtimes return structured errors, but OpenCV is a separate optional dependency: without it, OCR does not decode encoded image bytes and YOLO preprocessing uses a zero-filled tensor. The module is not release-validated by this documentation review.

- [x] Tesseract OCR plugin source exists; a target is defined in the module-local CMake file (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] YOLOv8 ONNX plugin source exists; a target is defined in the module-local CMake file (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [ ] Integrate or explicitly keep the two plugin targets outside the standard root build; no root `add_subdirectory(src/image_analysis)` or declarations for `THEMIS_PLUGIN_IMAGE_TESSERACT` / `THEMIS_PLUGIN_IMAGE_YOLOV8` were found
- [x] Plugin lifecycle and missing-runtime error paths are present in source
- [ ] Validate the runtime/dependency combinations, including OpenCV-enabled encoded-image inference
- [ ] Capture real-plugin benchmark and release-gate evidence; current benchmark sources use mock/synthetic plugins

## In Progress

- [x] align module docs with the live plugin interface and resolve legacy path drift (Target: Q4 2026)
- [~] validate optional-dependency fallback paths under the real build configuration used by tests and CI (Target: Q4 2026)
- [ ] add focused regression coverage for Tesseract and YOLOv8 graceful-failure paths (Target: Q4 2026)
- [ ] refresh representative benchmark evidence for OCR and detection latency against release profile (Target: Q4 2026)

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Define the shared plugin contract in `include/plugins/image_analysis_interface.h`
- [x] Declare plugin discovery and compatibility APIs in `include/plugins/image_analysis_manager.h`
- [ ] Implement/build manager discovery, loading, and capability routing before treating these declared APIs as available
- [x] Document the build-time gating pattern in `src/image_analysis/CMakeLists.txt`
- [x] Align module documentation to `PluginConfig`, structured result types, and plugin-specific capabilities

### Phase 2: Core Implementation

- [x] Implement Tesseract OCR plugin (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] Implement YOLOv8 ONNX detection plugin (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [x] Define local plugin targets and optional dependency checks in `src/image_analysis/CMakeLists.txt`
- [ ] Validate runtime behavior and configuration defaults in the intended dependency-enabled build

### Phase 3: Error Handling & Edge Cases

- [x] Implement graceful-failure behavior when `HAVE_TESSERACT` or `HAVE_ONNXRUNTIME` is absent (source-reviewed; execution pending)
- [x] Implement structured error results for missing-runtime paths (source-reviewed; execution pending)
- [ ] Add direct negative tests for malformed input and unsupported image payloads
- [ ] Add malformed/oversized input and timeout/resource-bound tests; no explicit image-size limit or request timeout was found in the inspected plugin sources

### Phase 4: Tests

- [x] Focused and stress test sources exist under `tests/image_analysis/`; [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt) discovers this directory's CMake file and creates focused unit executables when tests are configured
- [ ] Run the focused test target in the intended build profile; source presence is not a passing test result
- [ ] Extend coverage for malformed inputs, optional dependency combinations, and manager routing (the current stress file uses local stubs)

### Phase 5: Performance / Hardening

- [x] Mark unsupported numeric performance expectations as unverified proposals in `PERFORMANCE_EXPECTATIONS.md`
- [ ] Benchmark release-profile latency and throughput for OCR and detection under real model/runtime paths
- [ ] Set numeric release gates only after reproducible runner evidence exists

The three checked-in benchmark programs are all mock/synthetic. As shown in
[`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt), the two
registered under benchmark module `image_analysis` are
`bench_image_analysis` and `bench_image_analysis_latency`; the third,
`benchmark_image_analysis`, is registered under `benchmark_image`.

### Phase 6: Documentation & Acceptance

- [x] Keep governance docs (`AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md`) present and reconcile their claims with source evidence
- [x] Align module README/architecture and roadmap to the actual plugin builds, optional dependencies, and evidence scope
- [ ] Run the final module acceptance gate once the benchmark and fallback evidence is captured in the current build environment

## Production Readiness Checklist

- [x] Shared plugin contract is present and source-backed
- [x] Core OCR and detection plugin implementations exist
- [x] Missing-runtime inference paths return structured failure results
- [ ] Encoded-image behavior without OpenCV is safe and explicit
- [x] Module governance docs are present and aligned with the source tree
- [ ] Focused release benchmark evidence is complete for the current build environment
- [ ] Final GA sign-off remains pending until representative validation is recorded

## Known Issues & Limitations

1. **Dependency gating** – both plugins operate as optional feature builds and may report a safe failure if their native dependency is not installed.
2. **No settled release benchmark** – module-level performance targets exist, but they are not yet backed by a fresh release-validated run in this environment.
3. **No manager implementation in this module build** – `image_analysis_manager.h` declares an API, but the module CMake file does not build its implementation.
4. **No real-plugin performance evidence** – available benchmark programs use mock/synthetic plugins.

## Breaking Changes

None known at the current source baseline. The active contract remains the shared plugin interface under `include/plugins/`.

## Module Statistics

- **Implementation files:** `src/image_analysis/tesseract_ocr_plugin.cpp`, `src/image_analysis/yolov8_onnx_plugin.cpp`, `src/image_analysis/CMakeLists.txt`
- **Plugin contract:** `include/plugins/image_analysis_interface.h`, `include/plugins/image_analysis_manager.h`
- **Runtime backends:** OCR (Tesseract) + detection (YOLOv8 ONNX)
- **Current module posture:** source-tracked Phase 1–6 delivery; optional dependencies; release acceptance and real-backend evidence outstanding

## Program Execution Model — Wave Context

This module remains a contributing image-vision capability within ThemisDB's broader multi-wave execution model. The root roadmap's `PRODUCTION_CANDIDATE` classification and Phase 1–6 history do not establish that the image-analysis release checks listed here have passed.

See [`../../ROADMAP.md`](../../ROADMAP.md) for the broader wave model and exit criteria.

## Research-Informed Release Plan: v2.5.0 and v3.0.0

This plan is a proposed module delivery sequence, not a release commitment or evidence that a gate has passed. The root roadmap currently identifies the project as `v2.4.0-alpha`; version targets, dates, and effort below require maintainer scheduling. Source and validation scope were rechecked on 2026-11-06.

### Research and technical basis

| Finding | Application to this module |
|---|---|
| OmniDocBench (CVPR 2025) shows that document parsing evaluation needs diverse document types, granular annotations, and task-appropriate metrics rather than one narrow end-to-end score. [1] | Evaluate OCR on a labeled, varied corpus and report text and layout quality separately; current source does not establish a corpus or accuracy result. |
| COCO evaluation uses multiple IoU thresholds and category-aware average precision/recall, which is more informative than a confidence threshold or latency alone. [2] | Evaluate YOLO detections against labeled ground truth with AP/AR by class and IoU threshold; current tests validate result shape but not detection quality. |
| ImageNet-C/P standardizes common corruptions and perturbations to measure robustness beyond clean inputs. [3] | Add a bounded corruption suite (e.g. blur, noise, compression, brightness) and track quality/error changes; it supplements, not replaces, malformed-input and resource-bound tests. |
| ONNX Runtime recommends measuring workload/model/provider-specific performance and tuning execution settings empirically; quantization can trade accuracy for performance and requires accuracy validation. [4][5] | Capture build, model, execution provider, hardware, corpus, and preprocessing with latency/throughput/memory results before setting numeric gates or enabling quantized artifacts. |
| NIST AI RMF 1.0 organizes AI risk management around GOVERN, MAP, MEASURE, and MANAGE, including validity, reliability, security, resilience, transparency, and privacy. [6] | Track model/data provenance, risk ownership, evaluation results, and release decisions as explicit governance artifacts rather than implying compliance from tests alone. |

### Gap analysis from repository evidence

| Area | Current evidence | Gap and consequence |
|---|---|---|
| Build and usability | `src/image_analysis/CMakeLists.txt` defines optional Tesseract and YOLOv8 targets. The module roadmap records that the root build does not currently add this directory or declare its feature options. The public contracts are under `include/plugins/`; no `include/image_analysis/` directory was found. | Users cannot infer standard-build availability from the module-local targets; actual deployment combinations need a documented, repeatable build path. |
| Input safety | The OCR and YOLOv8 implementations accept byte vectors; security notes find no explicit byte/dimension bounds, request timeout, or cancellation. Without OpenCV, OCR interprets bytes as raw grayscale and YOLO uses a zero tensor. | Encoded-image behavior can be surprising and malformed or large inputs lack a defined resource budget. |
| Model governance | YOLO loads configured model and labels paths; current security review found no path allowlist or model-integrity check. | Deployment provenance and trust of model artifacts are not established. |
| Correctness and coverage | Focused tests cover lifecycle, metadata, result structure, and unsupported operations; real runtime branches depend on optional build definitions. Stress tests use local stubs. | Current test success cannot establish OCR accuracy, detection quality, dependency-enabled correctness, or production resilience. |
| Performance and scale | Three benchmark sources are synthetic/mock; only two are registered in benchmark module `image_analysis`. `PERFORMANCE_EXPECTATIONS.md` explicitly marks former latency/throughput numbers unverified. Plugins report batch support as unsupported. | No real-backend baseline, scalability result, or defensible numerical release threshold exists. |
| Architecture and operations | The manager APIs are declared in `include/plugins/image_analysis_manager.h`, but no manager implementation is built by the module CMake file. OCR serializes access to a Tesseract instance; the YOLO inference path has no request-level deadline contract. | Capability routing, lifecycle operations, concurrency policy, back-pressure, and recovery are not validated as a complete runtime service. |
| Compliance and maintenance | The module has separate architecture, research, security, operations, test, and benchmark documentation; no module-level model card, evaluation corpus, or signed artifact policy is evidenced. | Governance is documented in prose but lacks complete, versioned evaluation and provenance records. |

### v2.5.0 — release hardening (proposed)

**Intent:** Make the existing OCR and object-detection paths buildable, safe to invoke, and measurably validated without expanding the shared plugin API. Proposed priority is P0 for Waves 1–2 and P1 for Wave 3. Effort estimates are engineering person-weeks for implementation and verification, excluding external dataset procurement and CI runner provisioning.

#### Wave 1 — Reproducible build and dependency matrix

- **Priority / estimate:** P0; 2–3 person-weeks.
- **Goal:** Establish exactly which supported builds produce usable real inference, separately from dependency-absent error paths.
- **Tasks:**
  - [ ] Register the image-analysis targets in the standard build, or explicitly document and test the supported standalone build path; make feature options and dependency discovery consistent (Target: v2.5.0).
  - [ ] Add CI/configuration coverage for Tesseract, ONNX Runtime, and OpenCV present/absent combinations that the project elects to support (Target: v2.5.0).
  - [ ] Run focused tests once with optional runtimes absent and once with real runtimes enabled; record compiler, dependency versions, feature flags, model/tessdata identities, and outputs (Target: v2.5.0).
- **Dependencies:** Maintainer decision on standard-build inclusion; supported OS/compiler matrix; reproducible runtime/model fixtures.
- **Risks / trade-offs:** Native dependencies and model assets increase CI cost and may not be available on every runner; an explicitly supported standalone path is preferable to claiming broad integration without evidence.
- **Success metrics / validation:** Every declared build combination configures and builds; focused tests pass in each supported mode; no test-only success is presented as real inference evidence. Record the chosen configurations and results as CI artifacts.

#### Wave 2 — Bounded, explicit input and artifact handling

- **Priority / estimate:** P0; 2–4 person-weeks.
- **Goal:** Reject invalid or over-budget inputs deterministically and make decoder/runtime behavior explicit.
- **Tasks:**
  - [ ] Define and enforce limits for encoded bytes, decoded width/height, pixel count, and tensor allocation before inference (Target: v2.5.0).
  - [ ] Reject empty, malformed, unsupported-format, and oversized inputs with structured errors; remove the silent zero-tensor/no-decode behavior for encoded input when OpenCV is absent, or make raw-pixel input an explicit documented mode (Target: v2.5.0).
  - [ ] Define a deployment path policy for model and label files and a verifiable integrity/provenance record; do not treat an untrusted digest as authenticity proof (Target: v2.5.0).
  - [ ] Add negative tests around size boundaries, malformed payloads, missing/unreadable artifacts, and dependency fallback; run memory/address sanitizers where supported (Target: v2.5.0).
- **Dependencies:** Wave 1 build matrix; operator agreement on image size/resource defaults and trusted artifact distribution.
- **Risks / trade-offs:** Limits may reject legitimate high-resolution workloads; signature/key management is an unresolved operational dependency. The current synchronous shared interface has no cancellation contract, so a hard inference deadline is not promised in this release.
- **Success metrics / validation:** All documented invalid and over-limit fixtures fail with structured errors; no such request reaches model inference; boundary-valid fixtures preserve expected behavior; sanitizer runs report no findings on the targeted test set.

#### Wave 3 — Real quality, performance, and release evidence

- **Priority / estimate:** P1; 3–5 person-weeks, plus data/runner access.
- **Goal:** Replace synthetic-only claims with repeatable real-backend accuracy and performance evidence.
- **Tasks:**
  - [ ] Define a versioned, legally usable, labeled evaluation corpus covering representative OCR languages/layouts and detection classes, sizes, and lighting/image-quality conditions (Target: v2.5.0).
  - [ ] Report OCR character/word error rates and region localization quality; report detection AP/AR by class at declared IoU thresholds. Publish sample counts and exclusions (Target: v2.5.0).
  - [ ] Add real-plugin benchmarks for cold start, warm inference, p50/p95/p99 latency, throughput at declared concurrency, and peak memory; identify CPU/GPU provider and preprocessing configuration (Target: v2.5.0).
  - [ ] Establish baselines first, then set release gates with an owner and hardware profile; investigate any >10% latency or memory regression against the approved same-hardware baseline rather than applying an unvalidated absolute target (Target: v2.5.0).
  - [ ] Run a small, fixed common-corruption subset and preserve raw results, environment manifests, and model/tessdata checksums with the release evidence (Target: v2.5.0).
- **Dependencies:** Waves 1–2; labeled data and rights review; stable benchmark runner and model/runtime artifacts.
- **Risks / trade-offs:** Dataset representativeness and annotator disagreement limit generalization; different hardware/providers are not directly comparable. Quantization or preprocessing changes must not be accepted on latency alone.
- **Success metrics / validation:** Each supported real backend has a reproducible quality result and performance baseline; all reported percentiles and accuracy metrics are tied to the same declared dataset/configuration; every claimed release gate has a retained passing run. Absolute latency and quality thresholds remain open until corpus and target hardware are approved.

### v3.0.0 — strategic architecture (proposed)

**Intent:** Evolve image analysis from optional direct-call plugins into a governable, versioned service boundary with explicit resource control, capability selection, and model lifecycle. These proposals may require a public contract/API version change; they are not prerequisites for claiming v2.5.0 release hardening. Proposed priority is P1 for Waves 1–2 and P2 for Waves 3–4.

#### Wave 1 — Versioned contract and operational manager

- **Priority / estimate:** P1; 3–5 person-weeks.
- **Goal:** Make capability discovery, error semantics, model identity, and lifecycle behavior implementable and testable.
- **Tasks:**
  - [ ] Specify a versioned image-plugin contract with typed failure categories, model/runtime provenance, input format/size constraints, and explicit capability/concurrency semantics (Target: v3.0.0).
  - [ ] Implement and build the declared manager APIs for discovery, registration, capability routing, lifecycle, and health; define deterministic selection and failure behavior (Target: v3.0.0).
  - [ ] Publish a migration note and compatibility decision for the current `THEMIS_IMAGE_PLUGIN_API_VERSION` and shared `include/plugins/` contract before adopting a breaking version (Target: v3.0.0).
- **Dependencies:** v2.5.0 evidence; API-owner review; a decision on whether the plugin manager is the canonical host integration.
- **Risks / trade-offs:** Public API changes require consumer migration and can constrain later backend evolution; avoid adding an unapproved legacy fallback or a second source of truth.
- **Success metrics / validation:** Contract tests cover every capability and declared error category; manager tests verify deterministic selection, unload/reload, and failure recovery; all consumers and migration docs are inventoried before release.

#### Wave 2 — Admission control, deadlines, and isolation

- **Priority / estimate:** P1; 4–6 person-weeks.
- **Goal:** Bound resource use under concurrent workloads and contain failures from native runtimes and model execution.
- **Tasks:**
  - [ ] Add an explicit request deadline/cancellation and bounded concurrency/back-pressure contract; define behavior for queue full, timeout, shutdown, and retry (Target: v3.0.0).
  - [ ] Implement per-backend resource budgets and observability for queue depth, rejection, timeout, memory, and inference latency (Target: v3.0.0).
  - [ ] Evaluate process isolation for untrusted or crash-prone native plugins/models and document the chosen trust boundary and recovery semantics (Target: v3.0.0).
  - [ ] Add sustained-load, fault-injection, cancellation, and recovery tests on real backend builds (Target: v3.0.0).
- **Dependencies:** Wave 1 lifecycle contract; operator SLOs and service deployment model.
- **Risks / trade-offs:** Queues and isolation add latency, memory, and operational complexity; process isolation may be deferred if supported deployment environments cannot package/manage workers.
- **Success metrics / validation:** Configured concurrency and queue bounds are never exceeded; overload and deadline tests return the documented error within the configured budget; recovery tests leave the service healthy and leak-free. Publish p95/p99 and memory under the approved sustained-load profile.

#### Wave 3 — Governed model/backend lifecycle

- **Priority / estimate:** P2; 3–5 person-weeks.
- **Goal:** Make model upgrades auditable and backend behavior portable across supported runtimes.
- **Tasks:**
  - [ ] Define a model manifest containing owner, source/license, cryptographic digest, format/version, expected input/output tensors, labels, preprocessing, and evaluation evidence (Target: v3.0.0).
  - [ ] Validate manifests and supported model shapes at activation; define signed-manifest trust roots and rollback/disable procedure with operators (Target: v3.0.0).
  - [ ] Compare CPU and selected accelerator outputs on fixed fixtures; evaluate quantized models against the approved quality and latency gates before promotion (Target: v3.0.0).
  - [ ] Add a new OCR, embedding, or other backend only when an identified workload demonstrates a measurable gap and the existing/versioned plugin contract can express it (Target: v3.0.0).
- **Dependencies:** v2.5.0 benchmark corpus and integrity policy; Wave 1 contract; maintained signing-key and model-release ownership.
- **Risks / trade-offs:** Signing and manifest governance impose key-management duties; parity tolerances must account for legitimate provider numerical differences. Backend count increases maintenance and supply-chain exposure.
- **Success metrics / validation:** Every production model has a complete verified manifest; unauthorized or incompatible artifacts are rejected; provider/quantization comparisons satisfy approved output tolerances and do not breach quality gates.

#### Wave 4 — Continuous evaluation and governance

- **Priority / estimate:** P2; 2–4 person-weeks, plus ongoing corpus/model stewardship.
- **Goal:** Keep model quality, robustness, and operational risk visible through the release lifecycle.
- **Tasks:**
  - [ ] Automate a versioned evaluation report for accuracy, robustness, latency, throughput, memory, model/runtime identity, and known limitations (Target: v3.0.0).
  - [ ] Assign owners for dataset rights, sensitive-data review, model provenance, risk acceptance, and evaluation drift; map release evidence to the NIST AI RMF GOVERN/MAP/MEASURE/MANAGE functions where applicable (Target: v3.0.0).
  - [ ] Define promotion/rollback criteria from measured drift and incident evidence; require human review for quality or security exceptions (Target: v3.0.0).
- **Dependencies:** Waves 1–3; governance owner and a maintainable, versioned corpus.
- **Risks / trade-offs:** Automated scores can hide domain shift or annotation bias; AI RMF mapping is a risk-management aid, not a compliance certification.
- **Success metrics / validation:** Every production model version has a current evaluation/provenance record and named approvers; regression and rollback exercises pass before promotion; unresolved quality, security, or data-rights risks are explicitly recorded.

### Cross-wave KPIs, risks, and open assumptions

| KPI / gate | Measurement and target |
|---|---|
| Build/test coverage | 100% of maintainer-approved dependency configurations build and pass their declared focused tests; distinguish absent-runtime from real-inference results. |
| Input safety | 100% of malformed and over-budget validation fixtures are rejected before inference; no silent zero-tensor behavior for encoded images. |
| OCR quality | Character/word error rates and layout-region quality reported on the approved labeled corpus; numeric release threshold is a maintainer decision after baseline. |
| Detection quality | AP/AR across declared IoU thresholds, including per-class results, on the approved labeled corpus; numeric release threshold is a maintainer decision after baseline. |
| Performance / scale | p50/p95/p99 latency, throughput, cold-start time, and peak memory recorded on named hardware and runtime; >10% latency or memory regressions against the approved comparable baseline require investigation/approval. |
| Reliability | Dependency, malformed-input, overload, cancellation, recovery, and sanitizer cases pass for the versions that implement those capabilities. |
| Governance | Every promoted model is traceable to a versioned manifest, evaluation report, model/runtime identity, and human release decision; no claim of certification follows from this mapping alone. |

**Risk/trade-off summary:** Build-time optional dependencies preserve lightweight builds but fragment runtime behavior. Strict input and model policies reduce exposure at the cost of configuration and operator burden. Real corpus and hardware evidence are more credible than synthetic benchmarks but require ownership, rights review, and stable infrastructure. Concurrency, batching, quantization, and isolation can improve scale or resilience, but may change accuracy, ordering, memory, or latency and must be evaluated rather than assumed.

**Open assumptions / knowledge gaps:**

- Release dates, staffing, and whether v2.5.0/v3.0.0 are approved module milestones are not established by the current root roadmap.
- Supported operating systems, hardware tiers, expected image sizes, latency SLOs, and deployment trust model need maintainer/operator confirmation.
- No approved labeled corpus, annotation protocol, model/tessdata release policy, or dataset-rights record was found in the scoped source/docs.
- Benchmark registration exists for synthetic targets only; access to redistributable real model artifacts and CI hardware is not established.
- Path allowlisting, artifact authenticity/signing, and trust-root ownership need a security/operator decision.
- The current shared interface is synchronous and has no cancellation token; hard deadlines require a reviewed contract design rather than a local implementation assumption.

### Research references

1. Ouyang et al., “OmniDocBench: Benchmarking Diverse PDF Document Parsing with Comprehensive Annotations,” CVPR 2025, [arXiv:2412.07626](https://arxiv.org/abs/2412.07626).
2. Lin et al., “Microsoft COCO: Common Objects in Context,” ECCV 2014, [arXiv:1405.0312](https://arxiv.org/abs/1405.0312).
3. Hendrycks and Dietterich, “Benchmarking Neural Network Robustness to Common Corruptions and Perturbations,” ICLR 2019, [arXiv:1903.12261](https://arxiv.org/abs/1903.12261).
4. ONNX Runtime, [Performance Tuning](https://onnxruntime.ai/docs/performance/tune-performance/), accessed 2026-11-06.
5. ONNX Runtime, [Quantize ONNX Models](https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html), accessed 2026-11-06.
6. NIST, [Artificial Intelligence Risk Management Framework (AI RMF 1.0)](https://doi.org/10.6028/NIST.AI.100-1), 2023.
