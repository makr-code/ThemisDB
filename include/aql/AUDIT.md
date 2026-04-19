<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — AQL Module Public Headers

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass

---

## Summary

| Metric | Result |
|--------|--------|
| Public Header Files | 27 `.h` |
| Open Stubs | 0 |
| LLM Abstraction Layer | ✅ (`iasync_llm_backend.h`) |
| Observability Headers | ✅ (`llm_metrics_collector.h`, `llm_timeout_manager.h`) |
| Error Taxonomy | ✅ (`llm_error_codes.h`) |

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `iasync_llm_backend.h` | `IAsyncLLMBackend` | Primary async LLM interface |
| `llm_aql_handler.h` | `ILLMAQLHandler` | LLM → AQL translation |
| `aql_agent.h` | `IAQLAgent`, `AQLAgentConfig` | Autonomous agent |
| `aql_autocomplete.h` | `IAQLAutocomplete`, `AutocompleteResult` | Token autocomplete |
| `aql_confidence_scorer.h` | `IAQLConfidenceScorer`, `ConfidenceScore` | Confidence scoring |
| `aql_conversation_context.h` | `IAQLConversationContext`, `ConversationTurn` | Multi-turn context |
| `aql_fewshot_example_library.h` | `IAQLFewshotExampleLibrary` | Few-shot examples |
| `aql_lora_finetuner.h` | `IAQLLoRAFinetuner`, `LoRAConfig` | LoRA fine-tuning |
| `aql_migration_assistant.h` | `IAQLMigrationAssistant` | Migration query gen |
| `aql_optimizer_advisor.h` | `IAQLOptimizerAdvisor`, `OptimizationHint` | Optimisation hints |
| `aql_query_builder.h` | `AQLQueryBuilder`, `AQLQuery` | Programmatic construction |
| `aql_query_template_library.h` | `IAQLQueryTemplateLibrary`, `QueryTemplate` | Templates |
| `aql_query_validator.h` | `IAQLQueryValidator`, `ValidationResult` | Validation |
| `aql_schema_provider.h` | `IAQLSchemaProvider`, `AQLSchema` | Schema context |
| `aql_syntax_highlighter.h` | `IAQLSyntaxHighlighter`, `HighlightToken` | Syntax highlighting |
| `aql_token_stream.h` | `AQLTokenStream`, `AQLToken` | Lexer token stream |
| `classify_bridge.h` | `IClassifyBridge` | Intent classification |
| `docs_assistant_functions.h` | `DocsAssistantFunctions` | Docs-aware assistance |
| `llm_error_codes.h` | `LLMErrorCode` | Error taxonomy |
| `llm_metrics_collector.h` | `ILLMMetricsCollector`, `LLMMetric` | Metrics |
| `llm_timeout_manager.h` | `ILLMTimeoutManager`, `TimeoutPolicy` | Timeout policies |
| `llm_token_estimator.h` | `ILLMTokenEstimator` | Token budget estimation |
| `multimodal_infer_request.h` | `MultimodalInferRequest` | Multimodal inference |
| `aql_ingestion_bridge.h` | `AQLIngestionBridge` | ✅ Reviewed |
| `aql_model_router.h` | `AQLModelRouter` | ✅ Reviewed |
| `aql_query_diff_explainer.h` | `AQLQueryDiffExplainer` | ✅ Reviewed |
| `aql_rollback_suggester.h` | `AQLRollbackSuggester` | ✅ Reviewed |

---

## Findings

### Resolved
- `iasync_llm_backend.h` correctly abstracts all LLM dependencies.
- Error codes unified in `llm_error_codes.h`.

### Open
- `aql_lora_finetuner.h`: LoRA fine-tuning header requires `THEMIS_ENABLE_LORA` compile
  guard — verify flag is documented in CMakeLists.txt.
