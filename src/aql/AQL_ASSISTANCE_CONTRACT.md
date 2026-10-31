---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance API Contract and Implementation Review

## Status and source precedence

This document reconciles the AQL assistance contract with the implementation
reviewed on 2026-10-31. The former "Frozen Contract Semantics" text described
intended behavior, but several of its promises were not implemented. Those
statements are no longer presented as runtime guarantees. The current public
signatures are defined by
[`../../include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h);
implementation observations below are grounded in
[`llm_aql_handler.cpp`](llm_aql_handler.cpp) and focused tests.

This is a documentation reconciliation, not a change to runtime behavior or an
approval to change the frozen API/versioning policy. Any future code change to
make the runtime meet a stronger contract requires the applicable API review,
tests, and release-note/versioning decision.

## Public translation surface

`LLMAQLHandler` publicly declares `translateNLToAQL()`,
`translateNLToAQLStreaming()`, `translateNLToAQLWithConfidence()`,
`translateNLToAQLWithExamples()`, and batch translation methods. The function
`validateAQLWithParser()` is a file-local helper in `llm_aql_handler.cpp`; it
is not a public `LLMAQLHandler` method.

## Observed translation and validation behavior

### Input and empty requests

- `translateNLToAQL()` sanitizes the natural-language input and schema context
  for configured length limits, null bytes, and listed prompt-injection
  patterns. It does not special-case empty or whitespace-only natural-language
  input before invoking the generation path. Therefore, empty input has no
  guaranteed empty-string or no-exception result; the provider response and
  validation mode determine the outcome.
- The internal generated-AQL validation helper rejects an empty generated
  candidate. Under the default `WARN_ONLY` mode, a validation failure is logged
  and the candidate is still returned. `REJECT_ON_ERROR` throws
  `LLMException(INVALID_RESPONSE)`; `RETRY_ON_ERROR` retries with sanitized
  validation feedback and rejects after the configured attempts are exhausted.
- `RETRY_ON_ERROR` uses `validation_config.max_retries + 1` total attempts.
  The handler's default validation mode is `WARN_ONLY`; the default
  `validation_config.max_retries` is 1. Do not interpret that configuration as
  a guarantee that every translation receives an additional attempt.

### Parser service

- `Config::parser_service` defaults to null, but handler construction creates a
  default parser service when one is not injected.
- Calling `setParserService(nullptr)` explicitly disables parser-backed
  validation. The helper then uses `AQLQueryValidator` string-level checks;
  that fallback is not equivalent to parser/AST validation.
- The helper returns its validation result internally. It is not a public
  validation API and does not execute a query against a database.
- Parsing or string-level validation does not establish authorization or
  semantic correctness against live data. The translation path separately
  applies its configured collection-access check and schema-scope check.

### Errors, streaming, and determinism

- Over-limit input throws `LLMException(PROMPT_TOO_LONG)`. Detected null bytes
  and prompt-injection patterns throw `LLMException(PROMPT_INJECTION)`.
- Validation rejection uses `LLMException(INVALID_RESPONSE)`. Provider and
  other failures follow their actual call paths; this document does not promise
  a single retry/error mapping for every provider failure.
- Streaming translation applies the same input sanitization and validation
  mode. Callback chunks are partial and must not be executed; use only the
  final returned value and still enforce the host application's execution
  authorization.
- `LLMAQLHandler::Config` does not define `temperature` or `seed` fields.
  Provider options are supplied through the relevant request APIs. The
  implementation makes no cross-provider determinism guarantee.

## Declared error codes versus implemented behavior

[`../../include/aql/llm_error_codes.h`](../../include/aql/llm_error_codes.h)
declares `PROVIDER_UNSUPPORTED` (6001) and `CAPABILITY_UNSUPPORTED` (6002).
The module source/header search for this review found no corresponding
throw-sites. Their former provider/capability trigger conditions,
non-retryability promises, and remediation requirements are therefore
**unverified intended contract**, not established behavior. Add explicit
implementation and focused tests before documenting those conditions as
runtime guarantees.

Other numeric values in the enum are defined by the header. Specific retry
count, timeout, and error mapping must be verified against the operation's
implementation rather than inferred from the enum or the former frozen table.

## Concurrency and ownership

The former frozen text asserted that separate handler instances are safe and
shared-instance concurrent calls cause undefined behavior. That assertion is
not established by the public declarations or the tests reviewed here. Do not
rely on a thread-safety guarantee without an API-level contract and dedicated
concurrency evidence. Follow ownership/lifetime annotations in the individual
headers for borrowed dependencies and adapters.

## Focused evidence

Relevant current test sources include:

- [`../../tests/aql/test_aql_api_stability.cpp`](../../tests/aql/test_aql_api_stability.cpp)
- [`../../tests/aql/test_aql_llm_validation_pipeline.cpp`](../../tests/aql/test_aql_llm_validation_pipeline.cpp)
- [`../../tests/aql/test_aql_llm_validation_pipeline_phase04.cpp`](../../tests/aql/test_aql_llm_validation_pipeline_phase04.cpp)
- [`../../tests/aql/test_llm_aql_handler_phase03.cpp`](../../tests/aql/test_llm_aql_handler_phase03.cpp)
- [`../../tests/aql/test_aql_translation_recovery.cpp`](../../tests/aql/test_aql_translation_recovery.cpp)

The test registration and target naming are defined in
[`../../tests/aql/CMakeLists.txt`](../../tests/aql/CMakeLists.txt). These source
references are not a claim that the tests were run or that every behavior above
has a dedicated regression test.

## Reconciliation follow-up

- Do not silently change the runtime to match the former frozen prose. First
  decide whether the implementation or intended API contract is canonical.
- If changing runtime semantics, add regression tests for empty input,
  validation modes, null parser service, streaming, and unsupported capability
  handling; then update the API/versioning and changelog records as required.
- If retaining the present runtime, preserve this source-grounded behavior
  description and add tests for any contract guarantee that maintainers choose
  to make normative.
