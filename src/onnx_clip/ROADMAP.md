---
Author: "ThemisDB Team"
Created: "2026-11-07"
Last Updated: "2026-11-14"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.h"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "tests/onnx_clip/"
  - "benchmarks/onnx_clip/"
Related_Issue: "#7438"
---

Exact model backend not verifiable from final document text alone.

# ONNX CLIP Module Roadmap: v2.5.0 and v3.0.0

> **Plan status:** evidence-based proposal, not a release commitment. Estimates are
> engineering person-weeks and depend on model, license, runtime, and hardware
> decisions. No model-backed tests or performance results are claimed by this plan.

## Current Status

| Area | Source-backed status |
|---|---|
| Implementation | `ONNXClipPlugin` implements the shared `IImageAnalysisBackend` contract, but generates deterministic hash/PRNG reference vectors. It does not decode image pixels, load CLIP weights, create an ONNX Runtime session, or produce a learned image/text space. |
| API and lifecycle | Initialization, readiness, statistics, sequential sub-batching, and reload/drain behavior exist. `AUTO` resolves to CPU; other backend labels do not activate an inference provider. A reload drain timeout can return `false` after the new state has already been published. |
| Resource and integrity boundaries | `max_batch_size` limits sequential chunk size, not total batch allocation; image bytes, text length, and positive embedding dimensions have no upper bounds in the plugin. Digest checking requires both a path and expected digest; in non-OpenSSL builds it is skipped when no hash callback is installed. |
| Tests | Three focused executables are conditionally registered when the ONNX image plugin option is enabled. Their generated inputs and mock files validate reference behavior and lifecycle properties, not real model output, retrieval quality, or provider execution. The high-cardinality stress source is not selected by the focused-test glob. |
| Benchmarks | The two benchmark programs use synthetic/mock implementations rather than `ONNXClipPlugin` or ONNX Runtime. `baselines.json` is a historical mock artifact, not measured module performance or an enforced release gate. |
| Public include directory | `include/onnx_clip/` contains documentation, not C++ headers; the declaration is source-local in `src/onnx_clip/onnx_clip_plugin.h`. |

**Maturity assessment:** contract/reference implementation only. API and lifecycle
behavior can be exercised, but semantic CLIP inference and production retrieval
readiness are **not demonstrated**. Successful initialization, normalized vectors,
model labels, optional runtime linkage, and benchmark names must not be presented
as evidence of loaded weights or semantic quality.

## Planned Features

- [ ] Complete v2.5.0 Waves 1–4: approve the model contract, add bounded CPU ONNX
  inference, and establish correctness, retrieval-quality, and measured
  performance evidence (Target: v2.5.0).
- [ ] Complete v3.0.0 Waves 5–7: add only validated acceleration/optimization,
  operational and supply-chain controls, and release acceptance evidence
  (Target: v3.0.0).

## Research Findings and Design Implications

