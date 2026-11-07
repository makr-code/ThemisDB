---
Author: "ThemisDB Team"
Created: "2026-11-07"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Toolbox Module Roadmap

**Scope:** Research-backed planning for `src/toolbox/`, `include/toolbox/`, `tests/toolbox/`, and `benchmarks/toolbox/`. This roadmap is a proposal, not a release commitment or evidence of readiness.

**Canonical implementation plan:** this file. See [Future Enhancements](FUTURE_ENHANCEMENTS.md), [Architecture](ARCHITECTURE.md), [Research and evaluation](RESEARCH.md), [Performance evidence](PERFORMANCE_EXPECTATIONS.md), [tests](../../tests/toolbox/README.md), and [benchmarks](../../benchmarks/toolbox/README.md) for source-grounded details.

## Current Status

**Assessment:** toolbox has a useful, integrated set of ingestion and text-processing facilities, but the available test and benchmark evidence does not establish a certified production or release-readiness level. Plan it as a mature feature surface with material evaluation, bounds, and evidence gaps—not as a measured high-scale platform.

| Area | Evidence in the current tree | Gap / interpretation |
|---|---|---|
| Orchestration and integration | [`ingestion_toolbox.cpp`](ingestion_toolbox.cpp) delegates to `ingestion::WorkflowEngine`; [`content_toolbox_bridge.cpp`](content_toolbox_bridge.cpp) integrates with `ContentManager` and optional writers. See [architecture](ARCHITECTURE.md). | A successful/empty result does not always distinguish no match from failure. Optional writer failures can coexist with `BridgeResult::ok`. End-to-end behavior depends on injected workflows and sinks. |
| Text helpers | [`text_chunker.cpp`](text_chunker.cpp) delegates to `rag::DocumentSplitter`; [`text_normalizer.cpp`](text_normalizer.cpp) folds selected German characters; [`language_detector.cpp`](language_detector.cpp) uses English/German stopword heuristics; quality signals are heuristic; [`content_fingerprinter.cpp`](content_fingerprinter.cpp) hashes input bytes with SHA-256. See the [public header map](../../include/toolbox/README.md). | These APIs do not promise general Unicode normalization, broad language identification, calibrated quality, model-token counts, or semantic deduplication. Empty input has no valid fingerprint. |
| API and application fit | [`toolbox_builder.cpp`](toolbox_builder.cpp), [`toolbox_registry.cpp`](toolbox_registry.cpp), [`toolbox_composite.cpp`](toolbox_composite.cpp), and [`toolbox_streaming.cpp`](toolbox_streaming.cpp) implement builder, process-global access, MIME-prefix routing, and callback-based streaming. | The registry is process-global; routing order is insertion-based; streaming is synchronous. Limits, cancellation, backpressure, and multi-tenant isolation need explicit evaluation. |
| Tests | Six direct test sources are registered by [`tests/toolbox/CMakeLists.txt`](../../tests/toolbox/CMakeLists.txt), including [primitive](../../tests/toolbox/test_toolbox_primitives.cpp), [ingestion](../../tests/toolbox/test_toolbox_ingestion.cpp), [contract](../../tests/toolbox/test_toolbox_contract_hardening_focused.cpp), and [stress](../../tests/toolbox/test_toolbox_highcardinality_stress.cpp) cases. | Test presence is not coverage or release evidence. Some stress fixtures use stubs/simulations; no broad labeled text-quality corpus is documented. |
| Performance | [`benchmarks/toolbox/CMakeLists.txt`](../../benchmarks/toolbox/CMakeLists.txt) registers three targets. [`bench_toolbox_native_workloads.cpp`](../../benchmarks/toolbox/bench_toolbox_native_workloads.cpp) measures selected helper/orchestration calls. | [`bench_toolbox_dedicated_gates.cpp`](../../benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp) measures stubs; the bridge proxy is not `ContentToolboxBridge`; release-gates do not enforce historical thresholds. No current reproducible baseline or certified p95/p99 envelope is available. See [benchmark evidence limits](PERFORMANCE_EXPECTATIONS.md). |
| Security and operations | [Security notes](SECURITY.md) describe content/sink boundaries and warn that helper signals are not security controls. | Input/resource bounds and error/sink outcome distinctions require stronger contract tests. Toolbox metrics do not prove every helper incident is counted. |

### Research findings and implications

