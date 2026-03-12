### Context

This issue implements the roadmap item 'Per-Operation-Type Circuit Breakers' for the aql domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.6.0.

Primary detail section: 3 · Per-Operation-Type Circuit Breakers

### Goal

Deliver the scoped changes for Per-Operation-Type Circuit Breakers in src/aql/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### 3 · Per-Operation-Type Circuit Breakers
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `llm_aql_handler.cpp:Impl` (lines 216–222 and 247–248) creates a single `sharding::CircuitBreaker` instance shared across `executeInfer()`, `executeInferStreaming()`, `executeRAG()`, and `executeEmbed()`. When `executeInfer` accumulates 5 failures (`failure_threshold = 5`), the breaker trips and `allowRequest()` returns false — this blocks all RAG and EMBED commands as well, even if those operations would succeed. The 60-second `timeout` window is also a single global parameter.

**Implementation Notes:**
- `[ ]` In `LLMAQLHandler::Impl`, replace the single `circuit_breaker_` member with a map: `std::unordered_map<std::string, sharding::CircuitBreaker> circuit_breakers_` keyed by `"infer"`, `"rag"`, `"embed"`, `"finetune"`
- `[ ]` Refactor `executeInfer()`, `executeRAG()`, `executeEmbed()` to each look up their own breaker by key
- `[ ]` Allow per-command `CircuitBreaker::Config` to be injected via a `LLMAQLHandler::Config` struct so failure thresholds and windows are tunable per command type
- `[ ]` Add a `getCircuitBreakerStates()` method for observability; expose via `LLM STATS` command output
- `[x]` Circuit breaker state is already recorded in metrics via `metrics.recordCircuitBreakerState("infer", "open")` — preserve and extend to all command types

---

### Acceptance Criteria

- [ ] In `LLMAQLHandler::Impl`, replace the single `circuit_breaker_` member with a map: `std::unordered_map<std::string, sharding::CircuitBreaker> circuit_breakers_` keyed by `"infer"`, `"rag"`, `"embed"`, `"finetune"`
- [ ] Refactor `executeInfer()`, `executeRAG()`, `executeEmbed()` to each look up their own breaker by key
- [ ] Allow per-command `CircuitBreaker::Config` to be injected via a `LLMAQLHandler::Config` struct so failure thresholds and windows are tunable per command type
- [ ] Add a `getCircuitBreakerStates()` method for observability; expose via `LLM STATS` command output
- [ ] Circuit breaker state is already recorded in metrics via `metrics.recordCircuitBreakerState("infer", "open")` — preserve and extend to all command types

### Relationships

- Roadmap row: #33 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#3--per-operation-type-circuit-breakers
- Source key: roadmap:33:aql:v1.6.0:3-per-operation-type-circuit-breakers

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:33:aql:v1.6.0:3-per-operation-type-circuit-breakers -->
<!-- roadmap-ref: row=33;module=aql;target=v1.6.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#3--per-operation-type-circuit-breakers -->
