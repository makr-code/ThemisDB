---
Author: ThemisDB Contributors
Created: 2026-10-29
Last Updated: 2026-10-29
Status: active
AI_Used: true
AI_Models: GitHub Copilot
AI_Contributor: GitHub Copilot
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Module Issue Master Catalog

This document is the consolidated master catalog for the evidence-first issue portfolio used across ThemisDB modules. It is meant to be the operational index for module-level engineering tasks and for the v2.5.0 → v3.0.0 evolution plan.

## Canonical issue format

Each module issue follows the same structure:

- Scope
- Current state and evidence
- Research / system baseline
- Required design constraints
- Targeted improvements
- Validation
- Acceptance criteria

This format is intentionally concrete and measurable so that Copilot and maintainers can execute the work without vague or marketing-oriented prompt language.

## Master catalog

| Module | Issue title | Objective |
|---|---|---|
| acceleration | `[module][evidence] acceleration: reconcile backend selection and fallback policy with measurable latency and safety gates` | align dispatch, locality, queueing, and fail-safe execution with measurable latency and fallback targets |
| access_model | `[module][evidence] access_model: align permission semantics, policy validation, and fail-safe enforcement with explicit edge-case coverage` | verify role/permission semantics, invalid-state handling, and explicit deny/allow policy behavior |
| ai | `[module][evidence] ai: align orchestration, routing, and degraded-state semantics with explicit capability and safety gates` | ensure routing and orchestration decisions are explicit, bounded, and fail-safe under degraded model/service availability |
| ai_working | `[module][evidence] ai_working: reconcile operational evidence, execution history, and current-state reporting with explicit evidence and governance boundaries` | separate active governance evidence from historical execution artifacts and prevent false status claims |
| analytics | `[module][evidence] analytics: align aggregation semantics, latency, and degraded-state behavior with explicit performance and correctness gates` | validate aggregation correctness, bounded degradation, and measurable latency/throughput targets |
| api | `[module][evidence] api: reconcile contract stability, validation, and failure semantics with explicit compatibility targets` | align API validation and contract behavior with invalid-input and compatibility boundaries |
| aql | `[module][evidence] aql: align execution semantics, validation, and failure handling with measurable query and safety gates` | verify AQL validation, execution behavior, and safety under invalid or degraded query conditions |
| auth | `[module][evidence] auth: enforce fail-closed token and policy validation with explicit edge-case documentation` | ensure invalid token, policy, or trust state fails closed and is documented |
| base | `[module][evidence] base: align core primitives, invariants, and failure semantics with explicit correctness and stability gates` | validate foundational runtime invariants and compatibility under invalid or degraded state |
| cache | `[module][evidence] cache: align eviction, locality, and invalidation semantics with measurable latency and bounded memory targets` | ensure cache behavior is explicit, stale-safe, and memory-bounded under load |
| cdc | `[module][evidence] cdc: reconcile change capture ordering, validation, and recovery semantics with measurable consistency gates` | verify capture ordering, replay semantics, and fail-safe recovery behavior |
| chaos | `[module][evidence] chaos: align resilience experiments, fault-injection behavior, and safety boundaries with explicit validation targets` | make resilience tests explicit, bounded, and safe under fault injection |
| chimera | `[module][evidence] chimera: align hybrid execution semantics, compatibility, and degraded-state handling with explicit maturity and safety gates` | verify mixed-mode execution remains safe and compatible under degraded conditions |
| config | `[module][evidence] config: align validation, precedence, and fail-safe semantics with explicit configuration integrity gates` | ensure config precedence, validation, and invalid-state handling are explicit and safe |
| content | `[module][evidence] content: align versioning, validation, and fallback semantics with explicit correctness and freshness targets` | verify content versioning and fallback semantics remain correct and fresh under partial failure |
| core | `[module][evidence] core: align foundational runtime invariants, safety semantics, and compatibility gates with explicit correctness and stability targets` | validate core invariant enforcement and compatibility boundaries across subsystem runtime behavior |
| distributed_knowledge | `[module][evidence] distributed_knowledge: align graph synchronization, consistency, and degraded-state handling with explicit coordination gates` | verify distributed knowledge consistency and synchronization under partial node or replica failure |
| distributed_tensor | `[module][evidence] distributed_tensor: align shard placement, locality, and fallback semantics with explicit distributed correctness gates` | validate distributed tensor placement and recovery semantics under partial-device failure |
| document | `[module][evidence] document: reconcile versioning, validation, and schema semantics with explicit correctness and failure gates` | verify document integrity, schema validity, and lifecycle behavior under invalid inputs |
| ethics_ai | `[module][evidence] ethics_ai: align policy evaluation, fairness checks, and fail-safe reporting with explicit evidence and validation gates` | ensure fairness evaluation and reporting remain explicit, auditable, and fail-safe |
| evaluation | `[module][evidence] evaluation: align benchmark, model, and scoring semantics with measurable quality and safety gates` | validate model evaluation quality and scoring integrity under partial or invalid inputs |
| execution | `[module][evidence] execution: align scheduling, lifecycle, and fail-safe semantics with explicit runtime and safety gates` | confirm execution lifecycle and scheduling remain explicit and bounded under load |
| exporters | `[module][evidence] exporters: align format guarantees, failure semantics, and throughput with measurable compatibility targets` | verify export correctness and degraded-state behavior under invalid payloads and high volume |
| failover | `[module][evidence] failover: align detection, switch-over, and recovery semantics with explicit safety and latency gates` | ensure switch-over and recovery logic is explicit, safe, and bounded under degraded state |
| geo | `[module][evidence] geo: reconcile spatial index, validation, and failure semantics with explicit correctness and latency gates` | ensure spatial validation and query semantics remain correct under invalid or degraded input |
| governance | `[module][evidence] governance: reconcile enforcement, policy, and audit semantics with explicit validation and traceability` | verify policy enforcement, auditability, and invalid-policy fail-safe behavior |
| gpu | `[module][evidence] gpu: harden capability gating and fallback behavior for heterogeneous execution paths` | enforce capability gating and bounded fallback under heterogeneous GPU runtime constraints |
| graph | `[module][evidence] graph: align traversal, consistency, and degraded-state handling with explicit correctness and performance gates` | verify traversal correctness, graph consistency, and partial-failure handling |
| image_analysis | `[module][evidence] image_analysis: align inference, validation, and fail-safe semantics with measurable quality and runtime gates` | validate model execution and input validation for image analysis under degraded conditions |
| importers | `[module][evidence] importers: reconcile format compatibility, invalid-input handling, and throughput with explicit validation gates` | verify importer correctness and fail-safe behavior for malformed payloads and partial data |
| index | `[module][evidence] index: define bounded maintenance, freshness, and degraded-state semantics with measurable performance gates` | ensure index freshness and maintenance behavior remain bounded, explicit, and correct |
| ingestion | `[module][evidence] ingestion: reconcile throughput, validation, and bounded failure semantics with measurable pipeline targets` | validate throughput and failure handling under load and malformed input |
| llama_cpp | `[module][evidence] llama_cpp: align runtime compatibility, inference behavior, and fail-safe boundaries with explicit validation gates` | verify backend compatibility and runtime safety for llama_cpp execution |
| llm | `[module][evidence] llm: align inference, routing, and fallback behavior with bounded latency and safety constraints` | ensure LLM routed execution remains bounded, valid, and fail-safe under degraded states |
| llm_streaming | `[module][evidence] llm_streaming: align response streaming, latency, and fail-safe semantics with explicit bounded-output and recovery gates` | verify stream continuity, partial-output handling, and bounded retry behavior |
| llm_wiki | `[module][evidence] llm_wiki: align knowledge integration, retrieval quality, and evidence provenance with explicit operational and safety gates` | verify provenance-aware knowledge retrieval and fail-safe degraded behavior |
| maintenance | `[module][evidence] maintenance: align compaction, cleanup, and degraded-state handling with bounded operational overhead` | ensure maintenance remains safe, bounded, and non-disruptive to live state |
| metadata | `[module][evidence] metadata: align schema, lifecycle, and consistency semantics with explicit validation and bounds` | validate metadata lifecycle and consistency under invalid or partial state |
| network | `[module][evidence] network: align protocol behavior, retry policy, and fail-safe handling with explicit latency and resilience gates` | ensure networking semantics are explicit, bounded, and safe under degraded connectivity |
| observability | `[module][evidence] observability: align metrics, tracing, and degraded-state reporting with measurable signal quality and safety` | verify instrumentation fidelity and degraded reporting semantics under partial failure |
| onnx_clip | `[module][evidence] onnx_clip: align inference contract, model validity, and fail-safe behavior with measurable quality gates` | validate ONNX CLIP execution remains explicit and safe under invalid runtime state |
| performance | `[module][evidence] performance: align instrumentation, bottleneck detection, and regression gates with measurable system targets` | ensure metrics and benchmark gates remain trustworthy and actionable |
| plugins | `[module][evidence] plugins: align plugin contract, compatibility, and fail-safe behavior with explicit extension boundaries` | validate plugin lifecycle, compatibility checks, and safe extension behavior |
| process | `[module][evidence] process: align execution, scheduling, and failure handling with bounded runtime and safety constraints` | verify process scheduling and lifecycle remain explicit and bounded under failure |
| projects | `[module][evidence] projects: align lifecycle, configuration, and degraded-state handling with explicit project correctness gates` | ensure project lifecycle and config remain safe and explicit under partial failure |
| prompt_engineering | `[module][evidence] prompt_engineering: align prompt construction, validation, and safety boundaries with explicit correctness and quality gates` | validate prompt safety, context integrity, and bounded fallback behavior |
| query | `[module][evidence] query: align planner/executor behavior with latency, fairness, and fail-safe execution targets` | verify planner/executor contract remains explicit, fair, and fail-safe under load |
| rag | `[module][evidence] rag: reconcile retrieval quality, latency, and fallback policy with measurable system constraints` | ensure retrieval and generation remain quality-safe under degraded retrieval conditions |
| replication | `[module][evidence] replication: reconcile consistency, lag, and failover guarantees with measurable operational targets` | verify replica consistency, lag bounds, and failover semantics under degraded conditions |
| retrieval | `[module][evidence] retrieval: align retrieval quality, fallback behavior, and degraded-state semantics with measurable search and safety gates` | ensure retrieval logic remains bounded, safe, and quality-aware under partial failure |
| rpc_grpc | `[module][evidence] rpc_grpc: align transport semantics, retry policy, and failure handling with explicit latency and safety gates` | verify RPC transport semantics under degraded connectivity and retry boundaries |
| scheduler | `[module][evidence] scheduler: align dispatch fairness, bounded latency, and failure handling with explicit runtime and capacity gates` | validate scheduler fairness, latency bounds, and failure-safe dispatch behavior |
| scraper | `[module][evidence] scraper: align extraction quality, validation, and bounded failure semantics with explicit data-quality gates` | ensure extraction remains bounded and explicit under malformed or partial source data |
| search | `[module][evidence] search: document bounded retrieval and index fallback semantics under degraded or invalid state` | verify search fallback and degraded-state semantics remain explicit and bounded |
| security | `[module][evidence] security: align enforcement, isolation, and fail-safe policy with concrete validation targets` | ensure policy enforcement, trust boundaries, and auditability remain fail-safe |
| server | `[module][evidence] server: align request handling, bounded latency, and fail-safe semantics with explicit operational gates` | validate server request behavior, latency bounds, and failure handling under load |
| sharding | `[module][evidence] sharding: define bounded routing, failover, and safety semantics under degraded partition state` | ensure shard routing and failover remain safe under degraded partition state |
| stable_diffusion | `[module][evidence] stable_diffusion: align runtime compatibility, execution validity, and quality gates with explicit fallback and safety constraints` | validate model runtime compatibility and safe degraded execution semantics |
| storage | `[module][evidence] storage: align durability, consistency, and recovery semantics with explicit operational and correctness gates` | ensure storage integrity is explicit, durable, and recoverable under failure |
| temporal | `[module][evidence] temporal: align time semantics, consistency, and validation rules with explicit correctness and safety gates` | validate temporal ordering and consistency under invalid or drifted time state |
| tensor | `[module][evidence] tensor: align shape validation, execution semantics, and degraded-state handling with explicit correctness and runtime gates` | validate tensor correctness and runtime safety under invalid or mismatched shape state |
| themis | `[module][evidence] themis: align core runtime semantics, compatibility, and fail-safe design with explicit system-level gates` | validate integration-level runtime safety and compatibility across core boundaries |
| timeseries | `[module][evidence] timeseries: align ordering, validation, and degraded-state handling with explicit temporal correctness gates` | verify time-series correctness and ordering under partial or invalid data |
| toolbox | `[module][evidence] toolbox: align utility contracts, validation, and failure semantics with explicit correctness and compatibility gates` | validate general utility correctness and error handling under invalid input |
| training | `[module][evidence] training: align runtime quality, checkpointing, and failure handling with explicit resilience and correctness gates` | ensure training quality and checkpoint integrity remain safe and recoverable |
| transaction | `[module][evidence] transaction: reconcile isolation, durability, and fail-safe recovery semantics with measurable release gates` | validate isolation, durability, and recovery semantics under partial transaction failure |
| updates | `[module][evidence] updates: align mutation semantics, validation, and recovery with explicit correctness and bounded failure gates` | verify mutation ordering and rollback correctness under partial update failure |
| user_storage_encrypted | `[module][evidence] user_storage_encrypted: align encryption semantics, validation, and recovery with explicit safety and integrity gates` | validate encrypted storage integrity, invalid key handling, and recovery safety |
| utils | `[module][evidence] utils: align utility correctness, invalid-input handling, and compatibility with explicit safety gates` | ensure helper-layer correctness remains safe under invalid input and compatibility drift |
| vector_search | `[module][evidence] vector_search: align similarity search quality, fallback logic, and degraded-state handling with measurable performance gates` | validate vector similarity quality and bounded degraded fallback behavior |
| voice | `[module][evidence] voice: align inference quality, validation, and degraded-state handling with explicit runtime and safety gates` | ensure voice inference remains explicit, fail-safe, and quality-aware |
| whisper | `[module][evidence] whisper: align transcription quality, runtime compatibility, and fail-safe behavior with explicit validation gates` | validate transcription quality and safe degraded execution under invalid audio state |

## Priority order

The issue catalog is grouped in the recommended rollout order:

1. release-critical and performance-sensitive modules
2. correctness and safety-critical modules
3. integration and platform modules
4. operational and support modules

This order aligns with the evidence-first work pattern established in the acceleration roadmap and the module evolution blueprint.

## Related documents

- [docs/MODULE_EVOLUTION_ISSUE_BLUEPRINT.md](MODULE_EVOLUTION_ISSUE_BLUEPRINT.md)
- [docs/MODULE_ISSUE_PORTFOLIO_BATCH_1.md](MODULE_ISSUE_PORTFOLIO_BATCH_1.md)
- [docs/MODULE_ISSUE_PORTFOLIO_BATCH_2.md](MODULE_ISSUE_PORTFOLIO_BATCH_2.md)
- [ROADMAP.md](../ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](../FUTURE_ENHANCEMENTS.md)

## Operational rule

Each module issue remains actionable only when it includes a clear current-state baseline, an evidence-backed target, and explicit validation gates. This catalog is intended to keep the same standard across the whole module graph and to avoid vague, non-actionable issue language.
