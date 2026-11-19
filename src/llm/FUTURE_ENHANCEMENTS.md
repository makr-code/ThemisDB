---
Author: "ThemisDB Team"
Created: "2025-02-21"
Last Updated: "2026-11-19"
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

## Portable Agent Skill Packages (MVP; Remaining Work Proposed)

### Scope

The initial filesystem-backed `SkillRegistry` implementation provides
metadata-only discovery, explicit instruction/resource loading, and opt-in
`ToolRegistry` integration. Remaining work includes representative performance
evidence and any future persistence, embedding-based selection, package
signing, or script-execution designs. Skills complement `ModePack`/`ModeSpec`
and do not replace mode selection, tool authorization, or database execution.
Opt-in guidance packages are under `config/agent_skills/`; they remain inert.
The `asset-impact-analysis` package can be paired with a separately registered,
host-authorized graph and time-series executor. This plan is not a release
commitment.

### Design Constraints

- Follow the open Agent Skills directory convention: one package directory
  contains a `SKILL.md` with YAML frontmatter and Markdown instructions;
  `name` and `description` are required for discovery. Keep any Themis-specific
  fields namespaced and optional so packages remain portable.
- Apply progressive disclosure: startup/refresh exposes only package metadata;
  instruction bodies, references, and other resources are returned only after
  an explicit load request. Current buffered manifest reads can read ahead
  into the body; zero body-byte I/O is a remaining hardening target.
- Treat all skill metadata, instructions, and resources as untrusted input.
  Text can guide the model but cannot grant tool permissions, bypass policy,
  authorize database access, or commit effects.
- Keep authorization in the active `ModeSpec` and trusted executor. A skill's
  suggested tool list is descriptive only; `tools_allowed` and `tools_denied`
  remain authoritative.
- The first implementation is read-only and filesystem-backed. Do not execute
  bundled scripts, persist skill packages in the database, add automatic
  vector/embedding selection, or introduce a general agent graph runtime.
- Discovery roots are explicit configuration. Reject ambiguous package names
  and paths that escape a canonical package root; bound metadata and resource
  sizes before reading them.

### Required Interfaces

The initial internal contract is implemented as follows:

| Type / operation | Contract |
|---|---|
| `SkillDescriptor` | Stable package name, short description, and canonical package root; contains no instruction body. |
| `SkillDocument` | A validated `SkillDescriptor` plus the `SKILL.md` instruction body returned only by explicit load. |
| `SkillRegistry::refresh()` | Scan configured roots and atomically replace the in-memory metadata snapshot; return the number of packages or a typed error for malformed packages and duplicate names. |
| `SkillRegistry::list()` | Return the current metadata snapshot without opening instruction bodies or reference files; fail until a successful refresh. |
| `SkillRegistry::load(name)` | Load and validate the selected package's `SKILL.md`, subject to configured byte and format limits. |
| `SkillRegistry::readResource(name, relative_path)` | Read a package-relative resource only after canonical-path containment checks and resource-size validation. |

Use the repository's `Result<T>` error convention with distinct not-found,
invalid-metadata, duplicate-name, size-limit, path-escape, I/O, uninitialized,
and invalid-content errors. Registry refresh publishes one complete snapshot;
readers observe either the previous or new snapshot, never a partial scan.
Integrate these operations as opt-in built-in tools (for example
`list_skills`, `load_skill`, and `read_skill_resource`) only when registered by
the caller. Each tool must remain subject to the active mode allowlist.

The additive `SkillDelegationContext` primitive carries an exact
action/resource allowlist, fixed principal and tenant, monotonic expiry, and
shared aggregate tool-call/candidate/content-byte limits. Child contexts may
remove permissions and shorten expiry but cannot change identity, extend
expiry, add grants, or reset the shared budget. Root creation must remain in a
trusted host after authorization; skill content, model output, and tool
arguments are not authority sources. This primitive is not yet enforced by
ToolRegistry, native skill executors, or RAG and must not be described as
end-to-end authorization.

### Implementation Notes

- The implemented initial metadata contract requires scalar `name` and
  `description`; top-level metadata keys must be unique, and names are lowercase
  package slugs matching the directory.
  Other frontmatter is ignored rather than interpreted as policy.
- Refresh scans configured roots atomically and returns an explicit count.
  Listing returns sorted metadata; frontmatter, instructions, and resources
  enforce UTF-8 and configured byte limits.
- Discovery parses the frontmatter only, but the buffered stream may read body
  bytes into its buffer. Do not claim zero instruction-body I/O.
- Resource path arguments must be UTF-8 package-relative strings without NUL
  bytes; invalid encoding, absolute paths, and traversal are rejected before
  filesystem resolution.
- Validate the frontmatter and package directory name consistently; do not
  infer authorization from a field in the package.
- Initially let the caller expose the catalog to the selected mode and let the
  model request a matching skill through the loading tool. Do not claim
  automatic intent routing or embedding search.
- Support Markdown references as inert text resources. Scripts may be shipped
  by an upstream package but ThemisDB must not execute them in this scope.
- Keep discovery and reads outside database transactions and avoid logging
  skill bodies or resource contents.
- Preserve compatibility: existing mode packs, tool plugins, MCP bridging,
  and inference call signatures require no changes to use the registry.

### Test Strategy

- Verify metadata-only discovery for multiple packages and prove that no body
  or reference bytes are read until `load` or `readResource` is called.
