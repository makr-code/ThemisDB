# ThemisDB as Copilot Backend

## Executive Summary
ThemisDB should be integrated into VS Code/Copilot primarily as a knowledge and tool backend, not only as a text generation endpoint. The architecture is strongest when responsibilities are separated:
- Copilot/Agent UI as orchestrator
- ThemisDB as Graph-RAG retrieval and tool platform
- Local/remote model runtime (llama.cpp-based and compatible models) as generation plane

This model reduces cloud token usage, improves grounding, and keeps the system replaceable at model level.

## Design Goal
Enable code-assistant workflows in VS Code where Copilot can:
1. Query ThemisDB over MCP tools/resources.
2. Retrieve grounded context from graph, vector, and hybrid search.
3. Re-rank candidates before answer generation.
4. Use local model inference where possible and cloud selectively.
5. Improve domain quality over time via LoRA adapter lifecycle (train, validate, deploy, rollback).
6. Guarantee resilient storage and retrieval SLOs with explicit RAID + shard placement strategy.

## Current State (IST)

### 1. Native inference capability exists
ThemisDB already exposes llama.cpp plugin contracts and runtime integration:
- `LlamaCppPlugin` with load/unload, generation, embeddings, LoRA lifecycle, capability and perf metadata.
- Registrar integration with `LLMPluginManager`.
- Public API documented in `include/llama_cpp/README.md`.

Operationally relevant detail:
- `supports_function_call` is currently false in the llama.cpp plugin capability surface.
- Model loading and non-stub runtime depend on proper model path and LLM-enabled build/runtime path.

### 2. Retrieval and re-ranking capability exists
ThemisDB contains two relevant re-ranking paths:
- Search-level `LlmReranker` for top-N ranking blend with fallback semantics.
- RAG-level `CrossEncoderReranker` (heuristic and ONNX-backed profiles), input validation, and fail-closed behavior.

Hybrid retrieval components are present (vector + full-text + graph-related modules), and architecture docs position hybrid search as a first-class subsystem.

### 3. MCP integration path exists but is not default-on
Feature-flag documentation shows MCP server components and build path:
- `THEMIS_ENABLE_MCP` (default OFF in architecture flags docs)
- MCP server/resource/tool handlers in server module.

Meaning: MCP is architecturally present, but in many deployments not yet the guaranteed default access plane.

### 4. Copilot-side router extension exists
`tools/copilot-ollama-router` already provides:
- Chat participant routing (`@ollama`) and delegation policies.
- Auto-routing to local model or cloud Copilot.
- ThemisDB-specific routing heuristics.

This already enables practical cost optimization and staged local-first adoption.

### 5. Known maturity caveat in embedding-related paths
Audit documentation highlights critical implementation risks in parts of embedding-dependent LoRA/training flow (for example EmbeddingProvider gaps in specific audited scope). This does not invalidate all retrieval features, but it is a readiness signal for strict capability verification per pipeline.

## Target State (SOLL)

### Architectural Principle
Treat ThemisDB as a retrieval and tool substrate for coding assistants, with model runtime as a pluggable concern.

### SOLL Layering
1. Agent/UI plane
- VS Code Copilot Chat/Agents remains user-facing orchestration surface.

2. Tool plane (MCP)
- ThemisDB exposes read-safe MCP tools/resources for code intelligence and project knowledge.

3. Retrieval plane (Graph-RAG)
- Hybrid retrieval combines lexical, vector, and graph expansion.
- Re-ranking stage normalizes and orders top candidates.

4. Generation plane
- Local or remote llama.cpp-compatible models for most coding tasks.
- Cloud models for security-critical or complex architecture tasks only.

5. Adaptation plane (LoRA)
- Domain adapters for coding tasks (C++, CMake, test generation, architecture Q&A).
- Policy-controlled adapter routing by language/module/intent.
- Adapter registry with versioning, staged rollout, and rollback.

6. Data durability plane (RAID + Sharding)
- RAID-backed shard nodes for local fault tolerance.
- Cross-shard replication for node/zone failures.
- Shard-aware retrieval fan-out with deterministic merge/re-rank.

