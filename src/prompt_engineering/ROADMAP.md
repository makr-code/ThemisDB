---
Author: "ThemisDB Team"
Created: "2026-08-05"
Last Updated: "2026-11-07"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering Module Roadmap

## Current Status

The source tree contains implementations for template management, version control, evaluation/optimization, feedback, metrics, and the standalone rewrite engine. The root build adds these sources to `themis_core`. Code presence is not a production-readiness or test-pass claim; release acceptance requires current test, benchmark, and human-review evidence.

The rewrite engine is not invoked automatically by `PromptEngineeringIntegration`. Its current focused test checks three interface/compile contracts and does not exercise rule execution.

**Maturity assessment: implemented core, evidence-incomplete.** The module supplies useful authoring, evaluation, and deterministic rewrite building blocks, but the reviewed test and benchmark inventory does not yet demonstrate release-grade rewrite behavior or performance. The root roadmap's `PRODUCTION_CANDIDATE` label is an aggregate classification, not a substitute for the open module acceptance gates below. Its aggregate row reports `Tests=0` while the module test README lists seven test sources; reconcile the counter's meaning, and do not treat either source presence or aggregate labels as test execution evidence. The module does not own model serving, host authorization, or downstream output validation ([architecture](./ARCHITECTURE.md), [trust boundaries](./TRUST.md)).

### Research basis and code gap analysis

| Research / engineering finding | Current evidence and gap | Roadmap implication |
|---|---|---|
| Prompt-injection robustness is an evaluation problem across attack forms and contexts, not a property established by a pattern detector. | The detector and template checks are explicitly heuristic; the adversarial test corpus has hand-authored examples and does not establish generalization or a held-out false-positive rate ([TRUST.md](./TRUST.md), [adversarial tests](../../tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp)). | Keep authorization and execution policy with the host. Measure detector performance on versioned attack and benign sets; do not call it a firewall. |
| Prompt/model quality comparisons need repeatable scenarios, disclosed configuration, and more than a single score. | `PromptEvaluator` is lexical by default; semantic scoring depends on a caller-supplied embedding provider. No fixed, versioned quality suite or recorded provider/model configuration is evidenced here ([IMPLEMENTATION.md](./IMPLEMENTATION.md), [tests README](../../tests/prompt_engineering/README.md)). | Add a versioned evaluation suite and report per-scenario results and uncertainty; separate deterministic unit tests from provider-dependent experiments. |
| Performance claims require measurements of the production path under representative inputs. | Core microbenchmarks cover template/version/feedback/evaluation paths. The rewrite benchmark uses synthetic helpers, not `RewriteEngine` or `RewriteRuleLoader`; performance expectations are proposed, not measured ([benchmark README](../../benchmarks/prompt_engineering/README.md), [performance expectations](./PERFORMANCE_EXPECTATIONS.md)). | Benchmark the actual engine and loader, report p50/p95/p99 and throughput against a reviewed same-environment baseline, and keep source thresholds distinct from measured results. |
| Governance and privacy controls must account for prompt/response data and explicit ownership. | Feedback entries can retain query and response text; retention, access, and redaction remain host responsibilities. The checksum is not cryptographic integrity ([TRUST.md](./TRUST.md), [implementation notes](./IMPLEMENTATION.md)). | Define data-minimization, redaction, retention, and audit ownership before production pipeline integration; avoid raw prompt content in traces by default. |

Research references informing the recommendations:

