---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Module — Public Header Map

**Module path:** `include/aql/`
**Implementation overview:** [`../../src/aql/ARCHITECTURE.md`](../../src/aql/ARCHITECTURE.md)

## Scope

These headers declare public-facing AQL authoring, validation, translation
assistance, context, and adapter types. They are not the core AQL parser or query
execution implementation; those sources are in `include/query/` and `src/query/`.
Check each header for its exact namespace, ownership, and API contract.

## Header groups

| Area | Headers |
|---|---|
| Authoring and validation | `aql_query_builder.h`, `aql_query_validator.h`, `aql_query_template_library.h`, `aql_query_diff_explainer.h`, `aql_rollback_suggester.h`, `aql_syntax_highlighter.h`, `aql_token_stream.h`, `aql_autocomplete.h` |
| Translation and context | `llm_aql_handler.h`, `llm_query_context.h`, `aql_conversation_context.h`, `aql_fewshot_example_library.h`, `aql_confidence_scorer.h`, `llm_validation_pipeline.h`, `llm_semantic_validator.h` |
| Provider and runtime support | `iasync_llm_backend.h`, `llm_aql_embedding_bridge.h`, `llm_error_codes.h`, `aql_error_types.h`, `llm_extractive_compressor.h`, `i_history_compressor.h`, `llm_metrics_collector.h`, `llm_timeout_manager.h`, `llm_token_estimator.h`, `multimodal_infer_request.h` |
| Adapters and domain helpers | `aql_agent.h`, `aql_ingestion_bridge.h`, `aql_lora_finetuner.h`, `aql_migration_assistant.h`, `aql_model_router.h`, `aql_optimizer_advisor.h`, `aql_schema_provider.h`, `classify_bridge.h`, `docs_assistant_functions.h` |

This list groups current header filenames; it does not assert that every type
is used by every build or deployment.

## Translation and validation boundary

`LLMAQLHandler` exposes natural-language translation methods and parser-service
configuration. The implementation uses `query::AQLParserService` when one is
available and otherwise uses the weaker string-level `AQLQueryValidator`
fallback. `AQLQueryValidator` also offers string/builder overloads with
optional `CollectionMetadata`. Neither interface executes a query or replaces
the host application's authorization checks.

See [`../../src/aql/API.md`](../../src/aql/API.md) for the current method
inventory and the unresolved divergence from the frozen assistance contract.

## Build and compatibility

Build composition is controlled by root CMake options and the selected
configuration; `cmake/ModularBuild.cmake` conditionally groups AQL sources.
`tests/aql/CMakeLists.txt` and `benchmarks/aql/CMakeLists.txt` describe the
focused test and benchmark targets. Do not infer a stable ABI or universal
platform/language compatibility guarantee from the presence of a public header;
consult the repository versioning policy and the relevant build configuration.
