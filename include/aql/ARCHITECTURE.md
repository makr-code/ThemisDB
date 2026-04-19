<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# AQL Module — Public Header Architecture

**Version:** 1.8.0
**Last Updated:** 2026-04-06
**Module Path:** `include/aql/`
**Implementation:** `../../src/aql/`

---

## 1. Overview

The `include/aql/` directory exposes public C++ headers for ThemisDB's AQL (Adaptive Query Language)
engine. AQL combines a structured query language with LLM-assisted natural language query translation,
autocomplete, confidence scoring, schema-aware validation, syntax highlighting, and multi-turn
conversation context. The headers define the contract between AQL tooling and the query execution
pipeline.

---

## 2. Design Principles

- **LLM Abstraction** – `iasync_llm_backend.h` defines the async LLM interface; all LLM-dependent
  components depend only on this abstraction, not on any specific model.
- **Validation Before Execution** – `aql_query_validator.h` and `aql_schema_provider.h` must be
  consulted before query execution; invalid queries are rejected at the header contract boundary.
- **Composable Query Building** – `aql_query_builder.h` and `aql_query_template_library.h` allow
  programmatic query construction without string manipulation.
- **Observability** – `llm_metrics_collector.h` and `llm_timeout_manager.h` are always-available
  interfaces for LLM call monitoring.
- **Multimodal Ready** – `multimodal_infer_request.h` extends the inference interface for image,
  audio, and video inputs.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `iasync_llm_backend.h` | `IAsyncLLMBackend` | Primary async LLM inference interface |
| `llm_aql_handler.h` | `ILLMAQLHandler` | LLM-to-AQL translation handler |
| `aql_agent.h` | `IAQLAgent`, `AQLAgentConfig` | Autonomous AQL query agent |
| `aql_autocomplete.h` | `IAQLAutocomplete`, `AutocompleteResult` | Token-level AQL autocomplete |
| `aql_confidence_scorer.h` | `IAQLConfidenceScorer`, `ConfidenceScore` | Query confidence scoring |
| `aql_conversation_context.h` | `IAQLConversationContext`, `ConversationTurn` | Multi-turn context management |
| `aql_fewshot_example_library.h` | `IAQLFewshotExampleLibrary` | Few-shot example retrieval |
| `aql_lora_finetuner.h` | `IAQLLoRAFinetuner`, `LoRAConfig` | LoRA fine-tuning for AQL model |
| `aql_migration_assistant.h` | `IAQLMigrationAssistant` | Schema migration query generation |
| `aql_optimizer_advisor.h` | `IAQLOptimizerAdvisor`, `OptimizationHint` | Query optimisation hints |
| `aql_query_builder.h` | `AQLQueryBuilder`, `AQLQuery` | Programmatic AQL query construction |
| `aql_query_template_library.h` | `IAQLQueryTemplateLibrary`, `QueryTemplate` | Template-based query generation |
| `aql_query_validator.h` | `IAQLQueryValidator`, `ValidationResult` | Syntactic and semantic validation |
| `aql_schema_provider.h` | `IAQLSchemaProvider`, `AQLSchema` | Schema-aware query context |
| `aql_syntax_highlighter.h` | `IAQLSyntaxHighlighter`, `HighlightToken` | Token-based syntax highlighting |
| `aql_token_stream.h` | `AQLTokenStream`, `AQLToken` | Lexer token stream |
| `classify_bridge.h` | `IClassifyBridge` | Intent classification bridge |
| `docs_assistant_functions.h` | `DocsAssistantFunctions` | Documentation-aware query assistance |
| `llm_error_codes.h` | `LLMErrorCode` enum | LLM-specific error taxonomy |
| `llm_metrics_collector.h` | `ILLMMetricsCollector`, `LLMMetric` | LLM call metrics collection |
| `llm_timeout_manager.h` | `ILLMTimeoutManager`, `TimeoutPolicy` | LLM call timeout policy |
| `llm_token_estimator.h` | `ILLMTokenEstimator` | Input token budget estimation |
| `multimodal_infer_request.h` | `MultimodalInferRequest` | Multimodal (image/audio/video) inference |

> **Implementation details:** `../../src/aql/`