1. Unicode normalization is a defined, testable transformation; Unicode UAX #15 describes normalization forms and conformance. The current German umlaut folding is a narrower transliteration helper. Specify the exact intended equivalence and UTF-8/error policy before changing it; do not silently use compatibility normalization where byte identity, signatures, or identifiers matter. ([Unicode UAX #15](https://www.unicode.org/reports/tr15/))
2. Language identification quality depends on sample length, domain, and language coverage. Jauhiainen et al. evaluate out-of-domain identification across 285 languages and report that success on fewer languages does not necessarily scale. Evaluate the currently supported EN/DE heuristic on a labeled, in-scope corpus and measure short/mixed/unsupported input separately; do not extrapolate to general LID. ([Jauhiainen et al., NoDaLiDa 2017](https://aclanthology.org/W17-0221/))
3. FastCDC research improves content-defined chunking for data deduplication, but that objective is not equivalent to semantic or token-based document chunking. Keep `TextChunker` aligned with the RAG splitter unless a measured use case justifies another strategy and establishes retrieval-quality parity. ([Xia et al., USENIX ATC 2016](https://www.usenix.org/conference/atc16/technical-sessions/presentation/xia))
4. Microbenchmark output must not be mistaken for service-tail latency. Use repeated, controlled runs for helper comparisons, and a request-level integration harness for p95/p99 bridge/extraction latency. Record build, host, input corpus, repetitions, and raw output; Google Benchmark documents repetitions and random interleaving as tools for reducing run variability, not as proof of production performance. ([Google Benchmark User Guide](https://google.github.io/benchmark/user_guide.html))

## Priorities by Release

### v2.5.0 — compatible evidence and hardening

Prioritize explicit helper contracts, bounded deterministic behavior, real-path validation, and trustworthy measurements. Preserve current public behavior by default; add APIs only where needed and keep any stricter behavior opt-in until compatibility is assessed.

- [ ] Establish reproducible helper and integration baselines, including real bridge and extraction paths; replace stub-derived claims with source-backed results. (Target: v2.5.0)
- [ ] Define and test supported UTF-8, normalization, language-detection, and empty-fingerprint behavior against labeled/adversarial inputs. (Target: v2.5.0)
- [ ] Bound and test large-input, streaming, workflow, and optional-sink failure behavior; make partial outcomes observable. (Target: v2.5.0)
- [ ] Publish per-workload result artifacts and thresholds only after representative baselines are collected and reviewed. (Target: v2.5.0)

### v3.0.0 — explicit, composable contracts

Treat these as design proposals requiring API review, migration planning, and compatibility tests before implementation.

- [ ] Design explicit result states for empty/no-match, extraction failure, and per-sink success/failure; specify retry and idempotency behavior. (Target: v3.0.0)
- [ ] Design an instance-scoped toolbox context and explicit resource, timeout/cancellation, and streaming/backpressure contracts; assess migration from process-global access. (Target: v3.0.0)
- [ ] Version text-processing and fingerprint semantics; keep raw-byte identity distinct from any future normalized-content identity. (Target: v3.0.0)
- [ ] Evaluate whether to decouple public toolbox headers from RAG implementation types, with measured integration cost and an API migration plan. (Target: v3.0.0)

## Wave Plan

Effort estimates are engineering effort, not calendar commitments. Dependencies refer to the prior wave unless stated otherwise. Numeric thresholds below are proposed acceptance targets; freeze them against the documented workload before treating them as release gates.

### Wave 1 — Evidence baseline (v2.5.0, 1–2 engineering weeks)

- **Goal:** Replace historical/unverified performance and quality assertions with reproducible, source-grounded evidence.
- **Tasks:** Inventory production call paths and test registrations; define a versioned EN/DE and short/mixed/unsupported labeled evaluation set; benchmark helpers separately from workflow and bridge; record revision, build flags, compiler, host, input sizes, repetitions, raw output, and analysis.
- **Dependencies:** None; use current APIs and existing benchmark/test targets.
- **Risk / trade-off:** Representative content and hardware may be unavailable; synthetic fixtures can overstate real-world results. Keep synthetic and production-path results clearly separated.
- **Success metrics:** At least three workloads cover a helper, full extraction, and the real content bridge; each maps to the production symbol it measures. Capture 30 independent runs for helper baselines and at least 10,000 request-level observations per integration workload before reporting p99. No stub/simulation result is labeled a production measurement; record medians, dispersion, and raw artifacts.
- **Validation:** Build and run the focused toolbox tests; run native benchmarks on a recorded host; verify source-to-symbol mappings and artifact metadata. The current dedicated-gates benchmark is not accepted as production-path evidence.

### Wave 2 — Bounded helper contracts (v2.5.0, 2–3 engineering weeks)

- **Goal:** Make helper behavior deterministic and explicit at Unicode, language, and resource boundaries without silently changing existing semantics.
- **Tasks:** Specify valid/malformed UTF-8 handling and exact normalization behavior; add composed/decomposed and non-German Unicode cases; evaluate EN/DE language results across sample lengths, mixed text, and unsupported text; define safe maximum input/work and overflow behavior for helper entry points.
- **Dependencies:** Wave 1 corpus and behavior inventory.
- **Risk / trade-off:** Tightening validation may reject inputs previously processed; normalization changes can alter hashes and identifiers. Keep transliteration separate from identity/security decisions and document any opt-in limits.
- **Success metrics:** 100% of documented boundary cases have deterministic tests; for the agreed in-scope EN/DE corpus (plain text, at least 100 characters), proposed macro-F1 target is ≥0.95, with short, mixed, and unsupported cases reported separately. If the threshold is not met, document measured limits rather than claiming broad LID support.
- **Validation:** Run primitive and binary-fingerprint focused tests; add corpus-based regression and malformed-input tests; compare normalization against the declared contract and Unicode conformance data where applicable.

### Wave 3 — Real-path reliability and performance (v2.5.0, 2–4 engineering weeks)

- **Goal:** Validate real extraction, bridge, streaming, and optional-writer behavior under load and partial failure.
- **Tasks:** Add/extend tests with real configured workflows and sinks; distinguish empty extraction from workflow/sink failure in diagnostics without changing success semantics implicitly; exercise large documents, concurrent calls, and slow/failing callbacks; benchmark the actual `ContentToolboxBridge` path and extraction path.
- **Dependencies:** Waves 1–2; integration fixtures and a testable content/workflow setup.
- **Risk / trade-off:** End-to-end measurements include dependent modules and host configuration; isolate and label toolbox-only, integration, and system costs.
- **Success metrics:** Every integration benchmark calls the named production path and records setup costs; all injected failure cases have an asserted observable outcome; after a baseline exists, median regression stays within a reviewed 10% budget on the same host/configuration. Do not infer p99 from aggregate microbenchmark results.
- **Validation:** Run ingestion, contract-hardening, and stress focused suites; run integration benchmark with repeated request-level samples; execute available sanitizer/fault-injection checks and retain raw results.

### Wave 4 — v3 API and migration design (v3.0.0, 2–3 engineering weeks)

- **Goal:** Agree on explicit outcome, lifecycle, and resource contracts before a major-version implementation.
- **Tasks:** Specify typed extraction/bridge outcomes and per-sink status; define timeout, cancellation, input-size, and callback/backpressure semantics; design an instance-scoped context and compatibility/migration mapping from global registry calls; document fingerprint and text-normalization versioning.
- **Dependencies:** Waves 1–3 evidence; maintainer approval of the breaking API direction.
- **Risk / trade-off:** More explicit types and instance ownership improve isolation but require caller migration and can enlarge the API surface.
- **Success metrics:** Reviewed API proposal covers success, empty/no-match, partial sink failure, cancellation, timeout, invalid input, and retry/idempotency; migration examples exist for every affected public entry point; no behavior is marked stable without tests.
- **Validation:** Compile-only consumer examples and API contract tests; compatibility analysis of in-repository call sites; design review against `VERSIONING.md` and public-header documentation requirements.

### Wave 5 — v3 composable implementation (v3.0.0, 3–6 engineering weeks)

- **Goal:** Implement only the approved v3 contract and reduce hidden global/dependency coupling.
- **Tasks:** Introduce the reviewed context/result contracts; migrate callers from process-global state where approved; enforce configured resource and cancellation limits; preserve RAG-owned chunking behavior unless Wave 1 provides evidence and a separately reviewed design supports change.
- **Dependencies:** Wave 4 approval; dependent ingestion, content, and RAG APIs.
- **Risk / trade-off:** Cross-module migration and changed error semantics can break callers or duplicate work if retries occur after partial writes.
- **Success metrics:** All migrated call sites compile; tests prove no cross-context state leakage; limits and cancellation are deterministic; integration outcomes match the published per-sink and retry contracts.
- **Validation:** Full toolbox focused suite plus affected integration suites; API migration tests; concurrent multi-context, slow-sink, cancellation, and partial-write fault tests.

### Wave 6 — v3 scale, security, and release evidence (v3.0.0, 2–4 engineering weeks)

- **Goal:** Demonstrate bounded operation and maintainable evidence before v3 release claims.
- **Tasks:** Run long-document/high-cardinality and concurrency tests; fuzz parsing/UTF-8 boundaries where an existing harness permits; check diagnostics for content/secret leakage; publish real-path benchmark artifacts and versioned test corpus; synchronize runbook and module docs.
- **Dependencies:** Wave 5 implementation; representative CI and benchmark hardware.
- **Risk / trade-off:** Stress and hardware-specific thresholds can be costly and may not transfer across platforms; set per-platform baselines instead of a misleading universal number.
- **Success metrics:** Under the agreed maximum-input/concurrency profile, memory and latency remain within the recorded v3 acceptance envelope and show no growth across the soak interval; zero sanitizer-reported defects; zero raw document content or secret-bearing configuration in sampled toolbox diagnostics; benchmark artifacts identify exact source, revision, host, and workload; project-required release gates remain green.
- **Validation:** Focused and integration tests, available ASan/UBSan/TSan/fuzz checks, security review of logs and failure paths, and root release-gate checks on the governed branch.

## Implementation Phases

1. **Phase 1 — Design / API contract:** Agree the v2.5 compatibility boundary and Wave 1 corpus/benchmark protocol; review the v3 result, context, timeout, and cancellation contracts in Wave 4 before code.
2. **Phase 2 — Core implementation:** Add v2.5 evidence harnesses and bounded helper behavior; implement v3 APIs only after Wave 4 sign-off.
3. **Phase 3 — Error handling and edge cases:** Specify empty versus failed extraction, malformed/oversized text, mixed languages, sink partial failure, slow callbacks, cancellation, and retry behavior.
4. **Phase 4 — Tests:** Add corpus-backed helper tests and real-path integration/fault tests; keep stub-based stress fixtures labeled as simulations.
5. **Phase 5 — Performance and hardening:** Establish baselines before thresholds; separate microbenchmarks from request-level p95/p99; run concurrency, resource-bound, and available sanitizer/fuzz checks.
6. **Phase 6 — Documentation and acceptance:** Update API/migration docs for any public change; publish reproducible artifacts and residual risks; require root governance/release gates before any readiness claim.

## Production Readiness Checklist

- [ ] Every public behavior in scope has a source-backed contract and tests for success, empty, invalid, and failure cases.
- [ ] EN/DE language quality is measured on a labeled, versioned in-scope corpus; unsupported and short inputs are reported separately.
- [ ] Normalization semantics and UTF-8 failure policy are explicit; fingerprints are not described as authentication or semantic deduplication.
- [ ] Integration benchmarks exercise production bridge/extraction paths; stub/simulation measurements are never substituted for them.
- [ ] Performance claims include raw artifacts and reproducible host/build/workload metadata; tail latency comes from request-level samples.
- [ ] Resource bounds, partial sink outcomes, streaming behavior, and diagnostic privacy are tested.
- [ ] Breaking v3 changes have migration guidance, caller compatibility tests, and versioning approval.
- [ ] The applicable root `release_critical` and release governance gates pass; this module roadmap alone does not establish release readiness.

## Known Issues & Limitations

- Current language detection is EN/DE stopword-based; do not present it as general multilingual identification.
- Text quality is heuristic, chunk token counts are estimates, and the chunking implementation belongs to the RAG splitter.
- SHA-256 identifies input bytes; it does not authenticate provenance or provide fuzzy/semantic deduplication.
- Empty results may be ambiguous across orchestration; optional sink failure may not make the overall bridge result fail.
- Current benchmark sources include simulations and a non-bridge proxy; historical thresholds and certification statements are not supported by current raw-run artifacts.
- The wiki snapshot is generated at `2026-11-02` (`source_count: 8066`, 1450 removed and 26 changed sources). It is operational context, not a status source; re-sync before future work if module-source coverage or freshness matters.

## Breaking Changes

No v2.5.0 breaking change is planned. The v3.0.0 proposals may change public result types, registry lifecycle, text identity semantics, and streaming contracts. Do not implement or announce these as approved changes until maintainers review the API design, migration plan, and compatibility impact.

## Validation Commands

Use the repository's existing focused targets; commands below are documented test procedures, not results from this planning update.

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target module_toolbox_test_toolbox_primitives_focused
ctest --preset linux-release -R test_toolbox_primitives_toolbox_FocusedTests --output-on-failure
ctest --preset linux-release -L toolbox --output-on-failure
cmake --build --preset linux-release --target bench_toolbox_native_workloads
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads
```

## References

- Unicode Consortium, [Unicode Standard Annex #15: Unicode Normalization Forms](https://www.unicode.org/reports/tr15/).
- Tommi Jauhiainen, Krister Lindén, and Heidi Jauhiainen, [Evaluation of language identification methods using 285 languages](https://aclanthology.org/W17-0221/), NoDaLiDa 2017.
- Wen Xia et al., [FastCDC: A Fast and Efficient Content-Defined Chunking Approach for Data Deduplication](https://www.usenix.org/conference/atc16/technical-sessions/presentation/xia), USENIX ATC 2016.
- [Google Benchmark User Guide](https://google.github.io/benchmark/user_guide.html).
- Repository evidence: [toolbox implementation notes](IMPLEMENTATION.md), [test inventory](../../tests/toolbox/README.md), and [benchmark inventory](../../benchmarks/toolbox/README.md).