1. CLIP's cross-modal behavior comes from learned image and text encoders trained
   into a shared space; arbitrary deterministic vectors cannot establish that
   behavior. The original CLIP paper's zero-shot classification results are
   specific to its models and evaluation sets, not a performance or accuracy
   prediction for this plugin. Therefore evaluate image-to-text and text-to-image
   retrieval on a fixed, representative dataset, not only embedding shape or
   ImageNet classification. ([Radford et al., 2021](https://arxiv.org/abs/2103.00020))
2. Preprocessing and tokenization are part of the model contract. The selected
   ONNX export must be paired with its image transform, tokenizer, context limit,
   output normalization, and reference implementation. The OpenAI reference uses
   a specific RGB/resize/crop/normalization transform; it must not be assumed for
   another checkpoint. ([OpenAI CLIP preprocessing](https://github.com/openai/CLIP/blob/main/clip/clip.py))
3. ONNX Runtime execution providers, thread-pool settings, and quantization have
   hardware- and graph-dependent effects. Provider selection must be verified
   from actual session placement, and optimization must be measured against the
   same quality suite rather than inferred from a configuration label.
   ([Execution providers](https://onnxruntime.ai/docs/execution-providers/),
   [thread tuning](https://onnxruntime.ai/docs/performance/tune-performance/threading.html),
   [quantization](https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html))
4. Image bytes and model files are untrusted inputs. Decode with explicit format,
   byte, dimension/pixel-count, and time limits. Pin artifacts by digest and
   preserve provenance, conversion inputs, license/model card, and evaluation
   records. A digest establishes equality to a trusted expected digest, not
   model safety, provenance, licensing, or quality. ([OWASP File Upload
   Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/File_Upload_Cheat_Sheet.html),
   [SLSA Build Provenance](https://slsa.dev/spec/v1.2/build-provenance),
   [NIST AI RMF](https://doi.org/10.6028/NIST.AI.100-1))

## Prioritized Release Plan

The root roadmap currently identifies v2.4.0-alpha; repository versioning names
v2.5.0 as the next planned minor. This module plan uses v2.5.0 as the requested
near-term track and v3.0.0 as a strategic target only; it does not set release
dates, approve a breaking API change, or change project-wide release readiness.
See [`../../VERSIONING.md`](../../VERSIONING.md) and
[`../../ROADMAP.md`](../../ROADMAP.md).

| Priority | Release track | Waves | Intended exit |
|---|---|---|---|
| P0 | v2.5.0 | 1–4 | A selected, licensed model has a reproducible CPU ONNX path, bounded inputs, model-backed correctness and retrieval evidence, and measured performance. |
| P1 | v3.0.0 | 5–7 | Only evidence-supported provider/optimization combinations are exposed, with operational, supply-chain, quality, and release gates met. |

## Implementation Phases

### Phase 1: Design / API Contract — Wave 1 (v2.5.0, P0)

- **Goal:** define the model-serving contract before implementation.
- **Tasks:** select an ONNX checkpoint and version; confirm license and distribution
  terms; record upstream and converted artifact digests; specify image preprocessing,
  text tokenizer/context length, output dimension/normalization, CPU runtime version,
  supported input formats, failure behavior, and configuration migration. Select a
  licensed retrieval set, split, relevance rules, prompts, and an independent
  reference implementation. Decide whether model-backed inference is opt-in in
  v2.5.0 and the migration path from the current deterministic reference behavior.
- **Dependencies:** maintainer approval for model/license and compatibility policy;
  artifact source and retrieval-data availability.
- **Risk / trade-off:** model size, license, or host API constraints can change
  delivery scope. Avoid promising an accelerator before a representative runner
  and runtime build exist.
- **Success metric:** 100% of the contract decisions above are recorded and
  reviewed; artifact and evaluation-set redistribution/use terms are documented.
- **Validation:** design review against `IImageAnalysisBackend`,
  [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md), the threat model,
  target packaging rules, and the selected model's reference preprocessing.
- **Estimate:** 1–2 person-weeks.

### Phase 2: Core Implementation — Wave 2 (v2.5.0, P0)

- **Goal:** provide one verifiable, model-backed CPU inference path.
- **Tasks:** construct ONNX Runtime sessions from the pinned artifact; implement
  model-matched image decoding/preprocessing and text tokenization; validate
  runtime input/output names, shapes, dimensions, finite values, and normalization;
  report actual provider use. Keep the public shared interface stable where
  possible and fail initialization/request execution clearly when required model
  assets or runtime support are absent. Do not silently substitute synthetic
  vectors for model results.
- **Dependencies:** Wave 1's approved checkpoint, conversion, tokenizer, and
  preprocessing contract.
- **Risk / trade-off:** optional OpenCV/runtime package discovery does not prove
  current provider or decoder parity; CPU-only support may increase latency while
  delivering a portable first implementation.
- **Success metric:** 100% of fixed image/text fixtures return the declared
  dimension, finite values, and L2 norm within `1 ± 1e-4`; 100% of fixtures match
  the independent FP32 reference at cosine similarity `>= 0.999` (with the
  runtime/export tolerance frozen in Wave 1).
- **Validation:** model-backed unit tests for image and text paths, empty and
  malformed input, wrong tensor names/shapes, unavailable model files, and
  CPU/provider reporting. Run with the pinned artifact in CI; tests using mock
  model files do not count for this gate.
- **Estimate:** 3–5 person-weeks.

### Phase 3: Error Handling & Edge Cases — Wave 3 (v2.5.0, P0)

- **Goal:** bound request cost and fail closed at trust boundaries.
- **Tasks:** enforce configurable maximum compressed bytes, decoded width/height
  and pixel count, text bytes/tokens, embedding dimension, and total batch count
  before large allocation; reject unsupported or malformed formats; make
  integrity-required mode fail when a verifier is unavailable; restrict model
  files to an approved immutable artifact path; test session shutdown and reload
  semantics, including the existing publish-before-drain behavior.
- **Dependencies:** Wave 1 input/configuration contract and Wave 2 decoder/session.
- **Risk / trade-off:** strict limits can reject previously accepted large inputs;
  host-level authorization and request admission remain outside this plugin and
  must not be implied by plugin-local checks.
- **Success metric:** every over-limit, malformed, unsupported-format, digest
  mismatch, and missing-verifier case is rejected before inference/allocation
  beyond the configured bound; no silent runtime/provider fallback.
- **Validation:** boundary-value and adversarial decoder tests, digest/read-error
  tests, concurrent inference/reload tests, plus ASan/UBSan and supported
  thread-sanitizer runs where the existing build lanes permit.
- **Estimate:** 2–3 person-weeks.

### Phase 4: Tests — Wave 4 (v2.5.0, P0)

- **Goal:** establish semantic and reproducible evidence, not merely vector shape.
- **Tasks:** add fixed model-backed golden fixtures and both retrieval directions;
  report Recall@1/5/10 and a ranking metric, with dataset/version, gallery size,
  relevance rules, prompts, artifact digest, tokenizer/preprocessing, runtime,
  provider, and environment recorded. Add production-plugin benchmarks for
  batch sizes 1/8/16 and cold/warm initialization. Record p50/p95/p99 latency,
  embeddings/second, and peak RSS; keep measured results separate from proposed
  targets.
- **Dependencies:** Waves 1–3, a licensed fixed evaluation set, and a reproducible
  artifact-fetch/cache path.
- **Risk / trade-off:** dataset leakage, domain mismatch, and hardware variation
  can make scores or timings misleading. The checked-in synthetic benchmarks and
  `baselines.json` are not acceptable substitutes.
- **Success metric:** retrieval Recall@1/5/10 is reported for both image→text and
  text→image and is within 1.0 percentage point absolute of the selected
  checkpoint's independent reference on the same fixed protocol. A model-backed
  baseline and host/hardware metadata are published; no absolute latency target
  is adopted until this baseline exists and maintainers approve it.
- **Validation:** execute focused CTest targets with the pinned model artifact;
  compare against the independent reference; run benchmark repetitions on named
  hardware and retain raw results. Gate CI on functional/quality tolerances, not
  noisy wall-clock timing except on a controlled runner.
- **Estimate:** 2–4 person-weeks.

### Phase 5: Performance / Hardening — Wave 5 (v3.0.0, P1)

- **Goal:** improve throughput without weakening model quality or provider clarity.
- **Tasks:** profile the Wave 4 CPU baseline; implement true batched
  `Ort::Session::Run` where model graph and API permit; tune thread pools and
  provider options from measurements; add a GPU execution provider only if its
  runtime and representative CI/deployment hardware are supported. Evaluate
  FP16/quantized variants separately against the identical retrieval suite.
- **Dependencies:** v2.5.0 model-backed inference, fixed benchmark protocol, and
  named target hardware/provider availability.
- **Risk / trade-off:** batching, thread oversubscription, provider partitioning,
  and quantization can increase memory or degrade retrieval quality. Accelerator
  labels alone are not evidence of accelerator execution.
- **Success metric:** on a named supported GPU and batch size 32, measured
  throughput is at least 1.5× the same-model CPU baseline, with the actual
  provider assignment recorded; any reduced-precision candidate loses no more
  than 1.0 percentage point absolute Recall@1 versus the FP32 baseline. If the
  hardware gate is unavailable, no GPU performance claim is made.
- **Validation:** same artifact/data protocol across CPU and candidate provider;
  record provider placement, p50/p95/p99, throughput, peak RSS, cold start, and
  numerical/retrieval deltas. Test batch sizes 1/8/16/32/64, OOM/fallback, and
  concurrent calls; do not treat mock benchmark cases as evidence.
- **Estimate:** 3–5 person-weeks.

### Phase 6: Documentation & Acceptance — Waves 6–7 (v3.0.0, P1)

#### Wave 6: Operations, security, and governance

- **Goal:** make the model lifecycle supportable and auditable.
- **Tasks:** document model source, license/model card, digests and conversion
  provenance; define approved model update and rollback procedure; expose
  observable request/error/latency/provider metrics through existing host
  telemetry; document data retention and privacy boundaries; set provider,
  memory, and concurrency limits; define incident handling for corrupt models,
  decoder failures, resource exhaustion, and failed reloads.
- **Dependencies:** Waves 1–5 and agreement with host telemetry/deployment owners.
- **Risk / trade-off:** plugin telemetry can duplicate host metrics; retain
  host-owned authorization and avoid high-cardinality or sensitive labels.
- **Success metric:** every deployable model artifact has a verified digest,
  documented source/license, and conversion record; all supported provider and
  reload failure modes have an operator action; no unresolved critical/high
  security finding remains at sign-off.
- **Validation:** supply-chain/provenance review, security review of decoder and
  ONNX execution, failure-injection/reload tests, and an operator runbook exercise.
- **Estimate:** 2–3 person-weeks.

#### Wave 7: Release gate and compatibility

- **Goal:** ship only a reproducible, supported model-serving contract.
- **Tasks:** freeze supported model/runtime/provider combinations, config and
  migration notes; validate clean install, model retrieval, startup, shutdown,
  reload/rollback, and cross-provider behavior; update architecture, API,
  security, operations, test, and benchmark documentation together.
- **Dependencies:** Waves 1–6, maintainer sign-off, and all required CI/hardware
  lanes.
- **Risk / trade-off:** unsupported platforms must fail clearly rather than
  silently switch to a different model or backend.
- **Success metric:** all supported build/provider lanes pass the same model
  correctness and retrieval gates; zero undocumented runtime fallbacks; all
  release artifacts and migrations are reproducible from the recorded
  provenance.
- **Validation:** clean build/test on each declared lane, model-backed retrieval
  suite, provider/performance report, security/governance checklist, and
  documentation/link review.
- **Estimate:** 1–2 person-weeks.

## Production Readiness Checklist

| Area | Current status | Exit evidence |
|---|---|---|
| API and semantic behavior | Not production-ready for semantic retrieval | Wave 1 contract and Wave 2 model-backed reference parity |
| Model, tokenizer, preprocessing | Not selected or executed by current source | Pinned artifact and parity fixtures |
| Input and resource bounds | Incomplete in plugin | Wave 3 boundary/adversarial tests |
| Integrity and provenance | Optional digest; verifier can be absent | Fail-closed integrity mode and provenance review |
| Provider support | CPU `AUTO` reference label only; no inference provider runs | Verified provider placement and lane-specific tests |
| Retrieval quality | No model-backed benchmark | Bidirectional fixed-set metrics and accepted threshold |
| Performance and memory | No production-plugin measurements | Reproducible, hardware-labelled benchmark baseline |
| Operations and governance | Host owns several controls; model operations not established | Waves 6–7 runbook, monitoring, privacy, license, provenance sign-off |

## Known Issues & Limitations

- Current vectors are not CLIP embeddings and must not be used to claim
  cross-modal similarity, search quality, or model accuracy.
- The focused tests are conditional and currently exercise reference behavior;
  source presence is not evidence that the targets were built or passed.
- The benchmark directory contains synthetic/mock workloads and dated
  thresholds; there is no validated CPU/GPU latency, throughput, memory, or
  release-gate result for this plugin.
- Model identity/license, ONNX export method, tokenizer, preprocessing,
  retrieval dataset, supported providers, target hardware, and deployment limits
  remain open decisions.
- The Developer LLM wiki is derived context, not module evidence. Use the current
  source, tests, benchmarks, and canonical module documents above for release
  decisions.

## Breaking Changes

- Replacing deterministic reference vectors with learned model outputs changes
  output values and semantic expectations even if `IImageAnalysisBackend`
  signatures remain unchanged. Wave 1 must approve whether v2.5.0 makes this
  opt-in and document the migration before implementation.
- New request limits may reject inputs that currently reach the plugin.
- No compatibility fallback or synthetic production inference path is proposed.
  Any behavior migration must be explicit, reviewed, and must not silently return
  reference vectors when model loading or provider execution fails.

## Evidence and References

### Repository evidence

- [`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp) and
  [`onnx_clip_plugin.h`](./onnx_clip_plugin.h) — implementation and interface.
- [`tests/onnx_clip/README.md`](../../tests/onnx_clip/README.md) and
  [`tests/onnx_clip/CMakeLists.txt`](../../tests/onnx_clip/CMakeLists.txt) —
  focused test scope and registration.
- [`benchmarks/onnx_clip/README.md`](../../benchmarks/onnx_clip/README.md),
  benchmark sources, and [`baselines.json`](../../benchmarks/onnx_clip/baselines.json)
  — benchmark/mock boundary and historical values.
- [`include/onnx_clip/ROADMAP.md`](../../include/onnx_clip/ROADMAP.md) — confirms
  the include directory contains documentation and points to this canonical
  implementation roadmap.
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — candidate implementation
  details; its historical performance values are hypotheses, not current
  measurements or release gates.

### External research

- A. Radford et al., *Learning Transferable Visual Models From Natural Language
  Supervision* (2021), https://arxiv.org/abs/2103.00020
- OpenAI CLIP reference preprocessing, https://github.com/openai/CLIP/blob/main/clip/clip.py
- ONNX Runtime execution providers,
  https://onnxruntime.ai/docs/execution-providers/
- ONNX Runtime threading,
  https://onnxruntime.ai/docs/performance/tune-performance/threading.html
- ONNX Runtime quantization,
  https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html
- OWASP File Upload Cheat Sheet,
  https://cheatsheetseries.owasp.org/cheatsheets/File_Upload_Cheat_Sheet.html
- SLSA Build Provenance v1.2, https://slsa.dev/spec/v1.2/build-provenance
- NIST AI Risk Management Framework,
  https://doi.org/10.6028/NIST.AI.100-1