### Reference Flow
1. User asks coding question in Copilot.
2. Copilot agent invokes MCP tool in ThemisDB.
3. ThemisDB runs hybrid retrieval and optional graph expansion.
4. Re-ranking selects top grounded snippets/entities.
5. Copilot/model generates answer constrained by retrieved context.
6. Audit trail stores tool calls and retrieval metadata.

## SOLL vs IST Comparison

| Area | IST | SOLL | Gap |
|---|---|---|---|
| Inference | llama.cpp plugin stack available | Production local model runtime profiles per task class | Runtime policy and profile standardization |
| MCP | Components available via feature flag | MCP as primary assistant integration channel | Default-on profile + hardened tool contracts |
| Graph-RAG | Building blocks distributed across modules | Unified coding-assistant retrieval pipeline | Dedicated code-knowledge schema and retrieval orchestration |
| Re-ranking | LLM and cross-encoder rerankers exist | Tiered reranking policy (fast + accurate path) | Routing policy and objective evaluation metrics |
| LoRA adaptation | LoRA APIs and lifecycle hooks exist | Closed-loop adapter improvement pipeline with canary promotion | Training quality gates, adapter governance, and online rollout policy |
| RAID + sharding | Distributed and storage capabilities exist but not assistant-specific topology contract | Explicit shard placement, RAID profile, and failover SLOs for assistant workloads | Topology blueprint, capacity model, and recovery playbooks |
| Safety | AI safety architecture planned in docs | Enforced read/write tool classes with approval gates | Operational enforcement completion |
| Copilot Integration | Router extension available | Unified local-first routing + ThemisDB MCP grounding | End-to-end configuration and governance templates |

## Deep-Dive: Capability Analysis

### A. Inference inside ThemisDB
Strengths:
- Native plugin API for model lifecycle, inference, embedding, LoRA operations.
- Registry and manager integration allows controlled model registration.

Constraints:
- Function calling capability is currently limited in llama.cpp plugin surface.
- Operational mode can degrade to stub-like behavior without full runtime enablement.

Implication:
- Use ThemisDB runtime for generation where deterministic local control is required.
- Keep tool-calling orchestration at Copilot/MCP layer unless model capability is explicitly verified.

### B. MCP as integration contract
Strengths:
- Clear server/tool/resource handler structure already documented.
- Natural fit for agent tool calls from Copilot.

Constraints:
- Not default enabled in all builds.
- Requires strict policy classes for destructive vs non-destructive operations.

Implication:
- First production milestone should expose read-only MCP tools only.

### C. Graph-RAG for coding assistance
Strengths:
- ThemisDB multi-model substrate (graph + vector + document + query engine).
- Existing process-level Graph-RAG concept proves architectural intent.

Target coding graph entities:
- Repository, module, file, symbol, test, build target, API endpoint, issue, commit.

Target relations:
- defines, calls, imports, tests, depends_on, owns, changes_with, documents.

Implication:
- Build an explicit coding graph schema as productized layer on top of existing engines.

### D. Re-ranking strategy
Strengths:
- Two-level reranking stack already available.
- CrossEncoderReranker includes strict input checks and secure model load semantics.

Recommended policy:
- Stage 1: fast heuristic or compact ONNX reranker for broad candidate set.
- Stage 2: optional LLM reranking for very small top-N if confidence remains low.

Implication:
- Cross-encoder should be default for latency and predictability.

### E. LoRA-based model improvement loop
Strengths:
- The inference/plugin surface already includes LoRA lifecycle operations.
- Adapter-oriented design allows task-specific specialization without replacing base models.

Target operating model:
1. Collect assistant feedback signals (accepted edits, rejected proposals, retrieval misses).
2. Build curated training slices per domain (for example `cpp-core`, `cmake-build`, `test-authoring`).
3. Train LoRA adapters offline with strict data and quality controls.
4. Validate against coding benchmarks (compile success, test delta, patch acceptance rate).
5. Deploy canary adapter to a subset of requests.
6. Promote or rollback based on objective metrics.

Adapter routing policy:
- Route by intent + language + repository area.
- Keep a safe default adapter and explicit rollback target.
- Never auto-promote adapters without regression gate.

Implication:
- LoRA is the main mechanism to continuously improve local assistant quality while keeping compute and model size manageable.

