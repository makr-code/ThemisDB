---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Related_Issue: "#7429"
---

> Exact model backend not verifiable from final document text alone.

# llama_cpp Plugin Roadmap

The roadmap records module work against the current implementation in
`src/llama_cpp/llama_cpp_plugin.cpp` and `llama_cpp_registrar.cpp`. The plugin
reports version `2.1.0`; its advertised capabilities and loaded flag do not prove
that a real model backend is usable. Production gates are in
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).

## Current Status

The module implements the `ILLMPlugin` interface, with optional real inference
through `LlamaWrapper` when the parent build enables `THEMIS_ENABLE_LLM`
(`THEMIS_LLM_ENABLED` in the plugin). The module CMake target is a static C++17
library. `THEMIS_LLAMA_CPP_STUB_MODE` is used by focused test targets; it is not a
production backend.

`loadModel()` can return success when wrapper initialization or model loading
failed, leaving the wrapper unavailable while the plugin's loaded flag is true.
Consequently, a real inference probe is required for deployment readiness.

## v2.5.0 → v3.0.0 Research & Wave Plan

This plan is a research-backed proposal, not implementation approval or release
sign-off. The current maturity is **integration with production-hardening gaps**:
the plugin contract, wrapper bridge, RAG assembly, streaming and focused tests
exist, but load readiness, real-backend benchmark evidence, and operational
metrics do not yet meet a release-grade proof standard. Source review is current
to 2026-11-06; module source and tests take precedence over generated wiki
summaries.

### Evidence baseline and research rationale

| Dimension | Evidence-backed current state | Implication |
|---|---|---|
| Performance and scale | `generateBatch()` is sequential; `loadModel()` explicitly disables wrapper continuous batching and response caching. The wrapper already declares continuous-batch scheduling and paged-KV components, but plugin activation and measured benefit are not established. The benchmark target guard checks the wrong source path, so target registration may be skipped. | First repair measurement and capture real-backend baselines; then evaluate concurrency features without treating upstream results as ThemisDB results. |
| Use cases and usability | The plugin adapts `ILLMPlugin` and provides generation, RAG, embedding, streaming, and LoRA entry points. Capability flags are static; `embed()` can return 384 zeroes and those are not semantic vectors. | Make usable-backend state and fallback behavior explicit before production callers rely on advertised capabilities. |
| Security and resilience | `loadModel()` can report success when the wrapper did not load. The plugin's optional FNV-64 check is not cryptographic; `LlamaWrapper` separately supports SHA-256 checks, but missing checksums are allowed unless integrity is required. Caller identity, quotas, timeouts, and tenant controls remain outside this plugin. | Fail closed on backend and integrity failures, and preserve the host/deployment boundary for access and resource policy. |
| Governance and maintainability | Focused tests compile stub-mode plugin paths; the integration soak uses in-process simulations. The benchmark source includes real-model fixtures, but its registration guard and source-reported counters do not establish production performance. | Label stub, callback, and real-wrapper evidence separately, retain reproducible artifacts, and do not promote readiness without real-model validation. |
| Architecture and operations | Plugin statistics are counters, not latency histograms; no plugin queue, admission, or rollback API is implemented. Wrapper options exist but continuous batching is disabled at the plugin boundary. | Keep host-owned operations outside the plugin; expose only the minimum runtime state needed to integrate measurable serving and bounded execution. |

