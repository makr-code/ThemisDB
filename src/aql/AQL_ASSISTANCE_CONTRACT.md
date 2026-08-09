# AQL Assistance Module — Frozen Contract Semantics

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document defines the **frozen contract semantics** for the AQL Assistance
module's public translation and validation APIs, and the **explicit failure
contracts** for unsupported provider and capability modes.

Once frozen, changes to any semantic defined here require a version bump and
a corresponding CHANGELOG entry.

---

## 2. Translation Output Contract

### 2.1 `LLMAQLHandler::translateNLToAQL()`

**Return value semantics:**

| Condition | Return |
|---|---|
| Successful translation | Non-empty AQL string, syntactically valid per the AQL grammar |
| Empty natural-language input (after trim) | `""` — empty string, no exception |
| Input exceeds prompt size limit | throws `LLMException(PROMPT_TOO_LONG)` |
| Input contains injection patterns | throws `LLMException(PROMPT_INJECTION)` |
| LLM backend unavailable | throws `LLMException(INFERENCE_FAILED)` |
| Generated output fails validation | throws `LLMException(INVALID_RESPONSE)` |

**Determinism:** The function is **not** deterministic across LLM provider invocations.
Callers requiring reproducibility must fix the temperature and seed via
`LLMAQLHandler::Config::temperature` and `LLMAQLHandler::Config::seed`.

**Thread safety:** Concurrent calls to `translateNLToAQL()` from multiple threads
are safe provided each thread owns a separate `LLMAQLHandler` instance.
Shared-instance concurrent calls are **not** supported and will produce
undefined behaviour.

### 2.2 `LLMAQLHandler::translateNLToAQLWithConfidence()`

Returns `AQLTranslationResult { aql_query, confidence }` where:

- `aql_query` follows the same semantics as § 2.1
- `confidence.score` is in range `[0.0, 1.0]`
- `confidence.score == 0.0` indicates an unscored result (scorer unavailable)
- `confidence.score < 0.5` is the threshold below which the query SHOULD be
  discarded or flagged for human review

### 2.3 `LLMAQLHandler::translateNLToAQLStreaming()`

- Streams LLM explanation tokens via the registered callback
- The final returned `std::string` is identical to `translateNLToAQL()`
- Partial streamed output MUST NOT be executed as AQL — only the final return
  value is validated and safe for execution
- Exception semantics identical to § 2.1

---

## 3. Validation Output Contract

### 3.1 `LLMAQLHandler::validateAQLWithParser()`

**Return value semantics:**

| Condition | Return |
|---|---|
| Syntactically valid AQL | `true` |
| Syntactically invalid AQL | `false` (no exception) |
| Empty input (after trim) | `false` |
| Parser unavailable (no `IAQLParserBackend` set) | `false`, logs `WARN` |

The validation call does **not** execute the query against a live database.
Semantic errors (wrong collection name, missing field) are NOT detected.

---

## 4. Failure Contracts — Unsupported Provider / Capability Modes

### 4.1 Error Codes

Two dedicated error codes are frozen in `LLMErrorCode` (6xxx range):

| Code | Value | Semantics |
|---|---|---|
| `PROVIDER_UNSUPPORTED` | 6001 | The configured LLM provider does not support the requested operation mode |
| `CAPABILITY_UNSUPPORTED` | 6002 | The model or adapter does not expose the requested capability |

### 4.2 `PROVIDER_UNSUPPORTED` — Triggering Conditions

Thrown as `LLMException(PROVIDER_UNSUPPORTED, <message>)` when:

- Streaming is requested (`translateNLToAQLStreaming`) but the configured
  provider does not support streaming completions
- Batch translation is requested but the provider supports only single-shot
  completions
- Context-window extension is requested but the provider does not support it

**Caller contract:**
- `PROVIDER_UNSUPPORTED` is **non-retryable**
- Callers MUST NOT retry with the same provider configuration
- Callers SHOULD fall back to a non-streaming / single-shot equivalent or
  surface a configuration error to the operator

### 4.3 `CAPABILITY_UNSUPPORTED` — Triggering Conditions

Thrown as `LLMException(CAPABILITY_UNSUPPORTED, <message>)` when:

- LoRA fine-tuning is requested on a model that is not LoRA-capable (e.g.,
  a quantized base model without adapter slots)
- A feature requiring a specific model capability flag is requested but that
  flag is absent from the loaded model's metadata

**Caller contract:**
- `CAPABILITY_UNSUPPORTED` is **non-retryable**
- Callers MUST surface this as a configuration error
- The specific unsupported capability name MUST appear in the exception message

### 4.4 Degraded-Mode vs. Hard-Failure Decision Table

| Condition | Behaviour | Error Code |
|---|---|---|
| Provider unavailable (transient network) | Retry up to `Config::max_retries` → `INFERENCE_FAILED` | 4001 |
| Provider unsupported mode | Hard fail immediately | 6001 |
| Capability absent from model | Hard fail immediately | 6002 |
| Provider timeout | Retry → `TIMEOUT` | 4004 |
| Prompt too long | Hard fail immediately | 1002 |

---

## 5. Backward Compatibility

- The types and error codes defined in this document are frozen as of v1.6.0.
- New output fields may be **added** to result structs without a major version bump
  (additive changes only).
- Removal or semantic change of any field or error code requires a **major version bump**
  (v2.x) and an explicit deprecation notice in CHANGELOG.md.

---

## 6. Contract Validation

Tests enforcing this contract are in:

- `tests/aql/test_aql_assistance_contract_focused.cpp` (contract smoke tests)
- `tests/aql/test_aql_assistance_phase4_focused.cpp` (error handling paths)

See also: `include/aql/llm_error_codes.h` (frozen error taxonomy),
`include/aql/llm_aql_handler.h` (translation/validation API signatures).
