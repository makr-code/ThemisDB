<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — AQL Module Public Headers

All notable changes to public headers in `include/aql/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `aql_lora_finetuner.h`: `IAQLLoRAFinetuner` and `LoRAConfig` for LoRA fine-tuning of AQL models
- `aql_migration_assistant.h`: `IAQLMigrationAssistant` for schema-migration query generation
- `docs_assistant_functions.h`: `DocsAssistantFunctions` for documentation-aware query assistance
- `multimodal_infer_request.h`: `MultimodalInferRequest` for image/audio/video query inputs
- `llm_token_estimator.h`: `ILLMTokenEstimator` for pre-dispatch token budget estimation

### Changed
- `iasync_llm_backend.h`: Added `stream()` method for token-streaming responses
- `aql_conversation_context.h`: `ConversationTurn` extended with `tool_calls` list

## [1.7.0] — 2026-03-09
### Added
- `aql_optimizer_advisor.h`: `IAQLOptimizerAdvisor` and `OptimizationHint`
- `aql_fewshot_example_library.h`: `IAQLFewshotExampleLibrary` for dynamic example retrieval
- `llm_metrics_collector.h`: `ILLMMetricsCollector` for LLM call observability
- `llm_timeout_manager.h`: `ILLMTimeoutManager` with configurable timeout policies
- `llm_error_codes.h`: Canonical `LLMErrorCode` taxonomy

### Changed
- `aql_query_validator.h`: `ValidationResult` extended with `fix_suggestions` list
- `aql_autocomplete.h`: `AutocompleteResult` now includes confidence score per suggestion

## [1.6.0] — 2026-02-01
### Added
- Initial public header set: `iasync_llm_backend.h`, `llm_aql_handler.h`, `aql_agent.h`
- `aql_autocomplete.h`, `aql_confidence_scorer.h`, `aql_conversation_context.h`
- `aql_query_builder.h`, `aql_query_validator.h`, `aql_schema_provider.h`
- `aql_syntax_highlighter.h`, `aql_token_stream.h`
- `aql_query_template_library.h`, `classify_bridge.h`
