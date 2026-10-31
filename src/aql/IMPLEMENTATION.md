---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance Implementation

## Scope and ownership

`src/aql/` implements assistance around AQL authoring: translation and
validation orchestration, context and example management, confidence and
diagnostic helpers, and optional integration bridges. Public declarations
live in [`include/aql/`](../../include/aql/); query parsing and execution
services are owned by `include/query/` and `src/query/`. The AQL assistance
module may call query services but is not a substitute for query execution or
authorization.

## Main implementation paths

| Responsibility | Implementation | Contract/evidence |
|---|---|---|
| Translation and command orchestration | [`llm_aql_handler.cpp`](llm_aql_handler.cpp) | [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md) |
| Parser-backed generation/validation | [`llm_aql_handler.cpp`](llm_aql_handler.cpp), [`llm_validation_pipeline.cpp`](llm_validation_pipeline.cpp) | [`../query/AQL_LLM_INTEGRATION_CONTRACT.md`](../query/AQL_LLM_INTEGRATION_CONTRACT.md) |
| Query and schema assistance checks | [`aql_query_validator.cpp`](aql_query_validator.cpp), [`aql_schema_provider.cpp`](aql_schema_provider.cpp) | [`include/aql/aql_query_validator.h`](../../include/aql/aql_query_validator.h) |
| Few-shot retrieval and optional embeddings | [`aql_fewshot_example_library.cpp`](aql_fewshot_example_library.cpp), [`llm_aql_embedding_bridge.cpp`](llm_aql_embedding_bridge.cpp) | [`EMBEDDING.md`](EMBEDDING.md) |
| Bounded conversational state | [`aql_conversation_context.cpp`](aql_conversation_context.cpp) | [`include/aql/aql_conversation_context.h`](../../include/aql/aql_conversation_context.h) |
| Error taxonomy and diagnostics | [`include/aql/aql_error_types.h`](../../include/aql/aql_error_types.h) | [`ERROR_RECOVERY_MATRIX.md`](ERROR_RECOVERY_MATRIX.md) |

## Invariants and edge cases

- Treat every natural-language request and provider response as untrusted.
- Parse/validate generated AQL before handing it to a downstream caller.
  Parser acceptance does not establish semantic correctness or permission.
- The handler's parser-validation function is an internal helper. It uses
  `AQLParserService` when configured and falls back to string-level
  `AQLQueryValidator` checks when not; the fallback has weaker guarantees.
  The current empty-input and fallback behavior differ from parts of the
  frozen [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md), as
  detailed in [`API.md`](API.md).
- The frozen contract documents empty-input and parser-validation semantics
  that diverge from current implementation behavior. **TODO:** Reconcile the
  contract and implementation in a reviewed change; do not describe the frozen
  text as an implementation guarantee meanwhile. Other frozen error semantics
  also require per-path verification before being treated as runtime behavior.
- Provider/capability errors that are non-retryable must not be retried with
  unchanged configuration. Retry policies and limits are configurable; avoid
  unbounded loops.
- Conversation history and generated output must remain within their
  configured bounds. Context eviction is expected when a configured limit is
  reached.
- The frozen assistance contract states that concurrent translation calls
  require separate `LLMAQLHandler` instances; shared-instance concurrency is
  unsupported.

## Validation and tests

Focused tests are located under [`tests/aql/`](../../tests/aql/). Relevant
regressions include
[`test_aql_translation_recovery.cpp`](../../tests/aql/test_aql_translation_recovery.cpp),
[`test_aql_validation_error_handling.cpp`](../../tests/aql/test_aql_validation_error_handling.cpp),
[`test_aql_conversation_context.cpp`](../../tests/aql/test_aql_conversation_context.cpp),
and [`test_aql_bridge_degradation.cpp`](../../tests/aql/test_aql_bridge_degradation.cpp).
`tests/aql/CMakeLists.txt` globs `test_aql_*.cpp` into focused module targets;
the glob has no `CONFIGURE_DEPENDS`, so reconfigure after adding/removing a
matching file. Preserve the existing registration and linking conventions.

Before changing a contract, update its frozen specification, matching tests,
and [`CHANGELOG.md`](CHANGELOG.md). See [`API.md`](API.md) for the public
surface and [`MODULE_GUIDE.md`](MODULE_GUIDE.md) for a contributor workflow.
