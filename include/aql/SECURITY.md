<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — AQL Module Public Headers

**Module Path:** `include/aql/`
**Implementation Security:** `../../src/aql/SECURITY.md`

---

## Scope

Security considerations for the public AQL header API surface. Covers prompt injection,
LLM output validation, token budget enforcement, and sensitive data handling in queries.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Prompt injection | Malicious natural language input to `ILLMAQLHandler` | `aql_query_validator.h` — validation before execution |
| AQL injection via builder | Unsanitised string concatenation | `aql_query_builder.h` — parameterised builder API, no raw string concat |
| LLM token DoS | Unbounded input to `IAsyncLLMBackend` | `llm_token_estimator.h` — budget check before dispatch |
| LLM timeout DoS | Unbounded inference latency | `llm_timeout_manager.h` — configurable timeout policy per operation |
| Schema exfiltration via autocomplete | Autocomplete reveals schema to unauthorised user | `aql_schema_provider.h` — schema access requires tenant context |
| Conversation context leakage | Cross-user context mixing in `IAQLConversationContext` | Context keyed by session ID + tenant; isolation is a contract requirement |
| LoRA model poisoning | Malicious fine-tuning data | `aql_lora_finetuner.h` — requires `THEMIS_ENABLE_LORA` and operator approval |
| Confidence score manipulation | Attacker crafts queries with artificially high confidence | `IAQLConfidenceScorer` contract requires entropy-based scoring, not user-supplied |

---

## Security Controls

### Input Validation
`IAQLQueryValidator::validate()` must be called on all queries before execution;
`ValidationResult` includes rejection reasons that must not be surfaced verbatim to users.

### Token Budget Enforcement
`ILLMTokenEstimator::estimate()` must be called before `IAsyncLLMBackend::submit()`;
requests exceeding the per-tenant token budget are rejected.

### Schema Access Control
`IAQLSchemaProvider::getSchema(tenant_id)` enforces tenant-scoped schema visibility;
cross-tenant schema access is a contract violation.

### Conversation Isolation
`IAQLConversationContext` is keyed by (`session_id`, `tenant_id`); implementations must
reject `addTurn()` calls for mismatched tenant IDs.

---

## Known Limitations

- LLM output is non-deterministic; `IAQLQueryValidator` provides a safety net but cannot
  prevent all hallucinated query constructs.
- LoRA fine-tuning pipeline security (training data provenance, model signing) is
  operator-managed and outside the header contract scope.
- Multimodal inputs (`multimodal_infer_request.h`) may carry embedded PII in images or
  audio; callers are responsible for redaction before submission.