- Zhu et al., [PromptBench: Towards Evaluating the Robustness of Large Language Models on Adversarial Prompts](https://arxiv.org/abs/2306.04528), 2023 — use diverse adversarial transformations and evaluate robustness rather than infer it from a few hand-picked examples.
- Liang et al., [Holistic Evaluation of Language Models (HELM)](https://arxiv.org/abs/2211.09110), 2022 — use disclosed scenarios and multiple evaluation dimensions for reproducible model assessment.
- OWASP, [LLM01: Prompt Injection](https://genai.owasp.org/llmrisk/llm01-prompt-injection/) — treat direct and indirect injection as application-level risk requiring layered controls.
- NIST, [Artificial Intelligence Risk Management Framework: Generative Artificial Intelligence Profile (AI 600-1)](https://doi.org/10.6028/NIST.AI.600-1), 2024 — ground lifecycle risk management in documented measurement, evaluation, and governance.

These references are guidance, not proof that the module meets their recommendations. Direct external-page fetches were unavailable; the PromptBench, HELM, NIST, and OWASP publication details were checked using web-search results. Maintainers should verify current source revisions and applicability before release sign-off.

## In Progress

- [~] Close the rewrite behavior-test gap and record a current run of the `prompt_engineering` CTest label (Target: v2.5.0).
- [~] Replace synthetic rewrite microbenchmarks with direct `RewriteEngine` / `RewriteRuleLoader` measurements and establish reviewed release-profile baselines (Target: v2.5.0).
- [~] Complete module-level security, privacy, and human-acceptance evidence before making a release-readiness claim (Target: v2.5.0).

## Planned Features

- [ ] Add a versioned, reproducible prompt-quality and adversarial evaluation suite with disclosed datasets/configuration (Target: v2.5.0).
- [ ] Design an explicitly owned, opt-in rewrite integration contract; do not imply automatic rewriting until a caller and downstream validation boundary are approved (Target: v3.0.0 planning).
- [ ] Evaluate bounded scale, privacy, and operational requirements for any integrated pipeline before enabling it by default (Target: v3.0.0 planning).

## Implementation Phases

### v2.5.0 — evidence and hardening waves

Effort figures are preliminary engineering person-weeks (PW), excluding external-provider work, review queues, and release scheduling. They are planning estimates, not commitments.

#### Wave 1 — Behavior and contract evidence (P0, 2–3 PW)
- **Goal:** Establish reliable behavior evidence for existing public contracts before adding new runtime integration.
- **Tasks:** Add behavior tests for rewrite rule matching, phase/priority ordering, ties, idempotence, terminal/block behavior, max-step exhaustion, trace bounds, YAML rejection, and duplicate IDs; cover relevant validation and persistence failure behavior; run and record all tests with the `prompt_engineering` label.
- **Dependencies:** Existing public contracts and test registration in [`tests/prompt_engineering/CMakeLists.txt`](../../tests/prompt_engineering/CMakeLists.txt).
- **Risk:** Tests may lock in accidental behavior or mistake heuristic detection for authorization. Resolve expected semantics with API owners and keep security-policy decisions outside the detector.
- **Success metric / validation:** All registered module tests pass in a recorded build; each listed behavior has an assertion for success and relevant failure/edge cases; no claim of automatic rewrite integration is made. Commands: [`tests README`](../../tests/prompt_engineering/README.md).

#### Wave 2 — Production-path performance baseline (P0, 2–3 PW)
- **Goal:** Make performance and scaling claims reproducible and implementation-specific.
- **Tasks:** Replace synthetic rewrite benchmark helpers with direct engine/loader calls; vary prompt size, rule count, trace collection, and bounded step counts; retain core template/version/evaluation cases; emit a run manifest with build type, compiler, host, and benchmark revision.
- **Dependencies:** Wave 1 semantics and a stable release-profile benchmark environment; Google Benchmark availability.
- **Risk:** Hardware noise, synthetic workloads, or hidden setup cost can produce misleading thresholds. Separate setup from timed work and retain representative inputs and repetitions.
- **Success metric / validation:** The run manifest has zero missing mapped cases and includes p50/p95/p99 plus throughput for each applicable case. Compare against a reviewed same-environment baseline; investigate regressions over 10%. Absolute latency budgets are set only after baseline review. No existing source threshold is treated as an achieved result.

#### Wave 3 — Security, privacy, and reliability evidence (P1, 3–5 PW)
- **Goal:** Specify and measure module-owned safety signals and host-owned controls without overstating guarantees.
- **Tasks:** Maintain versioned attack and benign evaluation sets; report detector recall and false-positive rate by scenario; review literal context substitution, sensitive feedback retention, trace/log redaction, persistence failure semantics, and explicit size/step bounds; document which controls belong to the host.
- **Dependencies:** Wave 1 behavior tests; security and data-governance review; dataset provenance and permission to retain test cases.
- **Risk:** Curated tests can be overfit; benign/attack labels are context-sensitive; traces can expose prompt data. Detector scores must never bypass host authorization, validation, or safety policy.
- **Success metric / validation:** Proposed initial corpus target is ≥95% recall and ≤5% false positives on a held-out, reviewed set, reported with sample counts and per-category results. The owner must approve or revise these targets before gating; passing the corpus is not proof of security. Tests verify redaction and bounded behavior for the implemented paths.

#### Wave 4 — v2.5.0 acceptance and handoff (P0, 1–2 PW)
- **Goal:** Publish a reviewable release decision based on current evidence.
- **Tasks:** Reconcile API, implementation, test, benchmark, trust, and operations documentation; archive test/benchmark manifests; record unresolved risks and obtain module-owner/security review.
- **Dependencies:** Waves 1–3.
- **Risk:** Stale aggregate status or historical results can be mistaken for current evidence.
- **Success metric / validation:** All module acceptance checklist items have dated evidence or remain explicitly open; no production-readiness claim without human sign-off. The module roadmap remains subordinate to root [`ROADMAP.md`](../../ROADMAP.md) and [`VERSIONING.md`](../../VERSIONING.md).

### v3.0.0 — strategic capability waves (schedule and scope uncommitted)

The repository currently plans v2.5.0 as the next minor version; no reviewed source establishes a scheduled v3.0.0 release. These waves are conditional design work, not a release commitment.

#### Wave 1 — Explicit integration contract (P0, 2–3 PW)
- **Goal:** Select one named prompt/request boundary and define a safe, observable rewrite contract.
- **Tasks:** Identify the owning caller; specify opt-in behavior, input/output types, ordering, error/block semantics, trace redaction, and the downstream parser/validator/policy handoff; review API and compatibility consequences before implementation.
- **Dependencies:** v2.5.0 Waves 1 and 3; agreement from the owning integration and security teams.
- **Risk:** Callers may assume the current façade already rewrites requests, or rewritten content may reach execution without validation.
- **Success metric / validation:** Reviewed contract names the owner and every validation handoff; API compatibility and failure cases have approved tests and migration notes before code is scheduled.

#### Wave 2 — Staged pipeline integration (P1, 4–6 PW)
- **Goal:** Implement only the approved boundary, preserving explicit validation and policy enforcement.
- **Tasks:** Integrate the bounded rewrite engine behind an explicit opt-in; add end-to-end tests for valid, blocked, malformed, terminal, and exhausted-step outcomes; initially defer NL→AQL and post-generation expansion until their owners approve separate contracts.
- **Dependencies:** Wave 1 contract; v2.5.0 behavior and performance evidence; downstream parser/validator tests.
- **Risk:** Semantic rewrites can alter user intent, bypass safeguards, or create unbounded work.
- **Success metric / validation:** Every successful transformed request passes its named downstream validator; blocked/error cases fail closed; integration tests cover all configured phases; latency and trace overhead remain within Wave 1-reviewed budgets.

#### Wave 3 — Reproducible quality and governance (P1, 3–5 PW)
- **Goal:** Make prompt changes auditable and compare quality without hiding variance or sensitive-data handling.
- **Tasks:** Version task fixtures and evaluator/provider/model configuration; report per-scenario scores and confidence intervals; define feedback retention, deletion, access, and redaction ownership; keep external-provider tests separate from deterministic unit tests.
- **Dependencies:** Stable integration contract; approved datasets and privacy/security review.
- **Risk:** Model/provider drift makes results non-comparable; telemetry or fixtures may contain personal or confidential prompt content.
- **Success metric / validation:** Re-running a pinned evaluation reproduces the same deterministic metrics; provider-dependent runs disclose provider/model/configuration and variation; privacy review approves retention and redaction behavior before production telemetry is enabled.

#### Wave 4 — Scale and operational readiness (P2, 3–5 PW)
- **Goal:** Demonstrate bounded operation under representative load and failures before broad enablement.
- **Tasks:** Exercise large prompts, high rule counts, concurrent requests, trace limits, malformed configurations, and persistence/provider failures; define operator diagnostics, rollback/disable behavior, and workload-specific budgets.
- **Dependencies:** Waves 2 and 3; representative workload profiles and service-level owners.
- **Risk:** Microbenchmarks may not represent application workloads; concurrency and tracing may increase memory use or leak prompt data.
- **Success metric / validation:** Sustained-load tests meet owner-approved p99 and memory budgets on named hardware profiles; bounded execution and disable/rollback behavior pass fault tests; release evidence records workload and environment.

## Required cross-phase delivery gates

### Phase 1: Design / API Contract
- Complete v2.5.0 Wave 1 expected semantics and the v3.0.0 Wave 1 ownership/API decision; do not infer integration from code presence.

### Phase 2: Core Implementation
- Implement only accepted v2.5.0 hardening and the approved v3.0.0 integration contract; preserve the distinction between lexical YAML rules and compiled policy/semantic rules.

### Phase 3: Error Handling and Edge Cases
- Verify malformed templates/configuration, duplicate rules, persistence failures, terminal behavior, trace limits, and bounded-step exhaustion with explicit outcomes.

### Phase 4: Tests
- Run the focused CTest label; keep deterministic unit tests separate from provider-dependent evaluation; retain exact commands and results.

### Phase 5: Performance and Hardening
- Measure direct production paths, adversarial and high-cardinality cases, and trace overhead against reviewed baselines; do not turn unmeasured source comments into release gates.

### Phase 6: Documentation and Acceptance
- Synchronize the module API, trust, operations, test, and benchmark records; capture owner/security review and unresolved assumptions before any readiness claim.

### Risks and trade-offs

| Trade-off | Risk | Mitigation / decision rule |
|---|---|---|
| Detector recall vs. false positives | Raising sensitivity can reject benign prompts; lowering it can miss attacks, and neither threshold establishes authorization. | Measure both on reviewed held-out sets by category; keep enforcement and authorization with the host. |
| Rewrite coverage vs. caller control | Automatic transformation may alter intent or bypass downstream checks; keeping it separate limits convenience. | Preserve explicit opt-in and a named parser/validator/policy handoff; add further phases only after separate owner approval. |
| Audit detail vs. prompt confidentiality | Raw traces aid diagnosis but can persist sensitive prompt and response content. | Prefer bounded, redacted metadata; require privacy/security review before retaining raw content. |
| Realistic model evaluation vs. reproducibility | Live providers capture realistic variance but drift and external dependencies make runs hard to compare. | Pin and disclose provider/model/configuration; separate deterministic CI fixtures from provider-run reports and include variation. |
| Optimization quality vs. latency | More evaluation iterations or semantic scoring can improve ranking while increasing latency, cost, and provider exposure. | Set workload-specific quality/latency budgets and report both; keep optimization bounded and provider use explicit. |

### Assumptions and open questions

- Which host-owned caller, if any, will own rewrite integration, authorization, and downstream validation?
- Which datasets may be retained, and what retention/deletion, consent, and redaction obligations apply to feedback and traces?
- Who approves the initial adversarial corpus, per-category success targets, and performance budgets?
- Is v3.0.0 intended as a scheduled release, and which API or persisted-format breaks are actually required? Until answered, the v3.0.0 lane remains conditional and no breaking change is scheduled.
- The PW estimates and initial security metrics are proposals for owner review, not measured costs or achieved results.

## Production Readiness Checklist

- [x] Core public headers and implementation sources are present.
- [x] Focused module tests and benchmark targets are wired into the repository build.
- [~] Rewrite behavior tests cover execution semantics and error cases; current rewrite test file checks interface contracts only (Target: v2.5.0).
- [~] Release-profile benchmarks exercise production implementations and have reviewed baselines; the rewrite benchmark is currently synthetic (Target: v2.5.0).
- [ ] Security/privacy targets, ownership, and operational controls are reviewed and measured (Target: v2.5.0).
- [ ] Human acceptance confirms release criteria and operational ownership before any readiness claim (Target: v2.5.0).
- [ ] Any v3.0.0 integration has an approved caller, compatibility plan, downstream validation contract, and staged rollout evidence (Target: v3.0.0; conditional).

## Known Issues & Limitations

- `PromptEngineeringIntegration` does not automatically invoke `RewriteEngine`; callers must not assume requests are rewritten.
- `test_rewrite_engine_focused.cpp` checks interface contracts only; it is not evidence of rule execution behavior.
- `bench_rewrite_engine.cpp` uses local synthetic helpers and is not evidence of `RewriteEngine` performance.
- The error-code enum does not establish that each code is emitted by current runtime paths.
- Prompt scoring quality depends on caller-provided cases and, when configured, an external embedding provider.
- Prompt substitution and injection heuristics do not replace caller-owned authorization, output validation, or safety policy.
- v2.5.0 security and performance targets above are proposed validation criteria; no current measurement is claimed. v3.0.0 scope and schedule remain unconfirmed.

## Breaking Changes

No breaking change is scheduled here. Update the public header contract, migration guidance, and changelog before any breaking API or persisted-format change.

## Historical Evidence

Older PE-FT/RW test IDs and pass tables are historical snapshots, not current test names or results. See [`ISSUE_5663_CLOSURE_EVIDENCE.md`](./ISSUE_5663_CLOSURE_EVIDENCE.md) for the dated closure record. Current test source inventory and commands are in [`../../tests/prompt_engineering/README.md`](../../tests/prompt_engineering/README.md); benchmark source limitations are in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
