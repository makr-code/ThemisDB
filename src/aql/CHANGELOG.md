> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — AQL Module

All notable changes to the AQL (ThemisDB Query Language) module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `MultiModalInferRequest` type hierarchy: `ModalityType` enum (TEXT/IMAGE/AUDIO/VIDEO), `MultiModalInput` (MIME-validated, variant payload: string/bytes/path), `MultiModalInferRequest` extending `llm::InferenceRequest` with `addInput()`, `validateInputs()`, `hasNonTextInputs()` helpers (`include/aql/multimodal_infer_request.h`)
- `IAsyncLLMBackend` – pure abstract non-blocking inference interface: `inferAsync(InferenceRequest)` → `std::future<Result<std::string>>`, `embedAsync(string)` → `std::future<Result<vector<float>>>`, `supportsMultiModal()` (`include/aql/iasync_llm_backend.h`)
- `ThreadPoolAsyncLLMBackend` – concrete `IAsyncLLMBackend` adapter wrapping any `ILLMPlugin` via `std::async`; plugin exceptions are captured as `Result` errors rather than propagated through the future
- 28 unit tests for multimodal types (`tests/test_aql_multimodal.cpp`)
- 11 unit tests for async backend interface (`tests/test_aql_async_backend.cpp`)

## [1.7.0] — 2026-03-09
### Added
- Generic `AQLTokenStream` iterator API for all LLM inference calls (`include/aql/aql_token_stream.h`): thread-safe queue, `push(token)`/`close()` from producer; `nextToken()` blocking pop and range-based for-loop from consumer; `cancel()` with cooperative cancellation flag; push-after-cancel silently discarded; `nextToken()` after cancel/close returns `std::nullopt`
- `ReActAgent` multi-step reasoning framework (Reasoning + Acting): `AgentTool` registration, `AgentConfig`, `ReasoningStep` (Thought→Action→Observation cycles), `AgentResult` with reasoning trace; stops on "Final Answer:" prefix; tool errors captured as JSON observations (`src/aql/aql_agent.cpp`)
- `POST /api/v1/llm/aql/explain/stream` SSE endpoint for streaming AQL explanation responses (`LLMAQLHandler::streamExplainAQLAsSSE()`) (Issue #1950)
- LLM inference metrics collection (`src/aql/llm_metrics_collector.cpp`)
- Few-shot example library for improved NL-to-AQL accuracy (`src/aql/aql_fewshot_example_library.cpp`) (Issue #1521)

### Fixed
- Duplicate tool registration now raises `std::invalid_argument` in `ReActAgent::registerTool()`
- LLM failure in agent framework returns `AgentResult.succeeded = false` instead of propagating exception

## [1.6.0] — 2026-01-15
### Added
- AQL query validation and linting before LLM submission (`src/aql/aql_query_validator.cpp`) (Issue #1525)
- Schema-aware programmatic AQL query builder (`src/aql/aql_query_builder.cpp`) (Issue #1361)
- Query template library for common AQL patterns (`src/aql/aql_query_template_library.cpp`)
- AQL LoRA fine-tuner for ThemisDB-specific query generation (`src/aql/aql_lora_finetuner.cpp`) (Issue #1363)
- AQL migration assistant for ArangoDB AQL → ThemisDB AQL translation (`src/aql/aql_migration_assistant.cpp`) (Issue #1360)
- Integration with query optimizer for cost-aware query suggestions (Issue #1364)
- `AQLOptimizerAdvisor` for query plan analysis (`src/aql/aql_optimizer_advisor.cpp`)

## [1.5.0] — 2025-11-01
### Added
- AQL syntax highlighting and error annotation in LLM responses via `AQLSyntaxHighlighter` (`src/aql/aql_syntax_highlighter.cpp`) (Issue #1353)
- Confidence scoring for generated AQL queries: `LLMAQLHandler::translateNLToAQLWithConfidence()`, `AQLConfidenceScorer` (Issue #1357)
- Multi-turn conversation context for iterative query refinement: `LLMAQLHandler::executeChat()` (`src/aql/aql_conversation_context.cpp`) (Issue #1358)
- AQL auto-complete API for editor integrations (LSP-compatible) (`src/aql/aql_autocomplete.cpp`) (Issue #1359)
- Schema-aware query generation using live collection metadata (`src/aql/aql_schema_provider.cpp`) (Issue #1361)
- AQL function documentation auto-generation from C++ headers (Issue #1362)
- `DocsAssistant` functions for AQL function lookup and explanation (`src/aql/docs_assistant_functions.cpp`)

## [1.0.0] — 2024-06-01
### Added
- `LlmAqlHandler` for INFER, RAG, EMBED, MODEL, and LORA command processing (`src/aql/llm_aql_handler.cpp`)
- Natural language to AQL translation via LLM integration
- AQL documentation assistant for function lookup and explanation
- Query explanation and profiling assistance
- LLM command handler infrastructure (request routing, response parsing)
- Multi-paradigm AQL support: documents, graphs, vectors, geospatial, timeseries
- Provider integration: OpenAI, Anthropic, Azure OpenAI, llama.cpp
