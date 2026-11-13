---
Author: "ThemisDB Team"
Created: "2026-02-21"
Last Updated: "2026-11-13"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# LLM Module - Future Enhancements

<!-- Status: current | validated: 2026-09-24 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

## Scope

This document covers forward-looking hardening and capability expansion for the LLM module beyond currently shipped behavior in core runtime paths.

### Synchronization Evidence (2026-07-19)

- Canonical content pass completed against module source code
- All priorities extracted and cross-validated with ROADMAP.md
- Doxygen documentation extended to 100% of .cpp source files
- Build/test evidence collected from module_llm_test_active_vram_allocator_focused

## Design Constraints

- Preserve stable inference and streaming interfaces for existing callers.
- Avoid implicit policy bypass paths when introducing new routing/execution shortcuts.
- Keep model/adapter lifecycle ownership explicit across manager and plugin layers.
- Keep optional backend features degradable with clear fallback behavior.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| Engine submit and streaming APIs | server handlers, adapters, orchestration layers | Must stay backward-compatible across minor releases |
| Router and shard-load update interfaces | scheduler, route selection, distributed execution | Must provide deterministic fallback when telemetry is missing |
| Plugin and adapter lifecycle APIs | plugin manager, deployment/orchestration paths | Must expose explicit load/unload/error states |
| Policy and prompt guard interfaces | API handlers and inference submit paths | Must execute before backend inference calls |
| Versioned model/provider capability contract | native and optional remote provider adapters | Must report actual support for streaming, tools, structured output, deadlines, cancellation, and errors |

## Implementation Notes

- Complete remaining distributed speculative-routing and fan-out hardening.
- Consolidate runtime metrics and error-surface semantics across async/enhanced engines.
- Expand adapter and cache isolation behavior for high-concurrency tenant scenarios.
- Align operator observability surfaces with real runtime controls and failure states.
- Define provider interoperability by inventorying existing plugins and router behavior first; do not treat the OpenAI-compatible response adapter as an outbound provider client.
- Specify optional, version-pinned provider adapters with explicit authentication, timeout, cancellation, tool-call, streaming, and unsupported-capability behavior.
- Keep third-party framework dependencies outside core inference; distinguish
  bounded native intelligence control from optional long-lived orchestration.

### Wave B B3: Multi-Task LoRA Fine-Tuning
**Priority:** High
**Target:** Q1–Q2 2027

- design shared LoRA base with task-specific projection heads
- add domain-gating to route requests to task-specialized adapters
- add joint multi-task loss with configurable weighting
- add ablation workflow to compare shared vs separate adapters
- run three-task benchmark evaluation for transfer and robustness

## Test Strategy

- Add focused tests for distributed execution fallback and partial-failure merge behavior.
- Add stress tests for queue saturation, cancellation storms, and adapter churn.
- Add regression suites for policy enforcement and prompt safety checks under load.
- Keep benchmark-to-target mappings source-verifiable and release-repeatable.

## Performance Targets

- Maintain stable token throughput and prompt latency against current release baselines.
- Keep routing and orchestration overhead bounded under concurrent multi-shard requests.
- Keep cache and adapter operations within bounded latency envelopes under contention.

## Security / Reliability

- Preserve deny-by-default behavior for invalid or unauthorized inference requests.
- Ensure adapter/model path handling remains trust-bound and canonicalized.
- Ensure audit and diagnostics contain enough context for incident triage.
- Require explicit fallback signaling on partial distributed failures.

## Wave B Acceptance Gates (B3)

- average task performance ≥ +8% vs single-task baseline
- training-time increase ≤ 15%
- robust behavior across task configurations

## Related Documents

- AI wave tracker: `../ai/ROADMAP.md`
- bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Risk Backlog

- Distributed optimization regressions under heterogeneous backend availability.
- Cache consistency edge cases during rapid model/adapter lifecycle operations.
- Operational drift between documented and deployed backend feature combinations.

## Native Hybrid Execution and Framework Interoperability (Proposal)

### Scope

Prioritize smaller local models, embedded llama.cpp inference and decentralized
execution near authorized data. Define a bounded native retrieval/inference/tool
path that optional frameworks can also use without a core framework dependency.
See [the hybrid architecture](ARCHITECTURE.md). This is planning, not a new
runtime guarantee, approved provider matrix, or release commitment.

### Design Constraints

- Treat `include/llm/llm_plugin_interface.h` and `ModelRouter` as existing
  extension points; inventory actual implementations before describing support.
- Keep `OpenAICompatAdapter`'s current API direction distinct from a remote
  provider client.
- Permit bounded native re-retrieval, prompt planning, allowlisted tool
  selection, and approved local model/LoRA switching. Models propose;
  trusted database executors authorize, validate and execute.
- Do not infer native-loop safety from a configured iteration count alone.
  Require cumulative token/time/tool/candidate and CPU/RAM/VRAM admission
  budgets, including retries, prefill and distributed fan-out.
- Preserve ordinary database operation when inference is unavailable; a
  shared-process llama.cpp engine does not provide crash isolation.
- Make unsupported tools, streaming, cancellation, structured output, and
  context limits explicit; never silently downgrade a request.
- Keep credentials in the owning server/client secret boundary and never put
  prompts, completions, or credentials in traces or routine logs.

### Required Interfaces

- A versioned provider/model capability description compatible with the
  existing plugin lifecycle and routing surfaces.
- Normalized request, response, tool-call, streaming, deadline/cancellation,
  and error semantics, with provider-specific details retained only as
  explicit metadata.
- A documented capability matrix for each implemented provider and model
  family.