### F. RAID + sharding architecture for assistant workloads
Problem addressed:
- Coding-assistant retrieval is read-heavy, latency-sensitive, and must survive disk and node failures.

Recommended storage topology:
1. Node-local durability:
- Use RAID10 for hot retrieval shards (best mixed read/write latency + rebuild behavior).
- Use RAID6 for colder archival/index snapshots where capacity efficiency matters.

2. Cluster-level resilience:
- Replicate each shard across failure domains.
- Keep at least one replica in a different fault zone/rack.

3. Retrieval path:
- Scatter query to relevant shards.
- Perform local top-k preselection per shard.
- Merge globally and run final re-ranking centrally.

4. Recovery and operations:
- Define shard recovery SLO (RTO/RPO) for assistant index classes.
- Use snapshot + WAL policy per shard class.
- Test degraded mode (single disk failure, node loss, shard lag) regularly.

Implication:
- RAID handles media faults, sharding handles scale, replication handles node/zone faults; all three are required for production assistant reliability.

## Security and Governance Model

### Tool Classes
1. Read-only tools
- search_code_graph
- retrieve_symbol_context
- retrieve_related_tests
- trace_dependency_path
- query_docs_and_adrs

2. Write-safe tools (later phase)
- propose_patch_plan
- stage_validation_run

3. Destructive tools
- Not exposed to autonomous agent mode.
- Must require human approval and environment guard.

### Mandatory Controls
- Read-only enforcement in MCP layer.
- Human-in-the-loop for any write/destructive capability.
- Full audit trail: prompt hash, tools invoked, resources touched, result metadata.
- Environment isolation (dev/staging/prod).

## Phased Rollout Plan

### Phase 1: Local-first assistant baseline
- Enable copilot router policy and local model routing defaults.
- Publish standard model profiles (coding, reasoning, fallback).
- Define baseline adapter policy (default LoRA disabled or safe-default only).
- Acceptance: deterministic routing logs and stable local generation path.

### Phase 2: MCP read-only foundation
- Enable MCP build profile by default in assistant deployments.
- Implement and publish read-only tool catalog.
- Acceptance: all tools pass read-only policy tests and audit coverage.

### Phase 3: Coding Graph-RAG pipeline
- Implement explicit coding knowledge schema and ingestion pipeline.
- Wire hybrid retrieval + graph expansion + reranking.
- Add shard-aware retrieval fan-out and deterministic merge policy.
- Acceptance: retrieval quality benchmark improves over lexical baseline.

### Phase 4: LoRA improvement pipeline
- Implement adapter training/evaluation registry with benchmark gates.
- Add canary rollout and instant rollback workflow.
- Acceptance: measurable quality lift without regression in compile/test success.

### Phase 5: RAID + sharding production hardening
- Publish assistant-specific shard classes and RAID profiles.
- Validate recovery playbooks and degraded-mode behavior.
- Acceptance: storage/node fault drills meet RTO/RPO targets.

### Phase 6: Safety hardening and governance
- Enforce operation classes and approval gates.
- Add policy-driven execution guardrails and forensic logging.
- Acceptance: destructive simulation tests blocked by policy.

### Phase 7: Advanced optimization
- Add confidence-aware fallback policy (local -> cloud escalation).
- Add adaptive reranking and query rewrite loops.
- Acceptance: reduced cloud token usage with no regression in answer quality.

## Key Risks and Mitigations

1. Risk: Capability drift between documented and runtime model behavior.
- Mitigation: startup capability probes and continuous health checks.

2. Risk: MCP tools exposing unsafe operations.
- Mitigation: read-only default, explicit allowlist, approval workflow.

3. Risk: Retrieval quality inconsistency from partial embedding maturity.
- Mitigation: hard quality gates for embedding pipelines and reranking fallback strategy.

4. Risk: Tight coupling of model runtime and retrieval backend.
- Mitigation: strict separation of generation plane and knowledge plane.

5. Risk: Adapter drift or catastrophic LoRA regression.
- Mitigation: benchmark gates, canary rollout, automated rollback, and version-pinned fallback adapters.

6. Risk: Shard hot-spotting and RAID rebuild performance collapse.
- Mitigation: shard balancing policy, workload-aware placement, and rebuild-aware admission control.

## Source-Validated Deep Dive (Code Reality)

