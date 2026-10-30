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

# AQL Assistance API

## Audience and source of truth

This page is an integration map for developers calling the AQL assistance
interfaces. The declarations in [`include/aql/`](../../include/aql/) and the
frozen semantics in [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md)
are authoritative; this summary does not expand or override their contracts.

## Main interfaces

| Interface | Use | Important boundary |
|---|---|---|
| `aql::LLMAQLHandler` | Inference commands, NL-to-AQL translation, streaming, batch requests, confidence, formatting, and validation configuration | Provider-dependent; a generated query is not executed or authorized by translation alone |
| `aql::AQLQueryValidator` | Structural checks on a query string or query builder, optionally against supplied collection metadata | Reports issues; does not submit a query to the database |
| `aql::AQLConversationContext` | Start/refine a bounded assistance conversation | Holds a non-owning handler relationship; configure bounds and preserve the handler lifetime |
| `aql::AQLFewShotExampleLibrary` | Register, filter, and rank query examples; optionally inject `IEmbeddingProvider` | The provider pointer is non-owning; unavailable embeddings fall back to lexical ranking |
| `aql::LLMAQLEmbeddingBridge` | Adapt `LLMAQLHandler::executeEmbed()` to `IEmbeddingProvider` | Holds a non-owning reference to the handler |
| `aql::ReActAgent` | Compose a handler with explicitly registered tools | Tool executors are application-provided callbacks; enforce authorization in the tool/execution boundary |

Public headers include
[`llm_aql_handler.h`](../../include/aql/llm_aql_handler.h),
[`aql_query_validator.h`](../../include/aql/aql_query_validator.h),
[`aql_conversation_context.h`](../../include/aql/aql_conversation_context.h),
[`aql_fewshot_example_library.h`](../../include/aql/aql_fewshot_example_library.h),
and [`aql_agent.h`](../../include/aql/aql_agent.h).
For the complete public header grouping, see
[`include/aql/ARCHITECTURE.md`](../../include/aql/ARCHITECTURE.md).

## Translation and validation semantics

The public handler exposes `translateNLToAQL()`,
`translateNLToAQLWithConfidence()`, `translateNLToAQLStreaming()`, and parser
service configuration through `setParserService()`. The
`validateAQLWithParser()` function is an internal helper in
`llm_aql_handler.cpp`, not a public `LLMAQLHandler` method.

- The current `translateNLToAQL()` implementation does not special-case empty
  natural-language input; it proceeds through sanitization and the generation
  path.
- Prompt-size and detected-injection failures are explicit errors.
- Provider failures and invalid generated responses propagate according to
  the configured retry/validation policy.
- Streaming callbacks may receive partial explanation/output; callers must
  not execute partial streamed content. Only the final return value follows
  the translation contract.
- With a parser service configured, the internal helper checks the generated
  candidate through `query::AQLParserService`. Without one, it falls back to
  string-level `AQLQueryValidator` checks; this fallback is not equivalent to
  AST parsing. Empty generated AQL is rejected by the helper.
- Parsing is not live execution and does not by itself establish
  authorization. Translation also applies collection access and scope checks
  before returning generated AQL.
- Confidence is a review signal in `[0.0, 1.0]`; an unscored value is not a
  successful safety check.

## Contract drift requiring reconciliation

[`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md) currently describes
empty-input behavior and `validateAQLWithParser()` as a public method, and says
that validation without a parser returns `false`. Those statements do not
match the current implementation described above. This page does not change
the frozen contract; callers and maintainers should not treat the conflicting
statements as interchangeable. Reconcile the contract and implementation in a
dedicated reviewed change before relying on either behavior as a stable API.

For error codes, retry conditions, thread-safety, and compatibility rules,
consult the frozen contract while accounting for the drift noted above. Public
error categories and recovery strategies are defined in
[`include/aql/aql_error_types.h`](../../include/aql/aql_error_types.h) and
[`include/aql/llm_error_codes.h`](../../include/aql/llm_error_codes.h).

## Validation expectations for callers

1. Supply only the schema context needed for the request; do not treat it as
   an access-control grant.
2. Handle explicit errors and empty results rather than assuming every call
   returns executable AQL.
3. Apply the application's authorization and query-safety policy at the
   execution boundary.
4. Add contract tests for changed behavior; the API stability tests are in
   [`test_aql_api_stability.cpp`](../../tests/aql/test_aql_api_stability.cpp).

## Related documentation

- [`IMPLEMENTATION.md`](IMPLEMENTATION.md)
- [`MODULE_GUIDE.md`](MODULE_GUIDE.md)
- [`src/query/AQL_LLM_INTEGRATION_CONTRACT.md`](../query/AQL_LLM_INTEGRATION_CONTRACT.md)
- [`docs/aql/README.md`](../../docs/aql/README.md)