- A model/tokenizer/quantization/LoRA identity and effective-context contract
  shared with RAG; final rendered input plus output/safety reservation must fit.
- A trusted tool-execution boundary with argument/schema validation,
  caller-scoped authorization, cancellation and explicit effect/commit policy.
- An explicit locality and egress policy for node/model routing. No implicit
  remote-provider escalation on local failure.

### Implementation Notes

- Inventory plugin implementations, router inputs, API adapters, and runtime
  tests before choosing an interface or claiming external-provider support.
- Add contract-conformance tests for local and optional remote adapters before
  exposing a shared public abstraction.
- Prioritize native model capabilities and resource admission over a broad
  remote-provider catalog; external packages require approved dependency,
  credential, edition and network boundaries.
- Wire the existing backend tokenizer into final RAG admission without claiming
  that current character-estimated assembly is exact.
- Bind caches to the applicable tenant, data/policy revision, model/LoRA,
  tokenizer and prompt version. Keep embedding-index model identity separate:
  changing only the generator need not rebuild unchanged embeddings.
- Keep generation outside long-held transactions; validate proposed writes
  separately and recheck authorization/version on any delayed approval.
- Qualify the AdaLoRA-to-Multi-LoRA handoff with training/tensor owners;
  training completion, artifact validation, promotion, residency, per-request
  activation and weighted composition are separate states/capabilities.
- Pin adapter revision/composition for each request and its caches. Qualify
  apply/remove concurrency, in-flight unload and KV/prefix invalidation or
  re-prefill on change; reject required-adapter failure rather than silently
  execute the base model. Existing paths require verification against this target.
- Validate tensor-layer artifact selection and actual backend application;
  neither TT storage nor a text-memory compatibility mode proves tensor/KV
  injection into the generator.
- Target: v3.0.0 planning / Q2 2027; timing is provisional and not a release
  commitment.

### Test Strategy

- Verify capability negotiation, unsupported operations, malformed responses,
  timeout, cancellation, partial streams, tool-call round trips, and provider
  failure mapping using deterministic contract fixtures.
- Require the same conformance suite for every provider declared supported;
  test registration alone is not acceptance evidence.
- Compare deterministic local-provider adapter outputs with direct calls.
- Test repeated/no-new-evidence tool loops, all aggregate budget boundaries,
  disallowed tools, stale evidence, revoked permissions and local-node loss.
- Check final token admission for multilingual/code inputs, template/tool
  overhead, model/tokenizer changes and boundary-sized response reservations.
- Run ordinary DB reads/writes concurrently with local prefill/decode and
  background jobs; test inference unavailable, queue/VRAM exhaustion, and
  explicit rejection rather than unapproved cloud fallback.
- Test incompatible base/target shapes, stale/revoked adapters, non-finite
  composition weights, required-adapter apply failure, concurrent tenant
  requests, unload/rollback and cache reuse across adapter changes.
- Compare base-only, RAG-only, adapter-only, RAG+adapter and tensor-enabled
  runs with pinned models/corpora; hold context and compute budgets constant
  where comparable and report any extra tensor/gating/training work.

### Performance Targets

- Measure fixed-context task quality, TTFT/prefill/decode, database p95/p99,
  queue pressure, RAM/VRAM, and cross-node bytes on matched local-model
  workloads. Include model/tokenizer/quantization, corpus, topology and raw data.
- Measure adapter overhead separately from inference. The earlier 5% p95
  normalization figure was a proposal, not evidence; ratify workload-specific
  thresholds only after representative mixed-workload baselines.

### Security / Reliability

- Fail closed on missing credentials, invalid capability metadata, policy
  denial, and unsupported request features.
- Bound retries, request/response sizes, and stream buffering; do not retry
  non-idempotent tool effects without an explicit idempotency contract.
- Redact or exclude prompt, completion, retrieved content, and secrets from
  telemetry and error messages.
- Fail closed on denied locality, missing model/tokenizer identity, or unsafe
  tools; never treat model confidence or schema-valid JSON as authorization.
- Separate optional persistent workflow/approval state from transient KV-cache
  state and from committed database effects; no exactly-once external-effect
  guarantee follows from inference or checkpoint persistence.

## RAG-Readiness Audit Backlog (2026-09-23)

- [ ] Embedding- und Query-Cache Version Governance vereinheitlichen (Target: Q4 2026)
  - Rationale: Persistente Embedding-Caches sind vorhanden, aber Lifecycle-Vertraege ueber Modellwechsel muessen fail-safe sein.
  - Umsetzungsschritte: (1) Cache-Key um Modell-/Tokenizer-Fingerprint erweitern, (2) invalidate-on-model-change als Standard, (3) Migrationspfad fuer Legacy-Caches.
  - Abhaengigkeiten: `src/llm/wiki_index_store.cpp`, `src/rag`, `src/index`.
  - Messbares DoD: 0 stale-cache Treffer nach bewusstem Modellwechsel in Regressionstests; hit-rate Berichte bleiben reproduzierbar.

- [ ] Optionalen Cross-Encoder Re-Ranker als budget-aware Stage integrieren (Target: Q1 2027)
  - Rationale: Hybrid Retrieval braucht qualitaetsgetriebene, aber kostenkontrollierte zweite Ranking-Stufe.
  - Umsetzungsschritte: (1) Re-Ranker API in inference path, (2) Aktivierung ueber query intent + budget guard, (3) telemetry fuer gain/cost ratio.
  - Abhaengigkeiten: `src/rag`, `src/query`, `src/observability`.
  - Messbares DoD: mindestens 80% der Re-Ranking-Aufrufe liefern messbaren Qualitätsgewinn; Kostenbudget pro Query nicht verletzt.
