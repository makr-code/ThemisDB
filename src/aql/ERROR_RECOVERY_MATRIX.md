---
Author: "ThemisDB Team"
Created: "2026-07-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Error Handling and Recovery — Source-Checked Summary

## Scope and evidence status

This page records behavior visible in the current AQL assistance implementation
and public configuration. It is not a universal fail-closed policy or an
operational guarantee. Older phase-4 retry counts, backoff schedules, named
Prometheus counters, circuit-breaker thresholds, and test-pass claims from the
previous version were not consistently supported by current source and have
been removed rather than inferred.

## Current behavior surfaces

| Path | Source-checked behavior | Important limit |
|---|---|---|
| Prompt input | NL and schema-context inputs are sanitized using configured validation limits; size/injection errors are explicit in the handler path. | The limits and policy are configuration, not a universal deployment setting. |
| Generated AQL | Empty generated output fails the helper. Parser service is used when available; without it the handler falls back to `AQLQueryValidator` string-level checks. | The fallback is not AST parsing. `TranslationValidationMode` controls whether a validation issue is warned, rejected, or retried. |
| Retry | Translation attempts are derived from `validation_config.max_retries` and the selected mode. Validation feedback is sanitized/delimited before reuse. | Do not assume a fixed retry count or backoff schedule for all failures. |
| Provider resilience | `LLMAQLHandler::Config` exposes separate circuit-breaker configuration for inference, RAG, embedding, and fine-tuning operations. | Configurable surfaces do not establish live provider health or identical behavior in all paths. |
| Few-shot embedding | `LLMAQLEmbeddingBridge` returns an empty vector for empty input or handler exceptions; the example library can then use lexical Jaccard relevance. | Empty embeddings are an unavailable signal, not a “no match” result. |
| Conversation context | `AQLConversationContext` applies configured turn/token bounds and evicts old user/assistant pairs. Episodic compaction is gated by configuration and compressor availability. | These context rules are not a hard cap on every individual request; callers may need stricter bounds. |

## Error taxonomy and diagnostics

`include/aql/aql_error_types.h` provides `AQLErrorContext` fields for an error
type/category/component, message, operation, location, schema context, retry
count, hints, and recoverability. This type is a diagnostic container; its
presence does not prove that every error path populates it.

The handler uses structured `LLMErrorCode` exceptions for relevant failures and
validation log tags such as `[VALIDATION:MalformedAQL]`. Check
[`API.md`](API.md) and the implementation for the exact method/error behavior;
do not infer uniform exception categories or recovery actions across helpers.

## Focused regression source files

Current source files include:

- `tests/aql/test_aql_validation_error_handling.cpp`
- `tests/aql/test_aql_translation_recovery.cpp`
- `tests/aql/test_aql_bridge_degradation.cpp`
- `tests/aql/test_aql_provider_degradation.cpp`
- `tests/aql/test_aql_circuit_breaker_policy.cpp`
- `tests/aql/test_aql_token_policy.cpp`

`tests/aql/CMakeLists.txt` defines how matching focused tests are registered.
The list above indicates source presence only; no test execution or pass result
is asserted here.

## Unresolved verification

- **TODO:** Reconcile the frozen `AQL_ASSISTANCE_CONTRACT.md` with the current
  empty-input, parser-fallback, and validation-helper behavior described in
  [`API.md`](API.md).
- **TODO:** Audit each provider/bridge path before publishing fixed retry,
  backoff, timeout, circuit-breaker, metric-name, or recovery guarantees.
- **TODO:** Rebuild a component-by-component error matrix from implementation
  and tests if those guarantees are needed operationally.

## Canonical source references

- [`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h)
- [`include/aql/aql_conversation_context.h`](../../include/aql/aql_conversation_context.h)
- [`include/aql/aql_fewshot_example_library.h`](../../include/aql/aql_fewshot_example_library.h)
- [`include/aql/aql_error_types.h`](../../include/aql/aql_error_types.h)
- [`include/aql/llm_error_codes.h`](../../include/aql/llm_error_codes.h)
- [`llm_aql_handler.cpp`](llm_aql_handler.cpp)
- [`llm_aql_embedding_bridge.cpp`](llm_aql_embedding_bridge.cpp)
- [`aql_conversation_context.cpp`](aql_conversation_context.cpp)
- [`aql_fewshot_example_library.cpp`](aql_fewshot_example_library.cpp)
