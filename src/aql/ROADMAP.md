# AQL Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for LLM-assisted AQL query generation, natural language to AQL translation, and documentation assistance. Core AQL parsing and execution are handled by the query module.

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

## In Progress 🚧
*(no items currently in progress)*

## Planned Features 📋

### Long-term (6-12 months)
- [ ] (reserved)

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

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (42 unit tests in few-shot library + 3 performance benchmarks + 7 integration tests + 13 injection tests + 1 highlighter path integration test in handler)
- [x] Integration tests (handler ↔ highlighter path covered)
- [x] Performance benchmarks (few-shot library: findRelevant/buildPromptSection timing tests added; AQLSyntaxHighlighter, AQLConfidenceScorer, and AQLFewShotExampleLibrary benchmarks implemented in `benchmarks/bench_hybrid_aql_sugar.cpp`, Issue: #1523)
- [x] Security audit (prompt injection prevention via `sanitizePromptInput()` in `translateNLToAQL()`, `translateNLToAQLStreaming()`, and `translateNLToAQLWithExamples()`)  
- [x] Documentation complete (README.md and ROADMAP.md updated)
- [x] API stability guaranteed (Issue: #1524)

## Known Issues & Limitations
- NL-to-AQL accuracy depends on LLM provider quality and prompt engineering
- No offline fallback when no LLM provider is configured
- Prompt injection in `translateNLToAQL()` is mitigated by pattern-based input sanitization; advanced adversarial inputs not covered by the current pattern set may still bypass detection
- Schema-aware generation is supported: attach a metadata snapshot via `AQLQueryBuilder::setSchema()` (`include/aql/aql_query_builder.h`); schema context is injected automatically into LLM prompts and unknown collection names are flagged as validation warnings

## Breaking Changes
- LLM command handler API is stable; new command types will be additive
- Confidence scoring API was introduced as an optional field (non-breaking)