The following observations are derived directly from current source paths and interfaces.

### 1. Generation Plane (llama.cpp plugin) reality check

Observed implementation:
- `src/llama_cpp/llama_cpp_plugin.cpp` runs in two practical modes:
	- Real mode (when `THEMIS_LLM_ENABLED` and `loadModel()` succeeds)
	- Fail-closed/stub bridge mode (explicit errors when model is not loaded)
- `generate()` now fails closed when no model is loaded (`success=false`), except optional test stub mode.
- `generateDraftTokens()` and LoRA lifecycle APIs are present in the plugin surface.

Practical implication:
- This is good production behavior for backend reliability: "missing model" is no longer silently accepted.
- For Copilot grounding quality, this means retrieval can remain operational even while generation fails fast and observably.

### 2. Re-ranking Plane (cross-encoder) reality check

Observed implementation:
- `include/rag/reranker.h` + `src/rag/reranker.cpp` provide:
	- heuristic reranking path (always available)
	- ONNX model path (feature-flag dependent)
	- strict input guards (query/doc size and candidate count bounds)
	- model checksum checks and sidecar support
	- internal score caching

Practical implication:
- There is already a robust fail-closed default path for reranking.
- Latency profile is predictable in heuristic mode, with ONNX as optional accuracy upgrade.

### 3. Context Budgeting Plane reality check

Observed implementation:
- `src/rag/rag_context_assembler.cpp` computes context budget, ranks chunks deterministically, and optionally truncates the final chunk.
- Response budget is explicitly reserved (`min_response_tokens` and model-window-aware budgeting).

Practical implication:
- Prompt overflow risk is materially reduced.
- Deterministic tie-break behavior helps reproducibility and debugging in agent pipelines.

### 4. MCP Tool Plane reality check

Observed implementation:
- `src/server/mcp_server.cpp` has mature transport handling (stdio/SSE/WebSocket), tool/resource/prompt registries, AI safety guard bootstrapping, and audit logger integration points.
- `src/llm/mcp_tool_bridge.cpp` bridges MCP tools into AIOrchestrator `ToolRegistry` via JSON-RPC wrappers.

Practical implication:
- The tool plane is already technically strong enough for a read-first production rollout.
- Governance and operation-class policy need to be treated as deployment-critical, not optional hardening.

### 5. Retrieval + Vector/Shard scaling reality check

Observed implementation:
- ANN abstraction exists via `include/index/ann_index.h` with ScaNN and optional DiskANN adapter.
- HNSW defaults and runtime adaptation exist (`include/index/hnsw_production_defaults.h`, `src/index/hnsw_production_defaults.cpp`).
- Cross-shard fan-out/merge contracts are explicit in `include/sharding/sharding_interfaces.h`.
- Adaptive shard routing with scatter-gather fallback is available (`include/sharding/adaptive_shard_router.h`).

Practical implication:
- The architecture supports realistic scale-out patterns now.
- Biggest remaining gap is productized query orchestration and measurement discipline, not core primitive availability.

## Realistic Performance Envelope (Target vs. Code-Current)

The table below is a practical target profile for coding-assistant workloads, aligned with current code capabilities.

| Stage | Current Capability (from code) | Realistic Target (p95) | Notes |
|---|---|---|---|
| MCP request dispatch | Mature transport + handler stack | <= 15 ms | Excludes downstream query/model latency |
| Hybrid retrieval (vector + metadata/doc fetch) | Available primitives + ANN backends | <= 90 ms | Assumes warmed index + bounded candidate set |
| Cross-encoder rerank (heuristic) | Fully available, guarded | <= 20 ms (top-100) | Good default for local-first |
| Cross-encoder rerank (ONNX) | Optional path | <= 120 ms (top-100 CPU), <= 70 ms GPU | Requires model deployment discipline |
| Context assembly + budget enforcement | Deterministic + bounded | <= 10 ms | Mainly string/token-estimation cost |
| Local generation first token | Depends on model/runtime | 250-900 ms | Model-size and hardware dependent |
| End-to-end grounded answer | All pieces present | 0.8-2.2 s | For most coding Q&A use-cases |

### Throughput assumptions for planning

