### Context

This issue implements the roadmap item 'Post-Generation AQL Validation in `translateNLToAQL()`' for the aql domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.6.0.

Primary detail section: 1 · Post-Generation AQL Validation in `translateNLToAQL()`

### Goal

Deliver the scoped changes for Post-Generation AQL Validation in `translateNLToAQL()` in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 1 · Post-Generation AQL Validation in `translateNLToAQL()`
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `llm_aql_handler.cpp:translateNLToAQL()` (lines 1038–1059) validates the LLM-generated query using `AQLSyntaxHighlighter::annotateErrors()` which **only logs warnings** — it never rejects or sanitises the output. `AQLQueryValidator::validate()` (which can produce `ValidationResult` with severity-based `issues`) is never invoked on LLM-generated queries. The same pattern is repeated in `translateNLToAQLStreaming()` (line 1141) and `translateNLToAQLWithExamples()` (line 1389). A structurally invalid query silently reaches the caller and may be executed against the database.

**Implementation Notes:**
- `[ ]` In `llm_aql_handler.cpp:translateNLToAQL()`, after the markdown-fence stripping and `trim()` step, call `AQLQueryValidator::validate(aql_query)` and inspect `ValidationResult::issues`; if any issue has severity `ERROR`, throw `LLMException(LLMErrorCode::INVALID_RESPONSE, ...)` with the first error message instead of silently returning the malformed query
- `[ ]` Apply the same fix to `translateNLToAQLStreaming()` (line 1141) and `translateNLToAQLWithExamples()` (line 1389) — both currently use `annotateErrors()` as the sole post-processing check
- `[ ]` Add a retry path: if validation fails and `retry_policy_` has remaining retries, re-invoke the LLM with an augmented prompt that includes the error annotation as feedback ("Your previous attempt produced this error: …")
- `[ ]` Expose a `TranslationValidationMode` enum (`WARN_ONLY`, `REJECT_ON_ERROR`, `RETRY_ON_ERROR`) on `LLMAQLHandler` so callers can choose enforcement level
- `[ ]` Unit-test: craft an NL query that reliably causes the mock LLM to return broken AQL (`FOR x`) and assert that `translateNLToAQL` throws instead of returning it

**Performance Targets:**
- Validation overhead ≤ 1 ms per generated query (the validator is string-based with no I/O)

---

### Acceptance Criteria

- [ ] In `llm_aql_handler.cpp:translateNLToAQL()`, after the markdown-fence stripping and `trim()` step, call `AQLQueryValidator::validate(aql_query)` and inspect `ValidationResult::issues`; if any issue has severity `ERROR`, throw `LLMException(LLMErrorCode::INVALID_RESPONSE, ...)` with the first error message instead of silently returning the malformed query
- [ ] Apply the same fix to `translateNLToAQLStreaming()` (line 1141) and `translateNLToAQLWithExamples()` (line 1389) — both currently use `annotateErrors()` as the sole post-processing check
- [ ] Add a retry path: if validation fails and `retry_policy_` has remaining retries, re-invoke the LLM with an augmented prompt that includes the error annotation as feedback ("Your previous attempt produced this error: …")
- [ ] Expose a `TranslationValidationMode` enum (`WARN_ONLY`, `REJECT_ON_ERROR`, `RETRY_ON_ERROR`) on `LLMAQLHandler` so callers can choose enforcement level
- [ ] Unit-test: craft an NL query that reliably causes the mock LLM to return broken AQL (`FOR x`) and assert that `translateNLToAQL` throws instead of returning it
- [ ] Validation overhead ≤ 1 ms per generated query (the validator is string-based with no I/O)

### Relationships

- Roadmap row: #31 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql
- Source key: roadmap:31:aql:v1.6.0:1-post-generation-aql-validation-in-translatenlttoaql

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:31:aql:v1.6.0:1-post-generation-aql-validation-in-translatenlttoaql -->
<!-- roadmap-ref: row=31;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql -->
