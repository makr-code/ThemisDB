---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Stable Diffusion Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-10-31 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Core pipeline is operational in stub mode and optional real-backend mode when the
stable-diffusion.cpp library is found and linked.
ControlNet request fields, LoRA request handling, perceptual hash metadata,
request dimension guards, and the optional model SHA-256 gate are implemented
in `SDPlugin` and covered by focused tests; SHA-256 enforcement depends on the build gate.

## Completed ✅

- [x] `IImageGenerationBackend` interface + `THEMIS_IMGGEN_PLUGIN()` export macro
- [x] `SDPromptSanitizer` — keyword blocklist (case-insensitive, file-loadable)
- [x] `ISDGenerator` strategy interface
- [x] `SDStubGenerator` (CI / no model) with img2img pass-through
- [x] `InMemorySDGenerator` test double (with img2img inspection helpers)
- [x] `SDPlugin` — provenance stamps, blocked_count, DL entry points
- [x] `SDConfig::fromJson` / `toJson` with clamping
- [x] Real PNG encoder with IDAT chunk (stored-deflate, CRC-32, Adler-32 — no external deps)
- [x] `generateBatch(prompts, cfg)` on `IImageGenerationBackend` and `SDPlugin`
- [x] `generateImg2Img(prompt, cfg)` + `Img2ImgConfig` struct
- [x] `ISDGenerator::generateImg2Img()` default fallback
- [x] `SDStubGenerator::generateImg2Img()` — input-image pass-through
- [x] `SDCppGenerator` — real stable-diffusion.cpp wrapper (`THEMIS_ENABLE_STABLE_DIFFUSION` guard)
- [x] Thread-safety: `generate_mutex_` serialises all generate paths
- [x] `negative_prompt` content-policy enforcement (SECURITY.md gap SD-NP-01 resolved)
- [x] ControlNet request fields (`control_image_rgb`, `control_model_path`, `control_strength`)
- [x] LoRA request application (`applyLoRA`) with per-request scale validation
- [x] Perceptual hash metadata (`GeneratedImage::perceptual_hash`) with non-fatal fallback
- [x] Model SHA-256 verification gate in `initialize()` (`model_sha256`)
- [x] Dimension guards (`<=8192`, overflow-safe, positive dimensions)
- [x] 62 unit tests (`SDPluginFocusedTests`, groups A–Q)
- [x] Plugin manifest + CMake registration
- [x] **`SDCppGenerator`** — wraps `stable-diffusion.cpp` C API under `THEMIS_ENABLE_STABLE_DIFFUSION` guard (Issue: #4590) (2026-04-12)
- [x] **Real PNG IDAT encoder** — `encodeMinimalPng()` produces valid IDAT block via stored-deflate + CRC32 + Adler32 (Issue: #4590) (2026-04-12)
- [x] **`SDStubGenerator::generateImg2Img`** — returns input-image pass-through (Issue: #4590) (2026-04-12)
- [x] **51 unit tests** (`SDPluginFocusedTests`, groups A–Q) — 6 new tests groups P–Q for SDCppGenerator + real PNG encoder (Issue: #4590) (2026-04-12)
- [x] **`SDPluginAdapter` + `SDPluginRegistrar`** — `IThemisPlugin` adapter wrapping `SDPlugin`; `createPlugin`, `createAdapter`, `defaultReloadCallback`, `enableHotPlug`, `disableHotPlug`; 12 unit tests (`SDPluginRegistrarTests`, groups A–D) (2026-04-16)

## In Progress

*(none)*

## Planned Features

- [ ] Publish a representative real-model benchmark baseline and environment manifest (Target: v2.5.0)
- [ ] Execute the opt-in real-backend E2E suite against a pinned model/runtime and publish evidence (Target: v2.5.0)
- [ ] Verify `SDCppGenerator` concurrency, lifecycle, and resource behavior on the configured backend (Target: v2.5.0)

## Research-Based Roadmap: v2.5.0 and v3.0.0

### Research method and current maturity

**Evidence snapshot:** repository sources and tests under `src/stable_diffusion/`,
`include/stable_diffusion/`, `tests/stable_diffusion/`, and
`benchmarks/stable_diffusion/`, checked 2026-11-07, plus the external references below.
The module is **experimental / integration-capable, not yet operationally qualified**:
there is a public plugin contract, a stub path, and an optional `stable-diffusion.cpp`
backend, but the repository does not publish an executed real-model benchmark or soak
result. This is consistent with the maturity statement in `src/stable_diffusion/README.md`.

| Focus | Code and evidence today | Gap / consequence |
|---|---|---|
| Performance and scaling | `benchmarks/stable_diffusion/bench_stable_diffusion_release_gates.cpp` measures 512×512 stub and in-memory proxy time-to-PNG plus a 1/4-thread stability case. `benchmarks/stable_diffusion/BASELINE.md` has no measurements. `SDPlugin` and `SDCppGenerator` serialize inference calls with mutexes. | No real-model latency, throughput, startup, host-memory, or VRAM baseline; existing tests do not establish parallel speedup. Do not promise production throughput or extrapolate proxy numbers to GPU inference. |
| Use cases and usability | Public APIs cover text-to-image, batch, image-to-image, ControlNet fields, and LoRA; the real-backend E2E target is opt-in. | A direct server/API request path is unverified in `ARCHITECTURE.md`; deployment workflow, model acquisition, and user-facing use cases need confirmation before expanding the contract. |
| Safety and resilience | Prompt and negative-prompt checks, dimension/buffer validation, exception isolation, and an optional model SHA-256 gate exist. `SECURITY.md` correctly limits the keyword filter to a non-comprehensive control. | The literal blocklist is not a moderation guarantee; the digest is ignored unless its build gate and OpenSSL are available. Rate limiting is outside this module. The high-cardinality stress test uses in-process simulations, not `SDPlugin` or a real model. |
| Governance and maintainability | Plugin strategy, test doubles, CMake registration, provenance fields, and module security/production documents exist. | Failure paths do not populate every provenance field; prompt fingerprints are non-cryptographic; model/license ownership and retention policy are not evidenced. The generated `MODULE_GAPS.md` is stale and must not be used as current source evidence. |
| Architecture and operations | The optional backend is isolated behind `ISDGenerator`; 62 focused plugin tests, 12 registrar tests, two opt-in real-backend cases, and five modular test sources are documented. | Build-option/dependency and model requirements impede reproducibility; no executed real-backend E2E, hardware baseline, long-running soak, or module-specific operator runbook is evidenced. |

### Research-to-plan mapping

| Source | Relevant finding | Application to this module |
|---|---|---|
| Hugging Face, [Diffusers inference optimization](https://huggingface.co/docs/diffusers/optimization/fp16) | Diffusion generation is iterative; scheduler choice, precision, memory-efficient attention, and other optimizations trade latency and memory. The project publishes pipeline latency and memory benchmark data. | Measure end-to-end latency and memory on the actual backend first. Treat these methods as candidates only; stable-diffusion.cpp support and output quality must be verified before adoption. |
| Gong et al., [GreenStableYolo](https://arxiv.org/abs/2407.14982) (2024) | Reports a search-based time/quality trade-off for a particular Stable Diffusion setup; its results are configuration-specific, not transferable performance guarantees. | Compare optimizations at matched model, sampler, steps, seed policy, resolution, and hardware; record quality alongside speed. |
| NIST, [AI RMF Generative AI Profile, AI 600-1](https://doi.org/10.6028/NIST.AI.600-1) (2024) | Provides a voluntary risk-management profile for generative AI, organized around governance, mapping, measurement, and management. | Track intended use, threat assumptions, evaluation evidence, and deployment mitigations together; this is guidance, not a claim of regulatory compliance. |
| C2PA, [Content Credentials Technical Specification 2.5](https://spec.c2pa.org/specifications/specifications/2.5/specs/C2PA_Specification.html) | Defines technical claims and bindings for digital-media provenance; validity of provenance does not make a judgment about whether the content is good or trustworthy. | Consider opt-in asset provenance for generated PNGs, but keep content-policy decisions separate and validate privacy, codec, key-management, and consumer compatibility first. |

These references inform testable hypotheses, not evidence that the current backend supports
any particular optimization or meets a performance, safety, legal, or compliance target.

### v2.5.0 — evidence and production-boundary hardening

Scope: complete the missing proof and deployment controls for the existing plugin surface;
do not add a new serving architecture before real-backend behavior and consumers are verified.
Effort estimates are engineering person-weeks, not calendar commitments.

#### Wave 1 — Reproducible real-backend baseline (1–2 person-weeks)

- **Goal:** establish comparable baseline evidence for the real model path.
- **Tasks:**
  - [ ] Add an opt-in real-backend benchmark path to `bench_stable_diffusion_release_gates` (or a separately registered target) with model/runtime configuration supplied explicitly (Target: v2.5.0).
  - [ ] Publish hardware, OS, compiler, backend revision, model identifier and SHA-256, resolution, sampler, steps, seed policy, warm/cold state, and benchmark command with every result (Target: v2.5.0).
  - [ ] Capture p50/p95/p99 latency, images/second, initialization time, host RSS, and peak VRAM for stub, proxy, and real backend; use at least five independent repetitions (Target: v2.5.0).
- **Dependencies:** a licensed, integrity-pinned model fixture and a supported reference CPU/GPU runner; the benchmark target must build.
- **Risk / trade-off:** GPU availability and model licensing may block CI; proxy measurements must remain clearly separated from inference measurements.
- **Success metrics:** one reproducible real-backend baseline is published with all listed environment fields and percentile/resource measurements; repeated-run variance is reported, not hidden.

#### Wave 2 — Runtime reliability and resource limits (2–3 person-weeks)

- **Goal:** prove lifecycle, failure, and serialized-call behavior on the actual backend.
- **Tasks:**
  - [ ] Run `SDPluginRealBackendE2ETests` with the pinned model and record text-to-image and image-to-image results, PNG validity, model initialization, and shutdown behavior (Target: v2.5.0).
  - [ ] Add real-backend tests for concurrent callers, failed initialization, inference exceptions, invalid/oversized outputs, and repeated generation without resource growth (Target: v2.5.0).
  - [ ] Run and publish a minimum one-hour real-backend soak, recording request count, failures, latency percentiles, RSS, and VRAM (Target: v2.5.0).
- **Dependencies:** Wave 1 runner, pinned model/runtime, and deterministic test configuration.
- **Risk / trade-off:** the current mutexes serialize requests; changing that for throughput before the dependency's thread-safety contract is known risks corruption and VRAM exhaustion.
- **Success metrics:** all real-backend E2E cases pass; zero corrupt outputs or uncaught failures in the soak; memory returns to the measured steady-state envelope after requests; concurrent-call behavior is documented as serialized or proven safe.

#### Wave 3 — Threat controls and model trust (2–4 person-weeks)

- **Goal:** make the security boundary and model-integrity behavior explicit for production deployments.
- **Tasks:**
  - [ ] Define and test a production configuration that cannot silently ignore a supplied model digest; fail closed if verification is required but unavailable or mismatched (Target: v2.5.0).
  - [ ] Validate model and LoRA path authorization, file size/format limits, and deployment ownership at the loading boundary; document caller responsibilities where enforcement belongs outside the plugin (Target: v2.5.0).
  - [ ] Map every text2img, batch, img2img, ControlNet, and negative-prompt path to the configured content-policy check; document that literal keyword matching is not comprehensive moderation (Target: v2.5.0).
  - [ ] Establish a model/license inventory and approved-use decision for the pinned E2E fixture (Target: v2.5.0).
- **Dependencies:** Wave 1 fixture and threat-model review; rate limiting remains an API/deployment responsibility unless a server integration is established.
- **Risk / trade-off:** fail-closed integrity requirements may reduce optional-build portability; a blocklist-only filter must not be marketed as a safety guarantee.
- **Success metrics:** wrong/missing required digest and unauthorized model/adapter paths are rejected; tests cover policy screening on every generation path; model ID, digest, backend revision, and approved-use status are recorded for the reference fixture.

#### Wave 4 — API, provenance, and operator acceptance (1–3 person-weeks)

- **Goal:** confirm the consumer contract and make failure and deployment behavior supportable.
- **Tasks:**
  - [ ] Verify whether the server exposes a direct Stable Diffusion request path; either link its integration tests or explicitly retain plugin-only status (Target: v2.5.0).
  - [ ] Reconcile the public provenance contract with success and failure results; specify which fields are returned, which are safe to log, and ensure raw prompts are not persisted by default (Target: v2.5.0).
  - [ ] Add a concise operator procedure for model provisioning/verification, backend availability, resource exhaustion, and safe disablement (Target: v2.5.0).
  - [ ] Publish a v2.5.0 gate report linking Waves 1–3 evidence and unresolved limitations (Target: v2.5.0).
- **Dependencies:** Waves 1–3 and a confirmed caller/deployment owner.
- **Risk / trade-off:** adding metadata can expose sensitive prompt or model-path information; any public API change requires compatibility review.
- **Success metrics:** every advertised API has a verified integration path or is explicitly documented as plugin-only; provenance behavior matches the header contract; a reviewer can reproduce the baseline and follow the operator procedure without unstated model/runtime setup.

### v3.0.0 — scalable, governed inference service

Scope: only proceed after v2.5.0 provides real-backend evidence and the intended server/deployment use cases are approved. API/architecture work is subject to major-version compatibility review.

#### Wave 5 — Serving and resource-control design (3–5 person-weeks)

- **Goal:** specify an admission-controlled serving contract rather than exposing unbounded inference calls.
- **Tasks:**
  - [ ] Design an additive/major-version request lifecycle covering bounded queueing, cancellation/deadlines, per-tenant quotas, back-pressure, and shutdown semantics (Target: v3.0.0).
  - [ ] Specify model residency, load/unload, health reporting, and CPU/GPU fallback behavior against the confirmed server integration (Target: v3.0.0).
  - [ ] Review the contract for thread safety, ownership, privacy, and compatibility before implementation (Target: v3.0.0).
- **Dependencies:** v2.5.0 Waves 1–4, confirmed consumers, and the backend's model/context lifecycle guarantees.
- **Risk / trade-off:** queued requests improve utilization but increase tail latency and resource contention; cancellation may not be supported inside the inference backend.
- **Success metrics:** the design specifies finite queue and per-tenant limits, observable states for every request, defined cancellation/shutdown outcomes, and no unbounded resource-growth path.

#### Wave 6 — Measured throughput and quality optimization (4–8 person-weeks)

- **Goal:** improve useful throughput only where a matched benchmark demonstrates a real gain.
- **Tasks:**
  - [ ] Profile the v2.5.0 baseline and evaluate only backend-supported precision, attention, scheduler, batching, and quantization options (Target: v3.0.0).
  - [ ] Add a pinned prompt/evaluation set and compare latency, throughput, memory, and image/text alignment under matched model and generation settings (Target: v3.0.0).
  - [ ] Implement the best validated option behind an explicit configuration gate and retain the reference path for parity testing (Target: v3.0.0).
- **Dependencies:** Wave 1 benchmark harness, Wave 5 admission design, backend feature support, and an approved image-quality acceptance set.
- **Risk / trade-off:** speed, memory, and perceived quality may move in different directions; optimization results from Diffusers/PyTorch cannot be assumed to apply to stable-diffusion.cpp.
- **Success metrics:** demonstrate at least 1.5× throughput over the v2.5.0 baseline at the same hardware and request mix, without exceeding the pre-approved quality envelope (proposed: CLIP alignment decline ≤1% and FID degradation ≤5% on the pinned evaluation set); otherwise do not make the optimization a default.

#### Wave 7 — Provenance, policy, and release governance (3–5 person-weeks)

- **Goal:** make model and artifact lineage verifiable while preserving privacy and deployment choice.
- **Tasks:**
  - [ ] Define a versioned model manifest for source, license/approved use, digest, backend revision, and supported capabilities; reject artifacts that fail configured verification (Target: v3.0.0).
  - [ ] Evaluate opt-in C2PA-compatible provenance for generated images, including independent validation, key lifecycle, metadata stripping, and prompt privacy; implement only after security/legal review (Target: v3.0.0).
  - [ ] Define policy evaluation, incident handling, retention, and deletion responsibilities with the deploying product owner; do not treat provenance as proof of content safety (Target: v3.0.0).
  - [ ] Complete multi-tenant isolation, abuse/resource-exhaustion tests, dependency/model supply-chain review, and operator acceptance for the chosen deployment profile (Target: v3.0.0).
- **Dependencies:** Wave 3 model trust controls, Wave 4 consumer confirmation, Wave 5 serving contract, and legal/security review.
- **Risk / trade-off:** signed provenance improves traceability but adds key-management and compatibility costs; storing prompts or stable prompt hashes may create privacy and linkability risks.
- **Success metrics:** invalid signed manifests are rejected; 100% of generated results in the validated profile identify model digest and backend version; external provenance validation succeeds for supported output formats; isolation and resource-abuse suites pass with no cross-tenant result leakage.

### Cross-wave assumptions, risks, and decision gates

- Effort estimates assume one engineer-week equals focused implementation/test time; they exclude external model licensing, runner provisioning, review queues, and blocked dependency/build remediation. Re-estimate after Wave 1.
- The default benchmark fixture, target hardware, server consumer, model license, and required content policy are **open assumptions**, not established repository facts. Wave 1/4 must resolve them before release commitments.
- The Developer LLM Wiki status snapshot was generated 2026-11-02 and reports 1,450 removed sources with 26 changed; it is treated as secondary context. Module code, tests, benchmark files, and this module roadmap remain the evidence sources.
- A release gate cannot be called complete solely because a test or benchmark target exists: publish command, configuration, environment, and run result. No real-backend or performance pass is claimed by this plan.
- If real-model or representative-hardware evidence is unavailable, retain `[~]`/`[!]` status and do not claim production readiness. Preserve stub mode for ordinary CI without treating it as inference validation.

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `IImageGenerationBackend`, `GeneratedImage`, `SDGenerationConfig` defined
- [x] `SDPromptSanitizer` separating content policy from inference

### Phase 2 — Core Implementation ✅
- [x] `SDPlugin` wiring sanitizer → generator → provenance → PNG
- [x] `SDConfig` with JSON round-trip

### Phase 3 — Error Handling & Edge Cases ✅
- [x] Blocked prompts: `success=false`, `blocked_count++`, no generator call
- [x] Uninitialised state: `success=false`, `error_message`
- [x] Generator exception isolation

### Phase 4 — Tests ✅
- [x] Focused coverage expanded to 62 tests across groups A–Q
- [x] Added coverage for model SHA-256 gate, dimension guards, ControlNet+LoRA request flow, pHash behavior

### Phase 5 — Performance / Hardening
- [x] Thread-safe generate/generateBatch/generateImg2Img (v2.1.0)
- [x] `negative_prompt` content-policy enforcement (v2.1.0)
- [x] Real PNG encoder — stored-deflate zlib with CRC-32 and Adler-32 (v2.2.0)
- [x] Dimension guard enforcement for generation/img2img/control image paths
- [x] Model SHA-256 verification gate at initialization
- [~] Benchmark: time-to-PNG target added (`bench_stable_diffusion_release_gates`); real-model baseline pending
- [~] Thread-safety audit for SDCppGenerator (parallel calls) — opt-in real-backend audit target added

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (62 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Content-policy sanitizer before every generate call
- [x] Provenance stamps on every result
- [x] Thread-safe generate/generateBatch/generateImg2Img
- [x] `negative_prompt` content-policy enforcement
- [x] Batch generation API
- [x] Img2img interface with stub pass-through
- [x] Real PNG encoder (IDAT chunk, CRC-32, Adler-32 — no external deps)
- [x] `SDCppGenerator` — real stable-diffusion.cpp integration (compiled when `THEMIS_ENABLE_STABLE_DIFFUSION=ON`)
- [~] End-to-end integration test with a real GGUF model file — opt-in target present
- [~] `SDCppGenerator` thread-safety audit for parallel calls — opt-in target present

### Phase 7 — PluginManager Hot-Plug Integration ✅ (v2.3.0)
- [x] `SDPluginAdapter : IThemisPlugin` — wraps `SDPlugin`, implements `initialize(config_json)`, `shutdown()`, `getType()`, `getCapabilities()`, `getInstance()`; `PluginType::IMAGE_GENERATION`
- [x] `SDPluginRegistrar` — `createPlugin()`, `createAdapter()`, `defaultReloadCallback()`, `enableHotPlug()`, `disableHotPlug()`
- [x] 12 unit tests (`SDPluginRegistrarTests`, groups A–D) in `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`

## Known Issues & Limitations

- `SDCppGenerator` requires the `stable-diffusion.cpp` submodule and `THEMIS_ENABLE_STABLE_DIFFUSION=ON`;
  without them `SDStubGenerator` is used automatically.
- `SDPlugin` thread-safety applies to individual method calls; external callers must not
  share an `SDPlugin` instance across threads without external synchronisation for
  `initialize()` calls.
- `SDCppGenerator` parallel-call safety depends on stable-diffusion.cpp's own thread model;
  an opt-in real-backend parallel-generation test exists, but this document has no evidence
  that it has been run against a configured deployment model/runtime.
- Generated `DOXYGEN.md` (2026-09-20) and `MODULE_GAPS.md` (2026-06-04) are stale snapshots.
  **TODO:** regenerate them with their documented tools and revalidate their findings; Doxygen
  was unavailable during this documentation audit.

## Breaking Changes

None (v2.0.0 is the initial release).

## Historical Unused-Functions Snapshot

The 2026-04-20 [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md) is historical
and is not a current usage inventory.
Current source defines and uses `samplerFromString` in `include/stable_diffusion/sd_generator.h`;
the plugin and registrar are also present in source and CMake registration.
**TODO:** refresh `src/UNUSED_FUNCTIONS_REPORT.md` before using its classifications for cleanup.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `stable_diffusion`
- [!] TODO: confirm which Wave-D tracing/exporter/operator-remediation requirements apply; the
  inspected module source does not establish those capabilities. The scoped high-cardinality
  stress test uses in-process simulations and does not exercise `SDPlugin`.
- [ ] Provide or link long-duration soak evidence for the module's actual generation paths
  (Target: Q1 2027); no such evidence was found in the scoped tests.
- [!] TODO: link an operator runbook for module-critical scenarios or confirm that none is required.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [!] TODO: confirm applicability of distributed/acceleration fail-closed criteria; no such
  runtime path is evidenced by the scoped Stable Diffusion source/header set.
- [ ] Publish representative-hardware p95/p99 measurements; `benchmarks/stable_diffusion/BASELINE.md`
  currently records pending runs, not published measurements (Target: Q1 2027).
- [!] TODO: provide evidence or confirm applicability for diagnostics, alerts, and operator runbooks.
