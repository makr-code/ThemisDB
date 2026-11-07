# Voice Module Roadmap

<!-- Last source/research review: 2026-11-07. Release targets below are proposals, not commitments. -->
<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Team -->
<!-- Exact model backend not verifiable from final document text alone. -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

The module contains assistant/session orchestration, audio preprocessing, wake-word
and intent helpers, streaming and telephony interfaces, biometric/security helpers,
storage, and related utilities. This source inventory is not evidence that every
path is enabled, backed by an external service, or production-ready.

The current aggregate product version is `2.4.0-alpha`; the `v2.5.0` and `v3.0.0`
plans below are research-backed proposals, not scheduled releases. The root
[`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for aggregate maturity
and release blockers and currently records representative-hardware voice baselines
as outstanding.

| Area | Source-backed state | Evidence gap / maturity |
|---|---|---|
| Performance and scale | Voice has nine registered benchmark programs for assistant, preprocessing, session lifecycle, streaming, STT/TTS fixtures, wake-word and dedicated gates (`../../benchmarks/voice/`). The stream manager has a configured concurrency limit. | No representative-hardware p95/p99 result is established by benchmark registration. No source-backed accuracy, concurrency SLO, or end-to-end production backend result is published. |
| Application usability | Assistant, session, stream, telephony and accessibility APIs exist (`../../include/voice/`). Callers can inject transcription/synthesis callbacks into browser streaming. | Browser streaming emits placeholder transcript/TTS output without those callbacks. Intent and wake-word decisions are heuristic; there is no in-module labeled corpus or published recognition-quality study. |
| Security and resilience | Stream validation, fail-closed input checks, session cleanup, audit/security helpers, and adversarial/chaos test sources are present (`../../tests/voice/`). | Voice authentication and wake-word handling are disabled in the assistant defaults. Speaker features and liveness are heuristic and uncalibrated. In-process storage/audit state is not durable, cryptographic protection, or an application authorization boundary. |
| Governance and maintainability | Public contracts, architecture, research, operations, security, and benchmark-inventory documentation are maintained near the module. | CMake/test/benchmark source registration is not execution evidence. Historical closure records are not current CI evidence; keep datasets, model/backend versions, run artifacts and reviewers traceable. |
| Architecture and operations | The module is opt-in/configuration-dependent; voice sources, server handler, and RNNoise use distinct build options. Session, stream and validator limits are separate. | The in-tree telephony/browser types do not prove deployed SIP/WebRTC connectivity. The application/service owns authorization, consent, transport decoding, durable persistence, retention, key management and operational telemetry. |

### Research findings and code-to-research gaps

Research and standards guide evaluation design; they do not establish this module's
performance, accuracy, legal compliance, protocol conformance, or product SLOs.

| Finding and source | Implication for the module | Current code/evidence and gap |
|---|---|---|
| Large-scale weakly supervised ASR can generalize across speech tasks, but results are tied to evaluated data and systems: Radford et al., *Robust Speech Recognition via Large-Scale Weak Supervision* (ICML 2023, [PMLR](https://proceedings.mlr.press/v202/radford23a.html)). | Evaluate real injected backends using versioned, held-out audio; report WER/CER by language and acoustic condition alongside latency. | `src/voice/RESEARCH.md` documents there is no module corpus or published accuracy study; the browser default is placeholder output, not ASR. |
| ITU-T P.808 describes crowdsourced subjective speech-quality evaluation ([P.808](https://www.itu.int/itu-t/recommendations/rec.aspx?rec=P.808)). | Use a controlled human-listening protocol for TTS naturalness/intelligibility, separate from ASR error rates and runtime benchmarks. | Registered STT/TTS fixture benchmarks do not measure perceived quality or establish an external TTS backend's quality. |
| ISO/IEC 30107-3:2023 specifies PAD testing/reporting and attack classifications, but excludes specific countermeasure design and overall system security ([ISO standard](https://www.iso.org/standard/79520.html)); ASVspoof 5 includes crowdsourced, deepfake and adversarial speech ([2024 paper](https://arxiv.org/abs/2408.08739)). | Test presentation attacks on held-out attack sources; report false-accept/false-reject behavior at the selected threshold. Do not treat PAD as identity assurance or authorization. | The module uses hand-engineered speaker features and heuristic liveness. Existing adversarial tests are source evidence, not calibrated rates or independent attack-set results. |
| WebRTC and IETF transport standards define real-time communication APIs/protocol requirements, not an application's latency or recovery SLO ([W3C WebRTC](https://www.w3.org/TR/webrtc/), [RFC 8835](https://www.rfc-editor.org/rfc/rfc8835), [RFC 8836](https://www.rfc-editor.org/rfc/rfc8836)). | Keep media/control transport choices explicit and test application backpressure, reconnect, ordering, cancellation and session recovery under network impairment. | `voice_browser_streaming` and telephony types are in-tree interfaces; the architecture docs do not claim deployed protocol conformance. |
| NIST AI RMF 1.0 recommends a lifecycle risk approach through GOVERN, MAP, MEASURE and MANAGE; it is voluntary, not certification ([NIST AI 100-1, 2023](https://doi.org/10.6028/NIST.AI.100-1)). EDPB VVA guidance and GDPR biometric-data definitions make purpose, consent and minimization relevant to voice systems ([EDPB Guidelines 02/2021](https://www.edpb.europa.eu/system/files/2021-07/edpb_guidelines_202102_on_vva_v2.0_adopted_en.pdf), [GDPR](https://eur-lex.europa.eu/eli/reg/2016/679/oj/eng)). | Keep audio/transcript purpose, consent, retention, access, deletion, model provenance and risk review explicit at the owning deployment; legal applicability must be assessed for each use. | `VoiceSecurityManager`, audio storage and audit helpers are in-process. The module alone does not provide durable compliance records, encryption/key management, or a legal determination. |
| WCAG 2.2 includes live-caption criteria for applicable synchronized media ([W3C WCAG 2.2](https://www.w3.org/TR/WCAG22/)). | Validate captions/transcripts and non-voice alternatives in the user interface; an API or accessibility helper name is not a conformance claim. | The module exposes accessibility types, but no published end-user conformance audit or usability result is present. |

### Open assumptions and knowledge gaps

- **Use cases and backends:** no product owner has declared the supported languages, deployment types, STT/TTS providers, or whether speaker biometrics remain in scope. Wave 1 must establish these before accepting the target matrix.
- **Evaluation data:** corpus rights, consent, demographic coverage, speaker/session split, and retention are not yet established. Do not select or publish a production-quality claim until those conditions and data owners are recorded.
- **Hardware and workload:** representative devices, microphone/network conditions, concurrency distribution, and service SLO owners are unspecified. The numerical latency thresholds below are provisional proposals, not evidence-based universal limits.
- **Compliance and accessibility:** GDPR/EDPB and WCAG applicability depends on the product, processing purpose, jurisdiction, and user interface; this roadmap is not legal advice or a conformance assessment.
- **Wiki freshness:** the Developer LLM Wiki status was generated 2026-11-02 (8,066 sources; 1,450 removed and 26 changed) and its module index has no curated voice page. This plan therefore uses the current module sources and docs; the wiki refresh is outside this roadmap.
- **Effort estimates:** person-week ranges are planning approximations only; provider choice, corpus preparation, privacy review, and hardware access may change them.

## In Progress and Planned Features

**Release separation:** `v2.5.0` is the proposed near-term, backward-compatible
evidence and integration-hardening release. `v3.0.0` is strategic and may include
incompatible public-contract changes only where an API or wire incompatibility is
necessary and approved under [`../../VERSIONING.md`](../../VERSIONING.md). No
release date, provider, dataset, or staffing commitment is implied here.

### v2.5.0 — measurable, bounded integration

#### Wave 1 — Reproducible quality and latency baseline (P0; estimate: 2–3 engineer-weeks)

- **Goal:** establish comparable quality and performance evidence for each declared backend/hardware profile.
- **Tasks:**
  - [ ] Select supported use cases, languages, consented/licensed fixtures, disjoint speaker/session splits, and a versioned test manifest; record dataset, backend/model, build options, hardware, sample rate, codec, configuration and source revision (Target: v2.5.0).
  - [ ] Run real injected STT/TTS backends separately from synthetic fixtures; report WER/CER, time-to-first-partial, finalization delay, time-to-first-audio, real-time factor, and p50/p95/p99 latency by profile and condition (Target: v2.5.0).
  - [ ] Run at least 100 independent utterances per declared profile/condition and preserve raw benchmark summaries and environment provenance; identify any smaller sample set as exploratory, not a p95 release baseline (Target: v2.5.0).
- **Dependencies:** approved dataset rights/consent, a maintained real-backend integration, and representative hardware access.
- **Risk / trade-off:** hardware, corpus, and backend changes invalidate direct comparisons; maintain fast fixture benchmarks but do not promote them to release evidence.
- **Success metric / validation:** every supported profile has a reproducible manifest and per-condition quality/latency results; no accuracy regression greater than 5% relative to its approved same-profile baseline without review. Candidate interaction targets are p95 first partial ≤500 ms, finalization ≤800 ms after end-of-speech, and first TTS audio ≤300 ms; these are provisional engineering targets to be confirmed against the product workflow, not standards or current measurements.

#### Wave 2 — Production integration and privacy boundaries (P0; estimate: 3–5 engineer-weeks)

- **Goal:** make production integration requirements testable without claiming that in-process helpers provide deployment controls.
- **Tasks:**
  - [ ] Add an integration conformance path that supplies real transcription/synthesis backends, checks readiness before accepting production traffic, and verifies unavailable-backend errors; keep fixture/placeholder results clearly separated from production results (Target: v2.5.0).
  - [ ] Exercise consent, caller authorization, origin policy, retention/deletion, audit delivery, and sensitive-data redaction at the owning service boundary; verify that raw audio, transcripts, biometric features and credentials do not enter ordinary diagnostics (Target: v2.5.0).
  - [ ] Document ownership of encryption, key management, durable session/audit storage, model/backend provenance, and deployment-specific privacy review; test the implemented integration rather than inferring it from helper metadata (Target: v2.5.0).
- **Dependencies:** Wave 1 profile/fixture choices; service owner and privacy/security review; no new module-wide compliance claim.
- **Risk / trade-off:** stricter production readiness checks can expose incomplete deployments and require service integration work; keep test-only fixtures distinguishable and avoid silently accepting missing providers as successful recognition.
- **Success metric / validation:** all production integration profiles use an exercised real backend; missing/unavailable backends are observable failures, never successful recognition; seeded sensitive values are absent from ordinary logs; retention/deletion and durable audit checks pass at the service boundary.

#### Wave 3 — Sustained-load and recovery evidence (P1; estimate: 3–4 engineer-weeks)

- **Goal:** validate resource bounds, stream/session recovery and cleanup on representative hardware.
- **Tasks:**
  - [ ] Load-test session/stream counts from 1 through configured capacity with bounded audio duration and realistic chunk cadence; record throughput, p50/p95/p99, memory, queue/buffer high-water marks and rejection reasons (Target: v2.5.0).
  - [ ] Inject backend timeout/loss, cancellation, malformed/out-of-order chunks, client disconnect and restart; verify documented rejection, reconnect and cleanup behavior without duplicate privileged actions (Target: v2.5.0).
  - [ ] Execute the focused voice `release_critical` test labels and relevant sanitizer/chaos jobs on `develop`, preserving run IDs, source revision and artifacts (Target: v2.5.0).
- **Dependencies:** Waves 1–2 test profiles and the owning representative-hardware runner.
- **Risk / trade-off:** synthetic load can miss real network/backend bottlenecks; distinguish component-only stress results from integrated service results.
- **Success metric / validation:** zero leaked sessions after teardown, resource use stays within configured limits, every injected fault has an asserted outcome, and p95/p99 plus `release_critical` evidence is published for the declared profile. A registered test or label alone does not pass this gate.

### v3.0.0 — versioned capabilities and governed operation

#### Wave 4 — Explicit backend and streaming contracts (P0; estimate: 4–6 engineer-weeks)

- **Goal:** expose provider capabilities, failure states and session lifecycle semantics consistently across integrations.
- **Tasks:**
  - [ ] Specify versioned capability metadata for streaming, cancellation/deadlines, locale, chunk format, limits, and backend readiness; add conformance tests for each supported adapter (Target: v3.0.0).
  - [ ] Evaluate removing browser placeholder transcript/TTS defaults in favor of explicit backend-unavailable outcomes; provide a migration path and only make an incompatible default change if justified and approved as a major-version contract change (Target: v3.0.0).
  - [ ] Define reconnect, backpressure, final-result commitment, idempotency and teardown semantics at the public API boundary (Target: v3.0.0).
- **Dependencies:** v2.5.0 Waves 1–3 evidence, maintained adapter inventory and maintainer approval for any incompatible behavior.
- **Risk / trade-off:** explicit capability failures can require caller migration; do not imply WebRTC/SIP conformance unless the chosen transport implementation is independently verified.
- **Success metric / validation:** all supported adapters pass a shared contract suite; unsupported capabilities fail explicitly; migration guidance and compatibility results are published before contract freeze.

#### Wave 5 — Quality-calibrated speech and biometric options (P1; estimate: 6–10 engineer-weeks)

- **Goal:** evaluate speech and anti-spoof methods against independent, representative data before selecting a model or enabling identity use.
- **Tasks:**
  - [ ] Compare candidate ASR/wake-word/intent approaches against Wave 1 baselines by language, accent, noise, device and overlap; report confidence intervals and operating points (Target: v3.0.0).
  - [ ] If biometric use remains in scope, evaluate replay, synthesis, voice-conversion and unseen attacks with held-out speakers/attack sources; publish false-accept/false-reject rates and calibration, and keep voice match separate from application authorization (Target: v3.0.0).
  - [ ] Evaluate TTS with a controlled listener study and report naturalness/intelligibility separately from machine latency; use P.808-style test controls where appropriate (Target: v3.0.0).
- **Dependencies:** versioned consented evaluation data, Wave 1 baseline, independent security/privacy review, and explicit product justification for biometric processing.
- **Risk / trade-off:** public benchmarks may not match deployment populations; a learned method can improve average quality while worsening subgroup or spoof performance. Retain the simpler path if evidence is inconclusive.
- **Success metric / validation:** no component is described as accurate or spoof-resistant without held-out per-condition results; selected methods meet maintainer-approved quality and false-accept/false-reject limits with confidence intervals; biometric features remain opt-in and are not the sole authorization factor.
- **Acceptance prerequisite:** record numeric, risk-approved quality and false-accept/false-reject limits before evaluating the held-out set; do not tune thresholds on the final test partition.

#### Wave 6 — Accessible, durable and auditable deployment boundary (P1; estimate: 4–6 engineer-weeks)

- **Goal:** make the voice feature operable and reviewable at service boundaries without coupling the module to one deployment's storage or legal regime.
- **Tasks:**
  - [ ] Define adapter contracts and deployment tests for durable session/audit storage, retention/deletion, access controls and encryption/key ownership; keep in-memory implementations explicitly scoped to tests or deployments that accept volatile state (Target: v3.0.0).
  - [ ] Validate captions/transcripts, keyboard-operable controls, visible listening/processing/speaking state, and a non-voice alternative for essential actions against the applicable accessibility requirements (Target: v3.0.0).
  - [ ] Add operational dashboards/runbooks for backend health, latency, rejection/failure rates, active sessions, buffer limits, recovery and privacy incidents without logging sensitive payloads (Target: v3.0.0).
- **Dependencies:** service owners, Wave 4 capability contracts, data-protection/accessibility review, and real deployment telemetry.
- **Risk / trade-off:** durable storage and cross-jurisdiction retention add operational cost and policy variation; keep those decisions configurable and owned by the integrating service.
- **Success metric / validation:** deployment exercises prove retention/deletion and audit delivery end to end; accessibility review records applicable criteria and results; runbooks successfully exercise backend degradation and recovery without exposing audio or transcript payloads.

## Implementation Phases

### Phase 1 — Design / API contract

- [ ] Record supported user journeys, languages, backend availability semantics, audio/data flow, consent and retention owners, and evaluation profiles (Target: v2.5.0 Wave 1).

### Phase 2 — Core implementation

- [ ] Add production integration readiness checks and real-backend conformance tests; define the versioned capability surface only after Wave 1 profiles are agreed (Target: v2.5.0 Wave 2 and v3.0.0 Wave 4).

### Phase 3 — Failure handling and edge cases

- [ ] Specify and exercise unavailable backend, malformed/oversized audio, timeout, cancellation, disconnect, reconnect, retention expiry and teardown paths (Target: v2.5.0 Waves 2–3).

### Phase 4 — Tests

- [ ] Add held-out quality fixtures, service-boundary integration tests, attack-condition tests and adapter contract tests, preserving dataset/backend provenance (Target: v2.5.0 Wave 1 and v3.0.0 Waves 4–5).

### Phase 5 — Performance / hardening

- [ ] Run representative-hardware latency and sustained-load baselines, fault injection, sanitizer/chaos validation, and risk-based biometric/accessibility reviews (Target: v2.5.0 Wave 3 and v3.0.0 Waves 5–6).

### Phase 6 — Documentation and acceptance

- [ ] Publish versioned results, integration/migration guides, operator runbooks, open assumptions and human sign-offs; update release claims only after evidence is reviewed (Target: v2.5.0 and v3.0.0).

## Production Readiness Checklist

- [ ] Each supported production profile identifies a real STT/TTS backend and has a passing integration-readiness check.
- [ ] Quality and latency benchmarks use consented/versioned held-out data and representative hardware; p50/p95/p99 and corpus/backend provenance are published.
- [ ] Session/stream capacity, backpressure, timeout, cancellation, recovery and teardown are tested at configured limits.
- [ ] Caller authentication/authorization, consent, origin policy, encryption/key management, durable audit, retention and deletion are verified at the owning service boundary.
- [ ] Biometric use, if enabled, has independent held-out PAD/false-accept/false-reject evidence and is not treated as application authorization.
- [ ] Applicable accessibility checks, incident/runbook exercises and privacy/security reviews have named owners and retained evidence.
- [ ] `release_critical` voice jobs pass on `develop`; registration, labels, historical snapshots and local source presence are not substituted for current execution evidence.
- [ ] Before release acceptance, each applicable target has a named owner and pre-registered threshold; unknown legal, dataset, provider or hardware assumptions are resolved or explicitly accepted by the responsible human reviewer.

## Known Issues & Limitations

- Browser streaming emits placeholder transcript text and echoes transcript bytes as default TTS when callbacks are absent (`src/voice/voice_browser_streaming.cpp`); neither is a real speech backend.
- Wake-word/intent paths are heuristic. Speaker matching uses handcrafted features and heuristic liveness; no module corpus, calibrated rates or published accuracy is established.
- `VoiceAssistant::Config` disables voice authentication and wake-word handling by default. Per-header session, stream and validator limits are not interchangeable global limits.
- Session, audio-storage, biometric, security and audit helpers use in-process state unless an application integration provides persistence. Audio encryption metadata is not cryptographic encryption.
- Public telephony/browser interfaces do not establish a deployed SIP/WebRTC transport or standards conformance. Authorization, consent, durable audit, retention, key management and telemetry remain service-owned.
- Nine benchmark programs and focused/release-critical test sources are registered, but representative-hardware p95/p99 results and current passing execution records are not established by registration. See [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`RESEARCH.md`](RESEARCH.md), and [`WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`](WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md) for evidence boundaries.

## Breaking Changes

- `v2.5.0` work must remain backward-compatible; additions to integration checks or capability reporting must not silently change existing public defaults.
- Removing placeholder transcript/TTS behavior or changing public streaming/session semantics may be incompatible. Consider such changes only in `v3.0.0` with a documented migration, a concrete API/wire incompatibility rationale, and approval under [`../../VERSIONING.md`](../../VERSIONING.md). If no major-version incompatibility is necessary, deliver the change additively in a compatible release instead.

## Canonical References

- [Root roadmap](../../ROADMAP.md) — aggregate maturity, current release and blockers.
- [Voice implementation architecture](ARCHITECTURE.md), [API summary](API.md), [research/evaluation notes](RESEARCH.md), [security boundaries](SECURITY.md), and [benchmark inventory](PERFORMANCE_EXPECTATIONS.md).
- [Public voice headers](../../include/voice/) — authoritative declarations and limits.
- [Voice test registration](../../tests/voice/CMakeLists.txt) and [benchmark registration](../../benchmarks/voice/CMakeLists.txt) — source/registration only, not run evidence.
- [Research-to-implementation map](../../research/implementation_influence/by_module.md#srcvoice).
