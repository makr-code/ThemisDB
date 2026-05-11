# Prompt Engineering Guide (ThemisDB)

This document defines mandatory prompt patterns for reproducible and reviewable AI-assisted implementation.

## 1. Step-by-step decomposition (instead of "implement feature X")

Every implementation prompt MUST be decomposed into explicit stages:

1. **Clarify scope** (files/modules/out-of-scope)
2. **Load context** (ROADMAP, FUTURE_ENHANCEMENTS, relevant headers/tests)
3. **Define change plan** (smallest safe increments)
4. **Implement** (only agreed scope)
5. **Validate** (build/test/lint or documented environment limits)
6. **Review summary** (risks, open points, rollback/removal notes for non-production paths)

## 2. Acceptance criteria and test cases in the prompt (mandatory)

Prompts must include measurable acceptance criteria before implementation:

- Expected runtime behavior
- Inputs, failure cases, and edge cases
- Non-functional goals (performance, security, determinism)
- Explicit test strategy (unit/integration/regression)

### Prompt template

```text
Task: <specific task>
Scope: <affected files/namespaces>
Out of Scope: <clear boundaries>
Acceptance Criteria:
- ...
Test Cases:
- ...
Validation Commands:
- ...
Constraints:
- ...
```

## 3. Checkpoint strategy for complex agent runs

For larger changes, checkpoints are mandatory:

- **Checkpoint A:** analysis complete, plan stable
- **Checkpoint B:** core implementation complete, not finalized
- **Checkpoint C:** validation complete, diff ready for review

Each checkpoint must explicitly confirm scope, risks, and next steps.

## 4. Documentation enforcement (mandatory)

For C++ changes, apply these documentation rules:

- Public APIs must include API-facing documentation (purpose, parameter expectations, return behavior, failure/edge cases)
- When documenting in code, use Doxygen-compatible tags where applicable (`@brief`, `@param`, `@return`, `@throws`, plus `@tparam`/`@requires` for templates/concepts)
- Templates/concepts must document semantic requirements in plain language
- Comments explain **why** (constraints/trade-offs), not only **what**
- Refactoring changes must update relevant existing documentation in the same PR
- Edge-case behavior must be explicitly documented (error paths, null/empty handling)

## 5. Example prompts

### 5.1 Thread-safe queue

```text
Implement a thread-safe FIFO queue in <module> with clear ownership and no raw new/delete.
Acceptance Criteria:
- MPMC safety under concurrent access
- No busy-wait without explicit rationale
- Defined shutdown semantics
Test Cases:
- Producer/consumer concurrency test
- Shutdown during blocking pop
- Spurious wakeup race regression
```

### 5.2 Socket handler

```text
Extend the socket handler in <module> with timeout and explicit error-path handling.
Acceptance Criteria:
- Timeouts map to deterministic error codes
- Resources are released in all failure paths
- No silent failure handling
Test Cases:
- Timeout simulation
- Connection drop during read/write
- Reconnect stress scenario
```

### 5.3 Token bucket algorithm

```text
Implement a token-bucket limiter in <module> with configurable rate/burst.
Acceptance Criteria:
- Correct token refill over time
- Deterministic behavior at boundaries
- Thread-safe usage for concurrent requests
Test Cases:
- Full/empty burst behavior
- Precision under short intervals
- Concurrent acquire() calls
```
