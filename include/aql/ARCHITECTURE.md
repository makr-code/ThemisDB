> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/aql/ARCHITECTURE.md -->

# AQL (AI Query Language) Module — Public Header Architecture

**Module Path:** `include/aql/`  
**Implementation:** `../../src/aql/`  
**Canonical architecture doc:** [`../../src/aql/ARCHITECTURE.md`](../../src/aql/ARCHITECTURE.md)

---

## 1. Overview

`include/aql/` defines the **public AQL query building, validation, autocomplete, LoRA fine-tuning, LLM routing, and multi-modal inference bridges API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/aql/ARCHITECTURE.md`](../../src/aql/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Query Authoring

| Header | Public Type | Purpose |
|--------|------------|---------|
| `aql_query_builder.h` | `AQLQueryBuilder` | Fluent AQL query construction |
| `aql_query_validator.h` | `AQLQueryValidator` | Syntactic and semantic validation |
| `aql_query_template_library.h` | `AQLQueryTemplateLibrary` | Pre-built query template registry |
| `aql_token_stream.h` | `AQLTokenStream` | Token streaming for incremental parsing |
| `aql_syntax_highlighter.h` | `AQLSyntaxHighlighter` | Token-level syntax colouring |
| `aql_query_diff_explainer.h` | `AQLQueryDiffExplainer` | Human-readable diff between two AQL queries |
### 2.2 AI Assistance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `aql_autocomplete.h` | `AQLAutocomplete` | Context-aware query autocompletion |
| `aql_agent.h` | `AQLAgent` | Agentic AQL query planner |
| `aql_confidence_scorer.h` | `AQLConfidenceScorer` | Confidence scoring for generated queries |
| `aql_fewshot_example_library.h` | `AQLFewshotExampleLibrary` | Few-shot examples for query generation |
| `aql_conversation_context.h` | `AQLConversationContext` | Multi-turn conversation context manager |
| `aql_optimizer_advisor.h` | `AQLOptimizerAdvisor` | LLM-assisted query optimisation hints |
### 2.3 LLM Routing and Backends

| Header | Public Type | Purpose |
|--------|------------|---------|
| `aql_model_router.h` | `AQLModelRouter` | Multi-LLM routing and load balancing |
| `iasync_llm_backend.h` | `IAsyncLLMBackend` | Async LLM backend interface |
| `llm_aql_handler.h` | `LLMAQLHandler` | LLM request handler for AQL context |
| `llm_aql_embedding_bridge.h` | `LLMAQLEmbeddingBridge` | Embedding computation bridge |
| `llm_error_codes.h` | `LLMError` | LLM error code taxonomy |
| `llm_metrics_collector.h` | `LLMMetricsCollector` | Per-request LLM telemetry |
| `llm_timeout_manager.h` | `LLMTimeoutManager` | Deadline propagation for LLM calls |
| `llm_token_estimator.h` | `LLMTokenEstimator` | Token count estimation utilities |
### 2.4 Bridges and Adapters

| Header | Public Type | Purpose |
|--------|------------|---------|
| `aql_ingestion_bridge.h` | `AQLIngestionBridge` | AQL→ingestion pipeline bridge |
| `aql_lora_finetuner.h` | `AQLLoRAFinetuner` | AQL-driven LoRA fine-tuning trigger |
| `aql_migration_assistant.h` | `AQLMigrationAssistant` | Query migration and upgrade assistant |
| `aql_rollback_suggester.h` | `AQLRollbackSuggester` | Rollback suggestion for failed queries |
| `aql_schema_provider.h` | `AQLSchemaProvider` | Schema introspection for query context |
| `classify_bridge.h` | `ClassifyBridge` | Text-classification routing bridge |
| `docs_assistant_functions.h` | `DocsAssistantFunctions` | Documentation-aware helper functions |
| `multimodal_infer_request.h` | `MultimodalInferRequest` | Multi-modal inference request structure |

---

## 3. Namespace Layout

All public types reside in the `themis::aql` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/aql/` expose the **stable public API**; internal types live in `src/aql/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM/Graph**.