- Cover valid/invalid frontmatter, missing required fields, unsupported format,
  duplicate names, missing files, non-UTF-8 input, and each size limit.
- Test `..` traversal, absolute paths, invalid-UTF-8/NUL paths, symlink escapes,
  and root replacement. Add race-safe file-opening coverage before treating
  paths as secure against a concurrent untrusted filesystem writer.
- Verify mode allow/deny controls still gate every skill tool and a skill cannot
  enable a tool denied by the mode.
- Verify delegation rejects added capabilities and later expiry, preserves
  principal/tenant identity, and shares atomic budget charges across descendants.
- Exercise concurrent list/load/refresh to prove readers never see partial
  registry state.
- Confirm compatibility by loading existing mode packs and running existing
  tool/inference contract tests unchanged.

### Performance Targets

- Benchmark metadata-only discovery with 1,000 package directories on a
  documented release-CI storage profile; proposed p95 target is <= 100 ms.
- Assert zero instruction-body and resource bytes are loaded during discovery;
  report scan time, package count, metadata bytes, and hardware/storage profile.
- Do not make the latency target a release gate until a reproducible baseline
  exists and maintainers approve the tested storage profile.

### Security / Reliability

- Treat package content as untrusted prompt input; it may contain prompt
  injection or misleading instructions. The model's interpretation cannot
  override system policy or tool authorization.
- Resolve configured roots and requested resources canonically. Reject
  invalid-UTF-8/NUL paths, traversal, absolute paths, all symlinked package
  entries/resources, and files exceeding configured limits; never fall back to
  another package on failure.
- Path checks and file opens are not atomic. Until race-safe opening is
  implemented and tested, keep configured roots, package contents, and parent
  directories unwritable by untrusted users throughout registry use.
- Do not run shell commands, binaries, or scripts from a skill package.
- Fail closed on invalid or ambiguous metadata and expose a diagnostic error
  without returning success-shaped empty instructions.
- Keep content out of routine logs, traces, and metrics; record package identity
  and outcome only if existing privacy policy permits.

### References

- [Agent Skills open format](https://github.com/agentskills/agentskills)
- [Anthropic: Equipping agents with Agent Skills](https://www.anthropic.com/research/equipping-agents-for-the-real-world-with-agent-skills)
- ThemisDB mode and tool contracts: `include/llm/ai_orchestrator.h`
- ThemisDB LLM trust and authority boundaries: `ARCHITECTURE.md`

## Native Asset Impact Analysis (Opt-in)

### Scope

Provide a read-only executor that follows one host-bound outgoing property
graph relation from a start asset and compares a trusted TSStore metric over
two time windows for every reached asset. It is domain-neutral: asset IDs may
represent engineering equipment, financial entities, or other time-series
sources. Optional GeoJSON Point filtering is independent of the metric domain.

### Design Constraints

- The host binds and authorizes one graph ID, edge type, metric, RocksDB
  wrapper, graph manager, and TSStore before registration.
- Package loading does not register or allow the tool. `ModeSpec` remains the
  tool authorization boundary, and the executor accepts no query text.
- Traverse only outgoing edges matching the bound type and graph ID. Return
  errors rather than incomplete traversal results when any hard limit or
  integrity check fails.
- Treat graph and time-series reads as synchronous, non-atomic observations;
  never claim snapshot consistency or causal proof.

### Required Interfaces

- `GraphIndexManager::boundedBfs()` scans a graph-scoped adjacency prefix,
  verifies edge documents, and bounds depth, vertices, examined edges, and
  cooperative traversal time.
- `registerAssetImpactAnalysisTool()` registers `analyze_asset_impact` only
  when the host opts in; the active mode must separately allow it.
- The tool accepts a start asset, depth, two non-overlapping windows, duration,
  and optional geo radius. Graph ID, edge type, metric, and collection scope
  are not user-selectable.

### Implementation Notes

The current executor enforces depth <= 3, 64 vertices, 256 examined adjacency
entries, a cooperative 500 ms graph-traversal deadline, 64 KiB node documents,
and 4,096 samples per window per asset. The start asset is returned at depth
zero. Means are unweighted sample means; empty windows have null means and
delta. Geo filtering accepts a radius up to 1,000 km, skips and counts missing
locations, and fails on malformed locations. The graph timeout cannot interrupt
an individual synchronous RocksDB call.

### Test Strategy

`tests/graph/test_graph_bounded_bfs.cpp` covers graph/type scope, depth, and
fail-closed work limits. `tests/llm/test_skill_registry.cpp` covers the native
tool, mode allowlisting, time-window comparison, optional geo filtering, and
strict failure on malformed matching time-series records against RocksDB and
TSStore. The focused CTest must be built and executed before marking this item
validated.

### Performance Targets

The executor is capped at 64 assets, 256 examined adjacency entries, and
4,096 samples in each window per asset. The graph traversal has a cooperative
500 ms deadline. These caps bound logical work; they do not establish a hard
wall-clock bound for individual storage operations or an aggregate end-to-end
latency SLO.

### Security / Reliability

Bind the graph and TSStore to the same caller scope; do not reuse a registry
across principals. Missing/malformed node or edge data, iterator failures,
invalid samples, invalid locations, and work-limit exhaustion return errors,
not empty or partial success. Host authorization and mode allowlists remain
mandatory; graph traversal itself is not authorization.

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
