---
Author: makr-code
Created: 2026-11-11
Last Updated: 2026-11-12
Status: draft
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: makr-code
---

> Exact model backend not verifiable from final document text alone.

# Core Provider/Consumer Integration: Implementation Task

## Status and purpose

This document is the implementation contract for the follow-up work requested
after PRs [#8051](https://github.com/makr-code/ThemisDB/pull/8051) and
[#8055](https://github.com/makr-code/ThemisDB/pull/8055). PR #8051 is merged
and provides the existing RAG provider, typed retrieval request/context/result,
evidence candidate, and shared llama.cpp prompt-preparation contracts. PR #8055
is also merged, but its diff only refreshed generated developer-wiki files; it
did not deliver the Core integration described in its summary.

**This is a task definition, not evidence that the implementation is complete.**
The implementation PR must change production source, relevant canonical
documentation, and focused tests, and must leave the checkboxes below open until
each criterion has been verified.

## Required outcome

Provide one internal, typed retrieval path that uses ThemisDB's existing
RocksDB-backed storage and specialized indexes (relational, graph, geo, and
vector) and can be consumed by real HTTP, AQL, and YAML-agentic execution paths.
Retrieval must not depend on which endpoint asks for evidence. Inference is
optional and remains a separate consumer of prepared evidence.

Keep the ABI/API surface shallow: reuse the contracts delivered by #8051 and
existing storage/index/query types. Do not add a second RAG manager, a universal
data bus, another factory/manager layer, or a stack of endpoint-specific
wrappers.

## Current source facts to revalidate before implementation

- `QueryEngineBuilder` accepts `IStorageEnginePtr` and `IIndexManagerPtr`, and
  constructs the interface-based `QueryEngine`. The constructor stores those
  interfaces, while established retrieval paths still use the concrete
  RocksDB/index manager members. The interface-only builder path therefore
  must not be presented as a working replacement for the production backend
  composition without fixing and testing that boundary.
- `HttpServer`'s direct `POST /api/v1/llm/rag` route authorizes the requested
  collection, then creates a request-local concrete `query::QueryEngine` using
  the live server-owned storage and index managers (`storage_`,
  `secondary_index_`, `graph_index_`, `vector_index_`, and `spatial_index_`).
  It creates a local `RAGProvider` over that engine and passes a
  `RetrievalExecutionContext` containing the request deadline, tenant from the
  authenticated request context, and authorized-collection predicate. This is
  the concrete HTTP composition; it is not evidence that the builder's
  interface-only composition is production-ready, nor does it establish general
  row-level authorization or propagate caller identity to other consumers.
- `LLMAQLHandler::executeRAG()` uses its injected query engine and passes its
  cancellation token, configured retrieval deadline, and configured
  collection-check predicate to `RAGProvider`. A source-tree search finds no
  production call site for `executeRAG()` that supplies trusted caller context;
  the checks are cooperative and cannot interrupt a blocking backend operation.
  The method's injection API is not evidence of live startup/request wiring.
  Trace actual composition callers before claiming AQL is an integrated consumer.
  The YAML/MCP boundary is similarly not implied by an orchestration API.
- `VectorIndexSinkAdapter` writes vector entities to `VectorIndexManager`.
  New writes also persist chunk text in the dedicated relational
  `ingestion_sources` collection keyed by the exact `VectorRecord::chunk_id`.
  The vector entity points to that exact key and retains the source-file SHA-256
  as provenance. RAG resolves the record through one exact RocksDB lookup and
  validates the stored source-file identity before replacing the copied snippet.
  Missing, mismatched, or empty source records are rejected. Existing vectors
  written with the older `ingestion_vector` reference retain their prior snippet
  behavior; the new collection is additive and requires no key migration. The
  sink now requires a `SecondaryIndexManager` sharing the RAG retrieval database,
  which is a source/ABI change for its callers.
- ContentManager stores chunk text under `chunk:<id>` and its vector entity under
  `chunks:<id>`. The current work puts the chunk text and an exact
  `source_ref_key=chunk:<id>` on that vector entity, with `chunks` as the source
  collection and `content_id` retained as the owner identity in the vector
  entity. QueryEngine only hydrates a vector entity from its
  vector key when the initialized vector-index namespace matches the query
  collection; canonical relational and legacy table-key reads remain earlier
  lookup paths. The current RAG provider additionally resolves typed
  `content_chunk` references by one exact `chunk:<id>` RocksDB read and verifies
  stored chunk ID, owner `content_id`, and any supplied source version;
  missing/mismatched/stale sources are rejected rather than replaced by the
  vector's copied text. This private bridge adds no public API or layout change.
  Normal ingestion now has the equivalent exact-key source dereference through
  `ingestion_sources`, validating `source_file_id` against vector provenance.
- `RAGSourceReference` is an optional typed locator on `RAGCandidate`.
  `makeRAGCandidate` parses complete, nonempty source-reference type, collection,
  and key fields and materializes text from `content`, `text`, `body`, or
  `text_snippet`; the helper performs no source lookup.
- `search`, `LayeredRetrievalOrchestrator`, `ProcessGraphRag`, and
  `ethics_ai/RAGContextEngine` have specialized behavior. Preserve their domain
  policies; this task does not migrate all of them.

Recheck these facts against the current source, not generated wiki summaries.
Update or narrow this task if source evidence differs before coding.

## Verified incremental work in the current implementation diff

The MCP server now retains the graph index manager for the lifetime of the
`QueryEngine` that borrows it, and a focused MCP AQL graph traversal regression
covers the attached query path, including database reattachment after graph
data is written. Reattachment releases the old query/index borrowers before
replacing their database. This fixes that specific lifetime defect; it does not
complete server-wide provider composition or MCP retrieval authorization.

The direct HTTP RAG route now carries its configured live request timeout into
`RetrievalExecutionContext` from request dispatch through query embedding and
provider retrieval; an expired retrieval deadline returns HTTP 504. The deadline
is cooperative and cannot interrupt a blocking embedding or backend call. The
route now also builds an equality filter from the validated bearer-token tenant
claim for vector retrieval; it does not trust a caller-supplied tenant header.
The claim is passed through `RetrievalExecutionContext`; the provider filters
materialized relational, content, and vector evidence, and injects the tenant
predicate before vector search. Graph retrieval passes a request-scoped vertex
policy to QueryEngine, which checks each visited entity's `tenant_id` before
returning it or expanding that vertex. Missing, malformed, or mismatched tenant
values return access denied. This does not populate trusted tenant context for
AQL/YAML server consumers, establish general row-level policy outside these
retrieval paths, propagate caller identity, or add HTTP cancellation. Focused
provider regressions cover foreign/missing tenants, indexed top-1 behavior, and
same-tenant/foreign/tenantless graph vertices. A concurrent shared-provider
regression source also checks that distinct vector tenant contexts return only
their own evidence. These tests have not been built or executed. Adding the
context field is source-compatible for ordinary member assignment/default
construction but changes the struct layout; binary consumers must rebuild.

The current source diff adds `RAGSourceReference` and candidate parsing for exact
source-reference fields; persists normal ingestion chunk text in
`ingestion_sources` and resolves it by exact key plus source-file identity;
stores ContentManager chunk text and its exact `chunk:<id>` locator on the
corresponding vector entity; gates vector-key hydration on an exact initialized
vector-index/query namespace match; propagates the AQL RAG timeout
as a cooperative retrieval deadline; and supports a per-call collection
authorization predicate, wired from the AQL checker and direct HTTP route.
QueryEngine applies that predicate before graph vertex loading and expansion.
ContentManager `content_chunk` references now resolve exact source records and
reject missing, mismatched, or stale chunk sources. Focused test-source changes
cover source materialization, including a sink-only vector write that retrieves
persisted source text without a test-only duplicate write; source corruption;
matching-provenance source records with whitespace-only text;
deadline expiry after a blocking QueryEngine call with both matching and empty
results; cancellation after a blocking QueryEngine call with no matches; an
incomplete/malformed persisted ingestion-source record returning
`ERR_STORAGE_CORRUPTION`, covering both invalid JSON and a well-formed source
record with an array, null, or string root; missing required content; or
identity/content stored with non-string types, plus a truncated object header,
truncated field-name payload, unsupported type tag, wrong binary root container,
trailing bytes after a valid binary object, and truncated required source text;
and request-isolated collection authorization. These C++ tests have not been
built or executed.
For YAML-configured agentic modes, `AIOrchestrator::runAgentic()` now routes
retrieval-enabled modes through its existing `runRag()` pipeline before
preserving the existing tool-call parsing and dispatch. This reuses the typed
provider and the per-call `RetrievalExecutionContext`; it adds no workflow
executor and does not populate authorization from YAML.
`AIOrchestrator` now uses only the typed
`OrchestratorContext::retrieval_execution.tenant_id` for tenant-scoped adapter
selection, budget-map lookup, and cost attribution; the untrusted
`OrchestratorContext::extra["tenant"]` value is ignored. This prevents request
metadata or YAML extras from choosing a tenant identity, but does not itself
establish a trusted caller context at MCP/AQL/YAML startup.
Tenant budget overrides and per-tenant budget maps now use the explicit
`OrchestratorContext::rag_cost_budget` typed field instead of `extra` JSON.
That field is server-policy input and must not be copied from request metadata
or YAML. Non-finite and non-positive values are rejected. The new context field
changes `OrchestratorContext` layout; binary consumers must rebuild. Existing
server startup does not yet populate this policy field from trusted configuration.
The MCP `llm_orchestrate` bridge currently constructs its context from tool
arguments without a trusted caller authorization context. In
`McpServer::toolLLMOrchestrate()`, a retrieval-enabled mode returns an error
stating that retrieval authorization context is unavailable before the
argument-derived `OrchestratorContext` is run. MCP retrieval therefore remains
fail-closed; wiring authenticated request context remains required.
The focused target has not been built or executed: diagnostic CMake configuration
stopped at the required `fmt` dependency, which is not installed in the current
environment.

This is partial implementation progress, not completion of this task or its
acceptance criteria. Normal ingestion source rows contain the canonical chunk
text carried by `VectorRecord`; this source is validated against the originating
file SHA-256, while no independent source-file/version record is available at
the sink boundary. Existing vectors with the old `ingestion_vector` marker
retain their prior snippet behavior. The new sink constructor requires callers
to provide a `SecondaryIndexManager` on the same retrieval database.

Still pending: trace and verify real AQL/YAML startup and request wiring;
complete production composition; complete caller/tenant/row-level authorization
and full context propagation (the direct HTTP vector filter is route-specific and
does not establish general row-level policy);
verify application startup configures the orchestrator with the actual server
QueryEngine and supplies trusted authorization context, including a safe MCP
caller-context path; complete source materialization/compatibility and remaining
acceptance cases; and run and record the required focused tests and registration
checks. The evidence-backed current-source role map follows below; its presence
does not complete any runtime acceptance criterion. Keep every acceptance
checkbox below open until its criterion is independently verified.

## Current direct-`src/` Provider / Consumer / Helper role map

Provider, Consumer, and Helper describe behaviors, not exclusive class
categories. One directory or component may have several roles, and this map
records only what the cited source demonstrates. A role not listed is not a
claim that the behavior is impossible elsewhere in that directory. The table
covers every immediate directory currently present under `src/`; paths are
absolute repository paths so the evidence can be checked directly. It describes
source-tree behavior, not a claim that every file is compiled or enabled in a
particular build.

Build ownership was checked separately from behavioral roles. Root
`cmake/CMakeLists.txt` assembles `themis_core` and includes feature fragments;
`cmake/ModularBuild.cmake` declares explicit module source lists and conditional
module targets. Some directories additionally have local `CMakeLists.txt`
files, but those may be IDE/standalone/test entry points and do not alone prove
membership in a given production build. Conversely, many root-assembled sources
have no directory-local build file. Optional features and edition/build flags
may exclude sources. The three documentation-only/no-C++-source directories
below have no runtime C++ behavior to assign. For example,
`/home/runner/work/ThemisDB/ThemisDB/src/access_model/CMakeLists.txt` explicitly
says its sources are included in `themis_core` via the root CMake file and
provides a standalone-target fallback; this demonstrates why a local CMake file
is not, by itself, the authoritative production ownership declaration.

| Immediate `src/` directory | Observed role behavior and representative source evidence |
|---|---|
| `acceleration` | **Provider / Helper:** registers compute backends through `BackendRegistry::registerBackend` in `/home/runner/work/ThemisDB/ThemisDB/src/acceleration/backend_registry.cpp`; dispatches through `AiHardwareDispatcher::dispatch` in `/home/runner/work/ThemisDB/ThemisDB/src/acceleration/ai_hardware_dispatcher.cpp`. |
| `access_model` | **Provider / Helper:** coordinates cache/storage tier access via `AccessCoordinatorImpl` in `/home/runner/work/ThemisDB/ThemisDB/src/access_model/access_coordinator.cpp`; records and summarizes access latency with `LatencyHistogram::record` / `percentile` in `/home/runner/work/ThemisDB/ThemisDB/src/access_model/access_metrics.cpp`. |
| `ai` | **Consumer / Provider:** `CAIEthicsIntegration::evaluate` in `/home/runner/work/ThemisDB/ThemisDB/src/ai/cai_ethics_integration.cpp` integrates an AI capability with ethics evaluation; `AIPluginGenerator` in `/home/runner/work/ThemisDB/ThemisDB/src/ai/ai_plugin_generator.cpp` provides plugin-generation behavior. Scope is limited to these named integrations, not all AI orchestration. |
| `ai_working` | **No runtime-source role:** current immediate contents are Markdown working snapshots; no C/C++ runtime source/header was found. These materials are historical/evidence artifacts, not canonical current-release or security sources. |
| `analytics` | **Provider / Consumer:** `AnalyticsEngine` in `/home/runner/work/ThemisDB/ThemisDB/src/analytics/analytics_engine.cpp` exposes analytics operations; `AggregationResult` in `/home/runner/work/ThemisDB/ThemisDB/src/analytics/aggregation.cpp` represents aggregation output. This does not imply a single analytics pipeline or a specific storage backend for every operation. |
| `api` | **Provider / Helper:** `FederationAdminHandler::getStats` in `/home/runner/work/ThemisDB/ThemisDB/src/api/federation_admin_handler.cpp` supplies an API-handler operation; `ApiErrorTaxonomy::toErrorCode` in `/home/runner/work/ThemisDB/ThemisDB/src/api/api_transport_policy.cpp` maps transport errors. |
| `aql` | **Provider / Consumer / Helper:** `LLMAQLHandler::executeRAG` in `/home/runner/work/ThemisDB/ThemisDB/src/aql/llm_aql_handler.cpp` implements an injectable retrieval operation; the handler consumes an injected QueryEngine/provider and AQL parsing/execution services. Its production call-site/trusted-context gap is stated above; method presence is not startup wiring. |
| `auth` | **Provider / Consumer:** `ApiKeyAuthenticator::addCredential` / `removeCredential` in `/home/runner/work/ThemisDB/ThemisDB/src/auth/api_key_authenticator.cpp` supply credential-authentication behavior; authentication consumes presented credentials and configured state. |
| `base` | **Provider / Helper:** `BufferPool::preallocateSlab` in `/home/runner/work/ThemisDB/ThemisDB/src/base/buffer_pool.cpp` provides reusable memory allocation; this is foundational utility behavior rather than the Core RAG provider. |
| `cache` | **Provider / Helper:** `BoundedLRUCache::get` / `put` in `/home/runner/work/ThemisDB/ThemisDB/src/cache/bounded_lru_cache.cpp` provide cache operations; `AdaptiveQueryCache::generateFingerprint` in `/home/runner/work/ThemisDB/ThemisDB/src/cache/adaptive_query_cache.cpp` derives cache keys. |
| `cdc` | **Provider / Consumer:** `CDCMaterializedViewMaintainer::createView` / `dropView` in `/home/runner/work/ThemisDB/ThemisDB/src/cdc/cdc_materialized_view.cpp` expose change-driven maintenance; CDC behavior consumes change/sequence information from its upstream source. |
| `chaos` | **Helper:** `FaultInjector::makeKey` and `FaultInjector::faultTypeName` in `/home/runner/work/ThemisDB/ThemisDB/src/chaos/chaos_framework.cpp` implement fault-injection support. A source file alone does not establish which test/build profiles enable it. |
| `chimera` | **Provider / Consumer:** `MongoDBAdapter::connect` in `/home/runner/work/ThemisDB/ThemisDB/src/chimera/mongodb_adapter.cpp` supplies adapter connection behavior; the adapter consumes an external database endpoint. Adapter presence does not establish production activation. |
| `config` | **Provider / Helper:** `ConfigEncryptedBlob::toJson` / `fromJson` in `/home/runner/work/ThemisDB/ThemisDB/src/config/config_encrypted_store.cpp` serialize configuration payloads; `ConfigAuditLog` in `/home/runner/work/ThemisDB/ThemisDB/src/config/config_audit_log.cpp` records configuration actions. |
| `content` | **Provider / Consumer:** `ArchiveProcessor::detectFormat` / `IContentProcessor::extract` in `/home/runner/work/ThemisDB/ThemisDB/src/content/adapters/archive_extractor_adapter.cpp` provide content extraction behavior and consume archive bytes. This is distinct from RAG's authoritative source-materialization contract. |
| `core` | **Helper / Provider (aggregate, path-dependent):** `AdapterRegistry::hasAdapter` / `setTrustPolicy` in `/home/runner/work/ThemisDB/ThemisDB/src/core/concerns/adapter_registry.cpp` show adapter-registry behavior; `OpenTelemetryTracerAdapter::flush` in `/home/runner/work/ThemisDB/ThemisDB/src/core/adapters/otel_tracer.cpp` adapts telemetry. `core/` is cross-cutting infrastructure, not one uniform provider. |
| `distributed_knowledge` | **Provider / Consumer:** `GossipAdapterPublisher::announce` in `/home/runner/work/ThemisDB/ThemisDB/src/distributed_knowledge/adapter_capability_announcement.cpp` publishes adapter capability information; `CrossShardFeedbackSync::publishFeedback` in `/home/runner/work/ThemisDB/ThemisDB/src/distributed_knowledge/cross_shard_feedback_sync.cpp` consumes/publishes shard feedback. |
| `distributed_tensor` | **Provider / Consumer:** `DefaultDistributedTensorPlanner::plan_tensor_retrieval` in `/home/runner/work/ThemisDB/ThemisDB/src/distributed_tensor/distributed_planner.cc` plans retrieval; its availability inputs are consumed to construct a plan. `ArtifactManifest::add_shard_placement` in `/home/runner/work/ThemisDB/ThemisDB/src/distributed_tensor/artifact_manifest.cc` records placement evidence. |
| `document` | **Provider / Consumer:** `StoreBackedRoundTripEditor::beginRelay` / `saveInteraction` in `/home/runner/work/ThemisDB/ThemisDB/src/document/round_trip_editor.cpp` implements a store-backed editing workflow; it consumes and writes interaction data. This small directory is not a general document-storage contract. |
| `ethics_ai` | **Provider / Consumer / Helper:** `RAGContextEngine` is declared in `/home/runner/work/ThemisDB/ThemisDB/src/ethics_ai/rag_context_engine.h` for specialized ethics/RAG context behavior; `ArgumentStore` in `/home/runner/work/ThemisDB/ThemisDB/src/ethics_ai/argument_store.cpp` exposes argument storage. Preserve this domain policy; it is not the common Core RAG provider. |
| `evaluation` | **Provider / Helper:** `AblationReport::bestByRecall` / `bestByNdcg` in `/home/runner/work/ThemisDB/ThemisDB/src/evaluation/src/ablation_framework.cc` summarizes evaluation results. The source establishes evaluation machinery, not that benchmark gates ran or measured claims are current. |
| `execution` | **Provider / Consumer:** `QueryScheduler::enqueue` / `dequeue` in `/home/runner/work/ThemisDB/ThemisDB/src/execution/query_scheduler.cpp` schedules query work; it consumes submitted tasks. |
| `exporters` | **Provider / Consumer:** `ArrowIPCExporter::exportEntities` in `/home/runner/work/ThemisDB/ThemisDB/src/exporters/arrow_ipc_exporter.cpp` emits an export format; exporters consume supplied query/data results. |
| `failover` | **Provider / Consumer:** `AutoFailoverManager::start` / `stop` in `/home/runner/work/ThemisDB/ThemisDB/src/failover/auto_failover_manager.cpp` coordinates failover behavior from observed topology/health inputs. Source presence does not imply an active production failover policy. |
| `geo` | **Provider / Consumer:** `BoostCpuExactBackend` in `/home/runner/work/ThemisDB/ThemisDB/src/geo/boost_cpu_exact_backend.cpp` provides a spatial backend; spatial query inputs are consumed by that backend. Build inclusion is option/target-dependent. |
| `governance` | **Provider / Consumer:** `RightToKnow::evaluate` in `/home/runner/work/ThemisDB/ThemisDB/src/governance/ccpa_rules.cpp` evaluates a compliance rule; it consumes subject/request data. |
| `gpu` | **Provider / Helper:** `GPUAdminAPI::getStatsJson` in `/home/runner/work/ThemisDB/ThemisDB/src/gpu/admin_api.cpp` exposes GPU administration/status behavior; this does not establish that a GPU backend is available or enabled. |
| `graph` | **Provider / Helper:** `AdjacencyList::add_vertex` / `has_vertex` in `/home/runner/work/ThemisDB/ThemisDB/src/graph/adjacency_list.cpp` supplies graph data operations; `LocalShardGraphExecutor` in `/home/runner/work/ThemisDB/ThemisDB/src/graph/distributed_graph.cpp` executes a graph operation against shard-local state. |
| `image_analysis` | **Provider / Consumer:** `TesseractOCRPlugin::getInfo` in `/home/runner/work/ThemisDB/ThemisDB/src/image_analysis/tesseract_ocr_plugin.cpp` supplies an OCR plugin; OCR consumes image inputs. Plugin/build registration remains configuration-dependent. |
| `importers` | **Provider / Consumer / Helper:** `AdaptiveImportOptimizer::optimizeImportPlan` in `/home/runner/work/ThemisDB/ThemisDB/src/importers/adaptive_import.cpp` prepares an import plan; importer paths consume external source data and provide import operations. This is not proof every connector is enabled. |
| `index` | **Provider / Consumer:** `QueryPatternTracker` in `/home/runner/work/ThemisDB/ThemisDB/src/index/adaptive_index.cpp` provides index-related pattern tracking; the vector index implementations under this directory consume indexed records/query vectors. Exact storage and index wiring is path-specific. |
| `ingestion` | **Provider / Consumer:** `AdaptiveRateLimiter::tryAcquireToken` in `/home/runner/work/ThemisDB/ThemisDB/src/ingestion/adaptive_rate_limiter.cpp` supplies intake throttling; ingestion pipelines consume source records and produce downstream writes. This is not proof of end-to-end ingestion wiring. |
| `llama_cpp` | **Provider / Consumer:** `LlamaCppPlugin::loadModel` / `unloadModel` in `/home/runner/work/ThemisDB/ThemisDB/src/llama_cpp/llama_cpp_plugin.cpp` supplies a llama.cpp plugin lifecycle and consumes model configuration/artifacts. Runtime availability is build/config dependent. |
| `llm` | **Provider / Consumer / Helper:** `AIOrchestrator::runRag` in `/home/runner/work/ThemisDB/ThemisDB/src/llm/ai_orchestrator.cpp` consumes a typed retrieval context/provider and supplies orchestration; `ActiveVRAMAllocator::allocate` in `/home/runner/work/ThemisDB/ThemisDB/src/llm/active_vram_allocator.cpp` is a separate inference-resource helper. |
| `llm_l_wiki` | **Uncertain / narrow scope:** `WikiWorkspaceOrchestrator` in `/home/runner/work/ThemisDB/ThemisDB/src/llm_l_wiki/wikipedia/wiki_workspace_orchestrator.cpp` identifies a wiki-workspace workflow only; its name/path does not justify assigning it a general Core retrieval or model-provider role. |
| `llm_streaming` | **No runtime-source role in this directory:** only Markdown and `.gitkeep` were found, with no C/C++ implementation. A separately implemented helper is `StreamingHandler` in `/home/runner/work/ThemisDB/ThemisDB/src/llm/streaming_handler.cpp`; do not attribute that code to this documentation-only directory or treat its docs as runtime evidence. |
| `llm_wiki` | **Provider / Helper (scoped):** `ProcessPolicyManager::loadFromYaml` in `/home/runner/work/ThemisDB/ThemisDB/src/llm_wiki/process_policy_manager.cpp` loads wiki-process policy; `PluginStatus::PermissionDenied` in `/home/runner/work/ThemisDB/ThemisDB/src/llm_wiki/edition_gate.cpp` is an edition-gate result. This is not a general-purpose retrieval provider. |
| `maintenance` | **Provider / Consumer:** `DatabaseMaintenanceOrchestrator` in `/home/runner/work/ThemisDB/ThemisDB/src/maintenance/database_maintenance_orchestrator.cpp` coordinates maintenance work; it consumes schedules, locks, and database state. |
| `metadata` | **Provider / Consumer:** `CatalogExporter::publishSchema` / `publishTable` in `/home/runner/work/ThemisDB/ThemisDB/src/metadata/catalog_exporter.cpp` publishes catalog metadata; it consumes schema/table descriptions. |
| `network` | **Provider / Helper:** `AdaptiveCircuitBreaker::shouldAllow` in `/home/runner/work/ThemisDB/ThemisDB/src/network/adaptive_circuit_breaker.cpp` provides network-operation protection; `BbrCongestionController::applyConfig` in `/home/runner/work/ThemisDB/ThemisDB/src/network/bbr_congestion_control.cpp` assists transport control. |
| `observability` | **Provider / Consumer:** `AdvancedMetrics::recordSummary` / `getSummary` in `/home/runner/work/ThemisDB/ThemisDB/src/observability/advanced_metrics.cpp` provides metrics behavior and consumes measurements; it does not itself prove external telemetry export. |
| `onnx_clip` | **Provider / Consumer:** `ONNXClipPlugin` in `/home/runner/work/ThemisDB/ThemisDB/src/onnx_clip/onnx_clip_plugin.cpp` provides a CLIP/ONNX plugin path and consumes image/model inputs. Optional registration/build remains a separate question. |
| `performance` | **Helper / Provider:** `AdaptiveQueryCompiler` in `/home/runner/work/ThemisDB/ThemisDB/src/performance/adaptive_query_compiler.cpp` supplies query optimization/compilation behavior; `AdvancedCacheManager` in `/home/runner/work/ThemisDB/ThemisDB/src/performance/advanced_cache_manager.cpp` supplies cache management. |
| `plugins` | **Provider / Consumer:** `PluginHealthMonitor::startMonitoring` in `/home/runner/work/ThemisDB/ThemisDB/src/plugins/plugin_health_monitor.cpp` supplies plugin health monitoring; registry/loader paths consume plugin metadata and implementations. Presence does not imply a particular plugin is installed. |
| `process` | **Provider / Consumer / Helper (specialized):** `BpmnSerializer` in `/home/runner/work/ThemisDB/ThemisDB/src/process/bpmn_serializer.cpp` provides serialization; `ProcessGraphRag::buildKnowledgeGraph` in `/home/runner/work/ThemisDB/ThemisDB/src/process/process_graph_rag.cpp` is specialized process-graph retrieval behavior. Do not fold its policies into generic RAG without source-level review. |
| `projects` | **Provider / Consumer:** `CollaborationManager` in `/home/runner/work/ThemisDB/ThemisDB/src/projects/collaboration_manager.cpp` coordinates project collaboration; `InMemoryProjectAuditLog::record` in `/home/runner/work/ThemisDB/ThemisDB/src/projects/in_memory_project_audit_log.cpp` consumes and records project events. |
| `prompt_engineering` | **Helper / Consumer:** `ChainOfThoughtBuilder::addStep` in `/home/runner/work/ThemisDB/ThemisDB/src/prompt_engineering/chain_of_thought.cpp` prepares structured prompt material; `RAGPromptBuilder::build` in `/home/runner/work/ThemisDB/ThemisDB/src/prompt_engineering/rag_prompt_builder.cpp` formats supplied evidence. No storage lookup role is inferred. |
| `query` | **Provider / Consumer / Helper:** `AdaptiveJoinExecutor::executeJoin` in `/home/runner/work/ThemisDB/ThemisDB/src/query/adaptive_join.cpp` supplies query execution and consumes query inputs; optimizer/planner sources in this directory provide helper behavior. |
| `rag` | **Provider / Consumer / Helper:** `RAGProvider::retrieve` in `/home/runner/work/ThemisDB/ThemisDB/src/rag/rag_provider.cpp` provides typed evidence retrieval over QueryEngine; `adaptive_hybrid_router.cpp` consumes retrieval inputs and routes work. Other RAG components are specialized and do not imply that every consumer is wired. |
| `replication` | **Provider / Consumer / Helper:** `AsyncWalShipper` in `/home/runner/work/ThemisDB/ThemisDB/src/replication/async_wal_shipper.cpp` ships WAL data; `ThreeWayMergeResolver::resolve` in `/home/runner/work/ThemisDB/ThemisDB/src/replication/conflict_resolution.cpp` consumes competing values and resolves conflicts. |
| `retrieval` | **Uncertain / aggregate, specialized providers:** `DualReadValidator` in `/home/runner/work/ThemisDB/ThemisDB/src/retrieval/dual_read_validator.cpp` validates a dual-read comparison; `LoRAManifestStore::RuntimeConfig::loadFromYaml` in `/home/runner/work/ThemisDB/ThemisDB/src/retrieval/src/lora_package.cc` loads package runtime configuration. This mixed directory is not evidence of the common Core RAG path or one unified retrieval service. |
| `rpc_grpc` | **Provider / Consumer / Helper:** `GRPCServer::initialize` in `/home/runner/work/ThemisDB/ThemisDB/src/rpc_grpc/grpc_plugin.cpp` provides server setup; `CircuitBreaker::allowRequest` in `/home/runner/work/ThemisDB/ThemisDB/src/rpc_grpc/circuit_breaker.cpp` protects calls. Inclusion is feature/build dependent. |
| `scheduler` | **Provider / Consumer:** `DistributedTaskCoordinator::dispatchLeadershipCallback` in `/home/runner/work/ThemisDB/ThemisDB/src/scheduler/distributed_task_coordinator.cpp` dispatches coordinated work; `EventTrigger` in `/home/runner/work/ThemisDB/ThemisDB/src/scheduler/event_trigger.cpp` consumes configured events. |
| `scraper` | **Provider / Consumer:** `HttpScraperApiClient` in `/home/runner/work/ThemisDB/ThemisDB/src/scraper/scraper_api_client.cpp` provides an HTTP scraper client and consumes remote responses; `GovSourceCatalog::all` in `/home/runner/work/ThemisDB/ThemisDB/src/scraper/gov_source_catalog.cpp` exposes catalog entries. |
| `search` | **Provider / Consumer:** `AutocompleteEngine::suggest` in `/home/runner/work/ThemisDB/ThemisDB/src/search/autocomplete.cpp` provides search suggestions and consumes query/index data. `search` has specialized policies; it is not synonymous with the shared RAG provider. |
| `security` | **Provider / Consumer:** `AccessControl::authenticate` in `/home/runner/work/ThemisDB/ThemisDB/src/security/access_control.cpp` supplies an access-control decision path and consumes authentication inputs; use the relevant security SOT for current policy claims. |
| `server` | **Provider / Consumer / Helper:** `HttpServer`'s `/api/v1/llm/rag` route in `/home/runner/work/ThemisDB/ThemisDB/src/server/http_server.cpp` provides the concrete HTTP entry/composition path described above; server handlers consume Core services. `McpServer::toolLLMOrchestrate` in `/home/runner/work/ThemisDB/ThemisDB/src/server/mcp_server.cpp` currently rejects retrieval-enabled modes without trusted authorization context. |
| `sharding` | **Provider / Consumer / Helper:** `AdaptiveShardRouter` in `/home/runner/work/ThemisDB/ThemisDB/src/sharding/adaptive_shard_router.cpp` routes shard requests using capability information; `AdminAPI::registerTopologyHandler` in `/home/runner/work/ThemisDB/ThemisDB/src/sharding/admin_api.cpp` exposes shard administration. |
| `stable_diffusion` | **Provider / Consumer:** `SDPlugin` in `/home/runner/work/ThemisDB/ThemisDB/src/stable_diffusion/sd_plugin.cpp` supplies image-generation plugin behavior and consumes prompt/model inputs. Build/runtime availability is conditional. |
| `storage` | **Provider / Consumer:** `BackupManager::runScheduledBackupLoop` in `/home/runner/work/ThemisDB/ThemisDB/src/storage/backup_manager.cpp` provides backup operations over database state; storage implementations provide persistence and consume backend/storage operations. This directory's breadth does not mean every QueryEngine path uses interface-only storage injection. |
| `temporal` | **Provider / Consumer:** `BiTemporalTable::insertWithValidTime` in `/home/runner/work/ThemisDB/ThemisDB/src/temporal/bi_temporal.cpp` supplies temporal data operations and consumes valid-time records; temporal CDC paths integrate change input. |
| `tensor` | **Provider / Helper:** `TTDecompositionStrategy::compress` in `/home/runner/work/ThemisDB/ThemisDB/src/tensor/compression_strategy.cpp` supplies tensor compression; tensor adapter/repository paths bridge external storage/model capabilities. Build ownership is optional/target-specific. |
| `themis` | **Provider / Consumer / Helper:** `EditionManager::isFeatureAvailable` in `/home/runner/work/ThemisDB/ThemisDB/src/themis/edition_manager.cpp` provides feature gating; module-loader/resolver sources consume module metadata and dependencies. |
| `timeseries` | **Provider / Consumer:** `AggregateScheduler::start` in `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/aggregate_scheduler.cpp` schedules time-series aggregation; it consumes time-series records/configuration. |
| `toolbox` | **Helper / Provider:** `ContentFingerprinter::compute` in `/home/runner/work/ThemisDB/ThemisDB/src/toolbox/content_fingerprinter.cpp` provides fingerprinting as a helper; `ContentToolboxBridge` in `/home/runner/work/ThemisDB/ThemisDB/src/toolbox/content_toolbox_bridge.cpp` adapts toolbox/content behavior. |
| `training` | **Provider / Consumer:** `AdaLoRATrainingPipeline::train` in `/home/runner/work/ThemisDB/ThemisDB/src/training/adalora_training_pipeline.cpp` provides training behavior and consumes prepared samples/model capabilities. |
| `transaction` | **Provider / Consumer / Helper:** `BranchManager` in `/home/runner/work/ThemisDB/ThemisDB/src/transaction/branch_manager.cpp` provides branch operations; compensation-log paths consume transaction outcomes. `saga_plugin/` is a nested build subcomponent, not an additional immediate `src/` directory. |
| `updates` | **Provider / Consumer:** `BlueGreenDeployment::deployToStandby` in `/home/runner/work/ThemisDB/ThemisDB/src/updates/blue_green_deployment.cpp` provides a deployment operation and consumes deployment state/configuration. |
| `user_storage_encrypted` | **Provider / Consumer:** `GocryptfsBackend::initialize` in `/home/runner/work/ThemisDB/ThemisDB/src/user_storage_encrypted/gocryptfs_backend.cpp` provides encrypted user-storage backend behavior and consumes filesystem/key configuration. Optional target/build conditions apply. |
| `utils` | **Helper / Provider:** `BloomFilter::optimalBits` / `optimalHashCount` in `/home/runner/work/ThemisDB/ThemisDB/src/utils/bloom_filter.cpp` implements a reusable transformation/data structure; helpers here do not imply storage/index ownership. |
| `vector_search` | **No runtime-source role:** current contents are Markdown and `.gitkeep`; no C/C++ runtime source/header was found in the directory. Do not infer a runtime provider or canonical vector-search behavior from its documentation snapshots. |
| `voice` | **Provider / Consumer / Helper:** `NoiseSuppressor` in `/home/runner/work/ThemisDB/ThemisDB/src/voice/audio_preprocessing.cpp` supplies audio preprocessing and consumes audio frames; voice capability and backend availability are configuration-dependent. |
| `whisper` | **Provider / Consumer:** `WavAudioChunkReader::canRead` in `/home/runner/work/ThemisDB/ThemisDB/src/whisper/audio_chunk_reader.cpp` provides audio reading behavior; transcription paths consume audio/model inputs. The directory's source and target registration do not establish an enabled runtime model. |

This is a current-source inventory, not a design mandate or evidence of
production composition. For the Core retrieval question specifically,
`HttpServer` is the demonstrated HTTP consumer; AQL's injectable method has no
verified production caller supplying trusted context; retrieval-enabled MCP
orchestration is fail-closed without authenticated caller context. Do not infer
that a module is composed, authorized, built, or runtime-accepted solely because
its directory or class exists.

## Work packages

### A. Functional production composition

1. Make the required retrieval/query path use the actual RocksDB-backed storage
   and index instances that the server owns. A non-null interface pointer is
   not proof that query execution works.
2. Extend the existing bootstrap/composition surface minimally. Preserve
   ownership and shutdown order; do not create a second backend or introduce
   unsafe downcasts to make the builder appear generic.
3. Keep the remaining limitations of interface-based QueryEngine dependency
   injection explicit. A universal refactor of all QueryEngine execution
   methods is not required.

### B. Canonical source identity and materialization

1. Define an explicit typed source reference that connects an indexed entity to
   its authoritative source, collection, key, provenance, and any existing
   version/freshness information.
2. Implement the lookup path for normal vector ingestion and ContentManager
   chunks so a retrieved vector can be resolved to its actual stored text.
3. Preserve existing key schemas and on-disk compatibility, or provide an
   explicit migration. Do not guess keys using arbitrary prefix/colon
   heuristics, scan unrelated collections, or read an unvalidated vector
   namespace.
4. Do not treat embeddings, metadata, identifiers, or error messages as
   evidence text. Preserve existing error and missing-source semantics.

### C. Real consumer wiring and request context

1. Trace the actual startup and request paths for HTTP, AQL, and YAML-agentic
   consumers. Wire each to the shared typed retrieval provider/contract instead
   of implementing endpoint-local retrieval or materialization.
2. Carry caller authorization, tenant/collection scope, deadline, cancellation,
   budget, and trace context through the affected calls using existing typed
   contexts or narrowly scoped extensions.
3. Shared backend ownership must not imply shared mutable caller identity.
   Concurrent requests with different authorization contexts must remain
   isolated. Never accept credentials or ACLs from untrusted YAML.
4. Keep YAML as orchestration of existing domain operations. Do not introduce a
   second workflow executor or YAML language.

### D. Inference and helper boundaries

- Retrieval must work without a loaded model or GPU. The llama.cpp integration
  consumes already prepared evidence; it does not own document retrieval.
- Keep model lifecycle, GPU selection, flash-attention, KV-cache management,
  tokenization, and generation in the inference provider. These are not
  retrieval helpers and are not part of this task unless a directly caused
  regression must be fixed.
- Helpers remain pure transformations: no storage/index I/O, model calls, or
  retries. `RAGCandidate` remains typed evidence, not a general-purpose row,
  mutation, CDC event, or model-artifact contract.
- Propagate provider failures explicitly. Do not silently fall back to
  ungrounded generation or success-shaped empty data.

### E. Documentation and plan

Update the existing canonical Core/RAG documentation rather than publishing a
second architecture report. Keep implementation status separate from target
design and mark only verified work complete. Include:

- Provider, Consumer, and Helper roles as behaviors of components, not
  exclusive base classes; storage/index modules may both provide and consume.
- An evidence-backed role map for all current direct `src/` directories,
  including exceptions and directories without runtime source.
- The current and target Core composition, ownership/shutdown, trust boundaries,
  request-context flow, and the explicit limits of the interface-DI path.
- The source-identity/materialization contract, legacy compatibility, API/ABI
  impact, and migration requirements.
- A phased roadmap with completed, remaining, and blocked items kept honest;
  synchronize relevant Core architecture, RAG API/README, include-level
  architecture references, and the `[Unreleased]` changelog entry.

## Acceptance criteria

- [ ] Production bootstrap produces a functional retrieval/query path against
  the real RocksDB-backed server state; tests verify successful reads, not only
  constructed objects or non-null pointers.
- [ ] A record written through the normal vector-ingestion sink can be retrieved
  and materialized as its authoritative source text without a test-only
  duplicate relational write.
- [ ] A ContentManager chunk vector resolves to the corresponding chunk text
  and preserves source identity/provenance.
- [ ] Real HTTP, AQL, and YAML integration points given the same authorized
  typed request produce matching evidence IDs, sources, and content before
  consumer-specific prompt formatting. Mock-only proxy tests do not satisfy
  this criterion.
- [ ] Missing sources, wrong collection/namespace, access denial, expired
  deadline, cancellation, and budget exhaustion have explicit outcomes; none
  fabricate evidence or silently generate without grounding.
- [ ] Parallel requests with different tenants/permissions cannot affect one
  another's authorization.
- [ ] Existing relational/node paths and documented legacy reads remain
  compatible. Any on-disk migration is explicit and tested.
- [ ] Retrieval tests run without an LLM/GPU; inference tests use existing
  approved test mechanisms and do not claim GPU/flash-attention validation
  unless those paths were actually exercised.
- [ ] Focused tests are added to existing test targets and registrations.
  Record actual commands/results and any blocked gates. Run applicable LLM
  enabled/disabled and modular/monolithic registration checks.
- [ ] Canonical documentation, role map, roadmap status, API impact, trust
  boundaries, and changelog match the code that was actually delivered.

## Explicitly out of scope

- A 73-module refactor or conversion of every specialized retrieval subsystem.
- New universal Provider/Consumer base classes, a generic `DataResult`, a
  service-locator context, a new RAG manager, or plugin ABI redesign.
- Changes to unrelated llama.cpp GPU/flash-attention behavior, automatic
  self-RAG/FLARE loops, or model lifecycle.
- A new YAML execution language, general Core workflow engine, or unsupported
  CDC/transaction/Exactly-once guarantee.
- Removing domain-specific behavior from `process`, `ethics_ai`, or `search`.