- Single-node local-first baseline: 5-20 concurrent grounded requests before p95 latency degradation becomes obvious.
- Scale-out with shard fan-out + read replicas: 3x-8x retrieval throughput increase is realistic before orchestration overhead dominates.
- Rerank cost control: cap candidates pre-rerank (for example 100-200) to avoid nonlinear tail growth.

### Performance guardrails to enforce

1. Hard cap candidate count before reranking (already structurally supported by reranker input bounds).
2. Keep heuristic rerank as guaranteed fallback path.
3. Reserve response tokens explicitly (already implemented by context assembler).
4. Separate retrieval SLO from generation SLO in telemetry and alerting.

## Usefulness Assessment (where this architecture creates real value)

### High-value use-cases (immediate)

1. Grounded code explanation and impact analysis
- Why useful: retrieval + graph relations reduce hallucinated references.
- Expected impact: significantly fewer false file/symbol claims vs. plain LLM-only answers.

2. Build/test failure triage with local context
- Why useful: MCP tools can fetch schema/stats/query/doc context directly.
- Expected impact: faster mean-time-to-diagnosis and fewer context-switches to external docs.

3. Policy-constrained assistant operation
- Why useful: MCP operation classes + AI safety guard provide enforceable boundaries.
- Expected impact: lower risk of unintended destructive operations in agentic modes.

### Medium-value use-cases (after hardening)

1. Adapter-routed specialization (LoRA by domain)
- Benefit: quality lift in narrow domains (C++, CMake, graph query patterns).
- Risk: regression and drift without strict gates.

2. Confidence-aware local-to-cloud escalation
- Benefit: token-cost reduction while preserving quality on hard queries.
- Risk: requires robust confidence calibration and replay-based validation.

## Implementation Effort Estimate (realistic)

Assumption: 1 senior engineer-week ~= 5 focused implementation days including tests/docs.

| Work Package | Scope | Complexity | Effort (eng-weeks) | Risk |
|---|---|---|---|---|
| WP1 MCP read-safe production profile | default-on profile, allowlist, tests, docs | Medium | 2-3 | Low |
| WP2 Unified coding graph schema + ingest | repo/module/file/symbol/test entities + relations | High | 5-8 | Medium |
| WP3 Hybrid retrieval orchestrator | lexical+vector+graph fan-out + deterministic merge | High | 4-6 | Medium |
| WP4 Rerank policy + eval harness | fast/accurate routing, benchmark harness, SLO checks | Medium | 3-4 | Low-Medium |
| WP5 LoRA governance pipeline | dataset curation, eval gates, canary/rollback automation | High | 6-10 | High |
| WP6 Shard topology + reliability playbooks | shard classes, replica policy, failure drills, runbooks | Medium-High | 4-6 | Medium |
| WP7 Observability + cost control | stage-wise telemetry, error taxonomy, cloud-escalation metrics | Medium | 2-4 | Low |

Estimated total program (serial): 26-41 eng-weeks
Estimated total program (parallelized 3-4 engineers): 8-14 calendar weeks

## Recommended Delivery Sequence (value-first)

1. WP1 + WP4 first (fastest measurable quality/safety win).
2. WP2 + WP3 second (core Graph-RAG productization).
3. WP7 integrated throughout (no blind optimization).
4. WP6 before full production scale commitments.
5. WP5 last gate before broad autonomous-agent rollout.

## Acceptance Metrics (must be measured, not assumed)

### Quality
- Grounding precision@k for cited code artifacts.
- "Wrong file/symbol claim" rate per 1,000 answers.
- Human patch acceptance rate for assistant-generated changes.

### Performance
- Retrieval p50/p95/p99 split by stage.
- Rerank latency by mode (heuristic vs ONNX).
- End-to-end latency under mixed concurrency.

### Reliability and safety
- MCP tool error budget (5xx + policy denial clarity).
- Destructive-action block rate and false-positive rate.
- Recovery drill success against declared RTO/RPO objectives.

### Cost efficiency
- Local vs cloud token share over time.
- Cost per accepted assistant outcome.
- Marginal quality gain per adapter rollout.

## Practical Recommendation
Implement ThemisDB as the grounded retrieval + MCP tool backend first, and keep model generation pluggable. This gives the fastest path to a production-credible Copilot coding assistant with measurable cost reduction and stronger factual grounding.