Primary repository evidence: [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp),
[`llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h),
[`llama_wrapper.h`](../../include/llm/llama_wrapper.h),
[`llama_wrapper.cpp`](../llm/llama_wrapper.cpp),
[`tests/llama_cpp/CMakeLists.txt`](../../tests/llama_cpp/CMakeLists.txt),
[`bench_llama_cpp_inference.cpp`](../../benchmarks/llama_cpp/bench_llama_cpp_inference.cpp),
and [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt).
The benchmark guard at `benchmarks/CMakeLists.txt` currently checks for
`bench_llama_cpp_inference.cpp` at the directory root, while the actual file is
under `benchmarks/llama_cpp/`.

Research sources consulted 2026-11-06:

- Kwon et al., *Efficient Memory Management for Large Language Model Serving
  with PagedAttention*, SOSP 2023
  ([arXiv:2309.06180](https://arxiv.org/abs/2309.06180)). The paper reports
  2–4× throughput over its evaluated serving systems; this result motivates
  measuring KV-cache allocation and concurrency but is **not** a performance
  prediction for llama.cpp or this plugin.
- Leviathan et al., *Fast Inference from Transformers via Speculative
  Decoding*, ICML 2023 ([arXiv:2211.17192](https://arxiv.org/abs/2211.17192)).
  Speculative decoding can preserve the target distribution while reducing
  decoding latency; gains depend on draft acceptance and the workload/hardware.
  The plugin's draft-token and target-logit APIs therefore need parity and
  workload-specific tests, not assumed speedup.
- The upstream [llama.cpp speculative decoding guide](https://github.com/ggml-org/llama.cpp/blob/master/docs/speculative.md)
  documents draft-based methods and warns that synthetic acceptance produces
  invalid model output. Use only real verification for quality and release
  evidence. The upstream [llama.cpp README](https://github.com/ggml-org/llama.cpp)
  also documents multiple hardware backends and quantization choices, reinforcing
  the need to identify backend, model artifact, and hardware in every comparison.
- OWASP, [Top 10 for LLM Applications 2025](https://owasp.github.io/www-project-top-10-for-large-language-model-applications/assets/PDF/OWASP-Top-10-for-LLMs-v2025.pdf),
  identifies supply-chain, embedding, and unbounded-consumption risks. These
  support artifact-integrity, embedding-fallback, and resource-bound validation;
  they do not assign security responsibilities to this plugin that currently
  belong to the serving/deployment layer.

### v2.5.0 — production-contract and evidence closure

Scope: stabilize current interfaces and establish trustworthy release evidence;
do not introduce a new scheduler or claim new model capability. Estimates are
person-weeks for design, implementation, focused validation, and documentation;
they exclude hardware provisioning and upstream dependency changes.

#### Wave 1 — Reproducible benchmark baseline (P0)

- **Goal:** Make the module benchmark discoverable and distinguish real inference
  from stub, callback, and registry-only measurements.
- **Tasks:**
  - [ ] Correct the benchmark target existence check and verify that the
    configured build creates `bench_llama_cpp_inference` (Target: v2.5.0).
  - [ ] Record model digest, build/backend flags, hardware, context and generation
    settings, warmup, and workload in each real-backend result artifact
    (Target: v2.5.0).
  - [ ] Measure cold/warm load, TTFT, decode tokens/s, end-to-end latency,
    embedding validity, and peak RAM/VRAM; label stub/callback results separately
    (Target: v2.5.0).
- **Dependencies:** None; requires a controlled model artifact and representative
  CPU/GPU runner for real-backend results.
- **Effort:** 1–2 person-weeks.
- **Risks / trade-offs:** Hardware variance and model licensing/access can delay
  baselines; empty-path benchmarks remain useful only as plugin-overhead tests.
- **Success metrics:** Target is registered in the configured benchmark build;
  every real result carries all required environment/model fields; collect at
  least 100 measured requests after warmup for p50/p95, and report p99 only with
  at least 1,000 observations. If repeated p95 spread exceeds 10% on a pinned
  runner, mark the result inconclusive and investigate rather than claim a pass.
- **Validation:** Build the benchmark target, run its real-model fixture on a
  controlled runner, export benchmark JSON plus manifest, and verify CTest and
  benchmark output do not count synthetic counters as real inference evidence.

#### Wave 2 — Truthful lifecycle and fallback contracts (P0)

- **Goal:** A production plugin must not report a usable model when no real
  backend is available; synthetic behavior remains test-only.
- **Tasks:**
  - [ ] Define and implement backend-ready load semantics without changing
    `ILLMPlugin` accidentally; preserve explicit stub-mode test behavior
    (Target: v2.5.0).
  - [ ] Make wrapper load exceptions and unavailable-backend failures observable
    as structured failures; align `isModelLoaded()`, model info, and capabilities
    with their documented meaning (Target: v2.5.0).
  - [ ] Specify and test a production embedding failure contract so the 384-zero
    fallback cannot be mistaken for a semantic embedding (Target: v2.5.0).
  - [ ] Add lifecycle, reload, cancellation, and embedding-failure tests for
    success, failure, and degraded-state transitions (Target: v2.5.0).
- **Dependencies:** Wave 1 baseline harness; public-contract review if an API
  compatibility change is needed.
- **Effort:** 2–4 person-weeks.
- **Risks / trade-offs:** Existing integrations may rely on optimistic load
  results or zero-vector behavior; retain compatibility only through an explicit,
  documented migration and do not weaken the production fail-closed invariant.
- **Success metrics:** Zero false-ready states in focused live-wrapper failure
  tests; 100% of unavailable-backend cases fail visibly; no production-success
  path returns the zero-vector fallback as a valid embedding.
- **Validation:** Run `ctest --preset windows-release -R 'llama_cpp'
  --output-on-failure`; add a real-wrapper probe with a missing/corrupt model and
  confirm load, generation, and embedding outcomes are consistent.

#### Wave 3 — Artifact integrity and deployment acceptance (P1)

- **Goal:** Require trusted model bytes for deployments that opt into
  integrity enforcement and provide auditable, fail-closed acceptance evidence.
- **Tasks:**
  - [ ] Prefer the existing `LlamaWrapper` SHA-256 verification path and
    `require_model_integrity` configuration over the plugin's FNV-64 helper;
    reject missing required checksums and mismatches before model use
    (Target: v2.5.0).
  - [ ] Document that checksums establish byte equality only when the expected
    value comes from a trusted deployment channel; do not describe them as
    signatures or provenance by themselves (Target: v2.5.0).
  - [ ] Add tamper, missing-checksum, invalid-checksum-type, and real-model
    acceptance tests; verify sanitized diagnostics and deployment-owned rollback
    evidence (Target: v2.5.0).
- **Dependencies:** Wave 2 load semantics; deployment owner supplies checksum
  provenance and test artifact.
- **Effort:** 2–3 person-weeks.
- **Risks / trade-offs:** Requiring integrity may make artifact rollout fail
  closed when manifests are stale; SHA-256 cannot establish who approved a model
  without a trusted manifest/signature chain.
- **Success metrics:** 100% of tampered or missing-required-checksum test cases
  reject before inference; all production probes identify artifact digest,
  policy integration, and rollback owner; prompts and model content are absent
  from routine logs.
- **Validation:** Focused integrity tests plus a deployment rehearsal using the
  exact approved artifact; verify failure and rollback procedures without
  bypassing policy.

### v3.0.0 — measured concurrency and capability evolution

Scope: build on the v2.5.0 evidence gates. Changes to public API or ABI require
separate compatibility review; no performance technique is enabled by default
until its resource and correctness behavior passes the relevant wave.

#### Wave 4 — Bounded concurrent inference (P1)

- **Goal:** Evaluate plugin-level access to the wrapper's existing continuous
  batch, scheduler, and paged-KV facilities instead of treating sequential
  `generateBatch()` as parallel batching.
- **Tasks:**
  - [ ] Design an opt-in, bounded concurrency contract for queue capacity,
    admission/backpressure, ordering, timeout, cancellation, and unload/reload
    behavior; retain sequential API semantics unless explicitly versioned
    (Target: v3.0.0).
  - [ ] Wire supported wrapper scheduler settings through the plugin only after
    validating ownership, thread-safety, and GPU/CPU backend behavior
    (Target: v3.0.0).
  - [ ] Add mixed prompt lengths, queue saturation, cancellation, reload, and
    sustained-load integration tests (Target: v3.0.0).
- **Dependencies:** Waves 1–3; wrapper scheduler capability and supported backend
  matrix confirmed by source/build evidence.
- **Effort:** 4–8 person-weeks.
- **Risks / trade-offs:** Higher aggregate throughput can increase single-request
  tail latency and memory use; scheduler implementation and fairness may vary by
  backend. Authentication, quotas, and tenant isolation stay with the host.
- **Success metrics:** On the Wave 1 pinned workload, target at least 20% higher
  aggregate tokens/s at eight concurrent requests than the sequential baseline,
  with single-request p95 latency regression no greater than 10%; queue memory
  remains within a declared cap and overload is rejected or backpressured, never
  unbounded. These are proposed gates to confirm or revise from Wave 1 evidence
  before implementation.
- **Validation:** Run the same real model/hardware workload at concurrency 1, 4,
  8, and 32; publish p50/p95/p99, throughput, memory, errors, and queue behavior;
  run sanitizer and sustained-load tests on supported backends.

#### Wave 5 — Research-backed decoding and cache experiments (P2)

- **Goal:** Promote speculative decoding and KV/prefix-cache options only for
  workload/backend combinations where correctness, quality, and resource cost
  are measured.
- **Tasks:**
  - [ ] Benchmark the existing draft-token/target-logit bridge against ordinary
    decoding with compatible target/draft artifacts and fixed sampling settings
    (Target: v3.0.0).
  - [ ] Verify target-logit shape, token/vocabulary bounds, cancellation, and
    exact output-distribution or deterministic-greedy parity as applicable
    (Target: v3.0.0).
  - [ ] Measure prefix/KV-cache hit rate, memory overhead, isolation, and
    invalidation across model reload and request boundaries before enabling reuse
    (Target: v3.0.0).
- **Dependencies:** Waves 1 and 2; Wave 4 for shared-server workloads; compatible
  draft artifacts and an approved evaluation workload.
- **Effort:** 3–5 person-weeks.
- **Risks / trade-offs:** Draft-model memory and low acceptance can erase gains;
  cache reuse can create isolation or staleness risks; upstream llama.cpp results
  do not guarantee this wrapper integration will benefit.
- **Success metrics:** Enable an optimization only when a named workload shows
  at least 10% decode-throughput or TTFT improvement over its baseline, with
  output parity/quality acceptance met and peak memory below the declared cap;
  otherwise retain the feature disabled.
- **Validation:** Run paired baseline/optimized real-model tests with fixed
  seeds where deterministic, report output/parity checks and confidence
  distributions, and include cache-isolation and reload invalidation tests.

#### Wave 6 — Operations, governance, and release handoff (P1)

- **Goal:** Make real-backend behavior supportable without moving service-level
  security or lifecycle responsibilities into the plugin.
- **Tasks:**
  - [ ] Expose or integrate bounded latency/error/resource measurements needed by
    the host to report load, queue, inference, and fallback health
    (Target: v3.0.0).
  - [ ] Define backend capability reporting as runtime state distinct from
    static interface support; document ownership of authentication, policy,
    quotas, timeout, tenant boundaries, and rollback (Target: v3.0.0).
  - [ ] Publish a versioned backend compatibility matrix, model/artifact
    acceptance record, and upgrade/rollback checklist (Target: v3.0.0).
- **Dependencies:** Waves 1–5 and host observability/security owners.
- **Effort:** 2–4 person-weeks.
- **Risks / trade-offs:** Duplicate telemetry or policy in the plugin and host
  can create inconsistent controls; keep new APIs narrow and review ABI impact.
- **Success metrics:** Every supported backend has a passing real-model
  conformance report, required telemetry is available to the host without prompt
  disclosure, and all release-critical failure/rollback scenarios have
  auditable pass evidence or an explicitly approved exception.
- **Validation:** End-to-end test with serving-layer policy and resource limits,
  security review, docs/link checks, focused tests, benchmark comparison, and
  human release sign-off.

### Cross-wave KPIs and explicit assumptions

| KPI | v2.5.0 gate | v3.0.0 gate |
|---|---|---|
| Backend readiness | No false-ready model states; unavailable real backend fails visibly. | Runtime readiness and static capability advertisement are separately observable. |
| Benchmark provenance | Every real result identifies artifact, build, backend, hardware, configuration, workload, and run distribution. | Paired before/after results use the same pinned workload and backend. |
| Performance | Controlled p50/p95 baseline; p99 only with at least 1,000 observations; noisy results are inconclusive. | Proposed concurrency gain ≥20% aggregate tokens/s at 8 requests and ≤10% single-request p95 regression, subject to Wave 1 re-baselining. |
| Embeddings and decoding | No zero-vector fallback is accepted as semantic output; all enabled real paths have validity/parity tests. | Cache/speculative optimizations meet stated speed, quality/parity, and memory gates or remain disabled. |
| Security and operations | Required SHA-256 mismatch/missing-evidence tests fail closed; host responsibilities and rollback are documented. | Bounded overload behavior, backend conformance, sanitized telemetry, security review, and release evidence are complete. |

Open assumptions and knowledge gaps:

- No model artifact, deployment workload, or representative hardware matrix was
  specified by issue #7429. Absolute latency and memory targets must be set after
  Wave 1 selects these inputs; the relative v3.0.0 thresholds above are proposed
  starting gates, not current measurements.
- The upstream llama.cpp docs and `ai_context/developer_llm_wiki/WIKI_STATUS.json`
  are mutable/generated context. The wiki status is dated 2026-11-02, has a
  large source delta, and the curated module index has no llama_cpp page; verify
  claims against local code, tests, and current upstream documentation before
  implementation.
- Wrapper-level scheduler and cache components exist in source, but this roadmap
  does not establish that each is production-ready, thread-safe in this plugin,
  or supported by every build backend.
- A SHA-256 match is not a trusted signature or model-quality guarantee. Trusted
  artifact provenance, host authentication, authorization, tenant isolation,
  rate limits, output safety, and rollback remain deployment/service concerns.
- Proposed effort estimates are planning ranges, not commitments; re-estimate
  after hardware access, API compatibility review, and benchmark registration
  validation.

## In Progress

None currently tracked.

## Planned Features

None currently approved. Record new implementation work only after its interfaces,
failure behavior, validation evidence, and measurable performance target (where
applicable) are defined.

## Implementation Phases

### Phase 1 — Design / API Contract

- [x] Implement the `ILLMPlugin` contract in `include/llama_cpp/llama_cpp_plugin.h`.
- [x] Define registrar creation and server-config integration in
  `include/llama_cpp/llama_cpp_registrar.h`.

### Phase 2 — Core Implementation

- [x] Implement optional wrapper-backed generation and embeddings behind
  `THEMIS_LLM_ENABLED`.
- [x] Implement RAG context assembly through `RAGContextAssembler`.
- [x] Implement plugin registration, generation callbacks, sequential batching,
  capability reporting, and performance counters.

### Phase 3 — Error Handling & Edge Cases

- [x] Return structured generation failures for policy denial, pre-dispatch
  cancellation, unloaded state, and unavailable backends in non-stub builds.
- [x] Bound LoRA import size and check GGUF magic before wrapper delegation.
- [x] Keep embedding fallback behavior and wrapper-load limitations explicit in
  the module contract.

### Phase 4 — Tests

- [x] Provide focused lifecycle, inference-contract, registrar, and validation
  tests under `src/llama_cpp/tests/`.
- [x] Provide focused model-loading, inference, and stress tests under
  `tests/llama_cpp/`.
- [x] Register llama_cpp CTest targets in `tests/llama_cpp/CMakeLists.txt`.

### Phase 5 — Performance / Hardening

- [x] Expose inference, error, and stream-retry counters.
- [x] Add the module inference benchmark at
  `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`.
- [x] Keep batch execution sequential and document that it is not parallel
  batching; validate production performance separately with a real model.

### Phase 6 — Documentation & Acceptance

- [x] Align the README, architecture, roadmap, and production requirements with
  the live implementation.
- [x] Record test-fixture, fallback, validator, and deployment-readiness limits.
- [x] Provide focused CTest and direct Doxygen validation commands.

## Production Readiness Checklist

- [x] Implement model lifecycle and runtime configuration in `LlamaCppPlugin`.
- [x] Provide wrapper-backed generation and embedding when the LLM backend is
  enabled and a model successfully loads.
- [x] Apply an installed `PolicyFn` to generation and RAG dispatch.
- [x] Bound and format-check imported LoRA bytes before optional wrapper
  delegation.
- [x] Provide focused module tests and a module inference benchmark.
- [ ] Verify a real-backend inference and embedding probe for each deployed model
  and configuration before enabling production traffic.
- [ ] Validate model provenance with deployment-controlled cryptographic
  evidence; the plugin's FNV-64 check is not an authenticity control.

The checked implementation items do not constitute production sign-off. See
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for deployment gates.

## Known Issues & Limitations

- `loadModel()` may return `true` after a wrapper load failure; `model_loaded_`,
  `getModelInfo()`, and static capability flags are not readiness signals.
- Without a live wrapper or injected `GenerateFn`, generation fails in normal
  builds. The synthetic generation response is limited to builds defining
  `THEMIS_LLAMA_CPP_STUB_MODE`.
- `embed()` may return a 384-element zero vector after wrapper or callback
  failure; this is not a semantic embedding.
- The LoRA registry stores metadata. In a no-wrapper path, successful payload
  checks do not load adapter weights.
- `generateBatch()` processes requests sequentially and preserves their order.
- The opt-in model digest uses FNV-64, not a cryptographic digest. The separate
  validation helpers are not automatically invoked by `loadModel()`.
- Focused test fixtures and the integration soak do not establish production
  model quality, throughput, or load reliability.

## Breaking Changes

No public API change is introduced by this documentation update. The plugin
currently reports version `2.1.0` through its capability and version methods.
