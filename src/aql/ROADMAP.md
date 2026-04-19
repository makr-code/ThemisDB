# AQL Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for LLM-assisted AQL query generation, natural language to AQL translation, and documentation assistance. Core AQL parsing and execution are handled by the query module. Phase 4 adds a generic `AQLTokenStream` streaming interface and a `ReActAgent` multi-step reasoning framework with tool calling.

## Completed ✅
- [x] LlmAqlHandler for INFER, RAG, EMBED, MODEL, and LORA command processing
- [x] Natural language to AQL translation via LLM integration
- [x] AQL documentation assistant for function lookup and explanation
- [x] Query explanation and profiling assistance
- [x] LLM command handler infrastructure (request routing, response parsing)
- [x] Support for multi-paradigm AQL (documents, graphs, vectors, geospatial, timeseries)
- [x] Integration with OpenAI, Anthropic, Azure OpenAI, and llama.cpp providers
- [x] AQL syntax highlighting and error annotation in LLM responses (`AQLSyntaxHighlighter`) (Issue: #1353)
- [x] Confidence scoring for generated AQL queries (`LLMAQLHandler::translateNLToAQLWithConfidence`, `AQLConfidenceScorer`) (Issue: #1357)
- [x] Multi-turn conversation context for iterative query refinement (`LLMAQLHandler::executeChat`) (Issue: #1358)
- [x] AQL auto-complete API for editor integrations (LSP-compatible) (Issue: #1359)
- [x] AQL query migration assistant (ArangoDB AQL → ThemisDB AQL) (Issue: #1360)
- [x] Schema-aware query generation using live collection metadata (Issue: #1361)
- [x] AQL function documentation auto-generation from C++ headers (Issue: #1362)
- [x] Fine-tuned local model (LoRA adapter) for ThemisDB-specific AQL (Issue: #1363)
- [x] Integration with query optimizer for cost-aware suggestions (Issue: #1364)
- [x] Few-shot example library for improved NL-to-AQL accuracy (Target: Q3 2026) (Issue: #1521)
- [x] Streaming natural language responses for long AQL explanations (Target: Q2 2026) (Issue: #1950) — `POST /api/v1/llm/aql/explain/stream` SSE endpoint exposing `LLMAQLHandler::streamExplainAQLAsSSE()`
- [x] AQL query validation and linting before LLM submission (`src/aql/aql_query_validator.cpp`) (Issue: #1525)
- [x] Query template library for common AQL patterns (`src/aql/aql_query_template_library.cpp`)
- [x] Schema-aware programmatic AQL query builder (`src/aql/aql_query_builder.cpp`)
- [x] LLM inference metrics collection (`src/aql/llm_metrics_collector.cpp`)
- [x] Generic `AQLTokenStream` iterator API for all LLM inference calls (`include/aql/aql_token_stream.h`) (Phase 4)
- [x] ReActAgent multi-step reasoning framework with tool calling (`src/aql/aql_agent.cpp`) (Phase 4)
- [x] Runtime-configurable confidence scoring weights (`AQLConfidenceScorer::Config`, `calibrate()`, word-boundary keyword matching) (Target: v1.6.0) (Issue: #144)

## In Progress 🚧
*(no items currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Generic `AQLTokenStream` iterator API for all LLM inference calls (Target: v1.7.0)
  - Files: `include/aql/aql_token_stream.h` (header-only), `tests/test_aql_token_stream.cpp`
  - Behavior: thread-safe queue with `push(token)` / `close()` from producer thread; `nextToken()` blocking pop and range-based for-loop from consumer thread; `cancel()` with cooperative cancellation flag (`isCancelled()`)
  - Errors: push after cancel is silently discarded; `nextToken()` after cancel/close returns `std::nullopt`; destructor calls `close()` to unblock any waiting consumer
  - Tests: unit (single-threaded push+drain, ordering, empty stream, cancel, idempotent close, concurrent producer/consumer 100 tokens, range-based for loop, cancel-mid-stream)
  - Perf: `push()` and `nextToken()` overhead ≤ 500 ns excluding model generation time; no busy-wait (condition variable)
  - Compat: header-only, C++17; no external dependencies beyond STL

- [x] AQL Agent Framework – ReAct (Reasoning + Acting) multi-step agent with tool calling (Target: v1.7.0)
  - Files: `include/aql/aql_agent.h`, `src/aql/aql_agent.cpp`, `tests/test_aql_agent.cpp`
  - Types: `AgentTool` (name, description, JSON Schema, executor), `AgentConfig` (model, max_iterations, temperature), `ReasoningStep` (thought, tool_name, tool_input, tool_output, observation), `AgentResult` (final_answer, reasoning_trace, iterations_used, succeeded), `IAgent` (abstract), `ReActAgent` (Pimpl concrete implementation)
  - Behavior: iterates Thought→Action→Observation cycles up to `max_iterations`; stops when LLM emits "Final Answer:" prefix; tool executor errors are captured as JSON and fed back as observations (never propagate to caller)
  - Errors: duplicate tool registration → `std::invalid_argument`; unknown tool removal → `std::invalid_argument`; LLM failure → `LLMException(INFERENCE_FAILED)`; max iterations reached → `AgentResult.succeeded = false` with descriptive message
  - Tests: unit (register/remove/duplicate tools, config, execute with no model, execute with max iterations, execute with tool invocation, move semantics)
  - Perf: tool dispatch overhead ≤ 1 ms per step excluding tool execution; no heap allocation in reasoning-step parsing hot path

### Long-term (6-12 months)
- [x] Multi-modal AQL agent inputs (image/audio/video via `MultiModalInferRequest`) (Target: v1.8.0)
- [x] `IAsyncLLMBackend` – non-blocking `std::future<Result<T>>` inference interface (Target: v1.8.0)

## Implementation Phases
### Phase 1: LLM-Assisted AQL Foundation (Status: Completed ✅)
- [x] LlmAqlHandler for INFER, RAG, EMBED, MODEL, and LORA command processing (`src/aql/llm_aql_handler.cpp`)
- [x] Natural language to AQL translation via LLM integration
- [x] AQL documentation assistant for function lookup and explanation
- [x] Query explanation and profiling assistance
- [x] LLM command handler infrastructure (request routing, response parsing)
- [x] Multi-paradigm AQL support: documents, graphs, vectors, geospatial, timeseries
- [x] Provider integration: OpenAI, Anthropic, Azure OpenAI, llama.cpp

### Phase 2: Validation & Developer Experience (Status: Completed ✅)
- [x] AQL query validation and linting (`src/aql/aql_query_validator.cpp`)
- [x] Schema-aware programmatic AQL query builder (`src/aql/aql_query_builder.cpp`)
- [x] Query template library for common AQL patterns (`src/aql/aql_query_template_library.cpp`)
- [x] Token-level autocompletion / LSP-compatible suggestions (`src/aql/aql_autocomplete.cpp`)
- [x] Multi-turn conversation context (`src/aql/aql_conversation_context.cpp`)

### Phase 3: Advanced Tooling & Intelligence (Status: Mostly Completed ✅)
- [x] Batch NL-to-AQL translation for offline workloads (Issue: #1356)

### Phase 4: Streaming & Agent Framework (Status: Completed ✅)
- [x] Generic `TokenStream` iterator API for all LLM inference calls (Target: v1.7.0)
  - `include/aql/aql_token_stream.h` – header-only, thread-safe push/pop/cancel/range-for
  - `tests/test_aql_token_stream.cpp` – 14 unit tests covering single-threaded, concurrent, cancel, and range-for scenarios
- [x] AQL Agent Framework – ReAct multi-step agent with tool calling (Target: v1.7.0)
  - `include/aql/aql_agent.h` – `AgentTool`, `AgentConfig`, `ReasoningStep`, `AgentResult`, `IAgent`, `ReActAgent`
  - `src/aql/aql_agent.cpp` – Pimpl ReActAgent implementation
  - `tests/test_aql_agent.cpp` – 17 unit tests covering registration, config, execute, move semantics
- [x] Multi-modal agent inputs (image/audio/video) (Target: v1.8.0)
  - `include/aql/multimodal_infer_request.h` – `ModalityType`, `MultiModalInput` (MIME-validated), `MultiModalInferRequest`
  - `tests/test_aql_multimodal.cpp` – 28 unit tests covering construction, MIME validation, helpers
- [x] `IAsyncLLMBackend` async interface (Target: v1.8.0)
  - `include/aql/iasync_llm_backend.h` – pure abstract `IAsyncLLMBackend` + `ThreadPoolAsyncLLMBackend` adapter
  - `tests/test_aql_async_backend.cpp` – 11 unit tests covering construction, async inference, error propagation

### Phase 5: Hardening & Developer Ergonomics (Status: Completed ✅)
- [x] **Feature 8** – Semantic Few-Shot Selection (`IEmbeddingProvider` interface, `setEmbeddingProvider()` / `rebuildEmbeddingIndex()` on `AQLFewShotExampleLibrary`)
  - `include/aql/aql_fewshot_example_library.h` – `IEmbeddingProvider` abstract interface, new methods + semantic private helpers
  - `src/aql/aql_fewshot_example_library.cpp` – cosine-similarity ranking with Jaccard fallback, lazy embedding cache
- [x] **Feature 10** – Deduplicated Prompt-Building helpers in `LLMAQLHandler`
  - `include/aql/llm_aql_handler.h` – private static helpers: `buildNLToAQLSystemPrompt()`, `stripMarkdownFences()`, `logAnnotations()`
  - `src/aql/llm_aql_handler.cpp` – refactored all three translate methods to use shared helpers
- [x] **Feature 12** – Schema-aware `AQLQueryValidator::validate(query, schema)` overload
  - `include/aql/aql_query_validator.h` – includes `aql_schema_provider.h`; new `validate(string, vector<CollectionMetadata>)` overload + private helpers
  - `src/aql/aql_query_validator.cpp` – `checkUnknownCollections()` and `checkUnknownFields()` implementations
- [x] **Feature 13** – Runtime-overridable `ValidationLimitsConfig` + `setValidationLimits()` / `setTimeoutConfig()`
  - `include/aql/llm_error_codes.h` – new `ValidationLimitsConfig` struct (runtime version of `ValidationLimits` namespace)
  - `include/aql/llm_aql_handler.h` / `src/aql/llm_aql_handler.cpp` – `setValidationLimits()`, `getValidationLimits()`, `setTimeoutConfig()`
- [x] **Feature 14** – Named LoRA hyperparameter constants + `Config::fromOptions()` factory
  - `include/aql/aql_lora_finetuner.h` – `kDefaultRank`, `kDefaultAlpha`, etc. `static constexpr` members; `fromOptions()` declaration
  - `src/aql/aql_lora_finetuner.cpp` – `fromOptions()` with range validation for all hyperparameters
- [x] **Feature 15** – `DocsAssistantFunctions` degraded-mode reporting
  - `include/aql/docs_assistant_functions.h` – `DegradedReason` enum, `isFullyReady()`, `degradedReason()` declarations
  - `src/aql/docs_assistant_functions.cpp` – `Impl` tracks `degraded_reason_` + `degraded_message_`; emits `spdlog::warn` before degrading

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (42 unit tests in few-shot library + 3 performance benchmarks + 7 integration tests + 13 injection tests + 1 highlighter path integration test in handler + 14 token-stream tests + 17 agent tests + 28 multimodal tests + 11 async-backend tests)
- [x] Integration tests (handler ↔ highlighter path covered)
- [x] Performance benchmarks (few-shot library: findRelevant/buildPromptSection timing tests added; AQLSyntaxHighlighter, AQLConfidenceScorer, and AQLFewShotExampleLibrary benchmarks implemented in `benchmarks/bench_hybrid_aql_sugar.cpp`, Issue: #1523)
- [x] Security audit (prompt injection prevention via `sanitizePromptInput()` in `translateNLToAQL()`, `translateNLToAQLStreaming()`, and `translateNLToAQLWithExamples()`; AgentTool executor exceptions are caught and returned as JSON error objects)
- [x] Documentation complete (README.md and ROADMAP.md updated; FUTURE_ENHANCEMENTS.md Implementation Notes added)
- [x] API stability guaranteed (Issue: #1524)

## Known Issues & Limitations
- NL-to-AQL accuracy depends on LLM provider quality and prompt engineering
- No offline fallback when no LLM provider is configured
- Prompt injection in `translateNLToAQL()` is mitigated by pattern-based input sanitization; advanced adversarial inputs not covered by the current pattern set may still bypass detection
- Schema-aware generation is supported: attach a metadata snapshot via `AQLQueryBuilder::setSchema()` (`include/aql/aql_query_builder.h`); schema context is injected automatically into LLM prompts and unknown collection names are flagged as validation warnings

## Breaking Changes
- LLM command handler API is stable; new command types will be additive
- Confidence scoring API was introduced as an optional field (non-breaking)