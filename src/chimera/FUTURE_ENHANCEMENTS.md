---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera future enhancements

**Purpose / audience:** evidence-led areas for future engineering work. Items below are candidates inferred from current source limitations, not approved commitments or dated roadmap promises.

## Scope and design constraints

Keep simulation, injected-engine, and vendor-driver behavior distinct; unavailable paths must remain explicit. Do not promise backend parity, transaction atomicity, or performance without implementation and test evidence.

## Candidate work and required interfaces

- Add an enforced operation deadline/cancellation contract for retries. Current code classifies concrete error codes, validates backoff settings, requires an explicit idempotency assertion, and caps retry count/backoff; it cannot bound time spent inside the operation callable.
- Decide and document whether transaction helpers are bookkeeping only or should coordinate backend operations; do not describe the current helper as distributed two-phase commit.
- Establish explicit support/build policy for local MongoDB, Neo4j, and Qdrant implementations, including dependency discovery, compile definitions, and behavior when a driver is unavailable.
- Align the main `THEMIS_BUILD_CHIMERA` CMake source list and focused-test registrations with the local source files and the external checkout, then validate each supported configuration.
- Add benchmarks that exercise actual adapter operations. Existing dedicated-gate cases use local stubs; existing adapter benchmarks use the in-process ThemisDB adapter.

## Test strategy

Exercise concrete retryable/non-retryable errors, invalid policy settings, idempotency opt-in, retry exhaustion, transaction-state transitions, conservative adapter capability reporting, and each explicitly supported driver/build combination. Record exact CMake option and test targets.

## Performance and security / reliability

Any future performance target should identify a real benchmark case, reproducible configuration, baseline, and enforced gate. Review external connection configuration, secret handling, resource limits, timeouts, retry behavior, and explicit failure paths for each enabled backend.

## References

`include/chimera/retry_executor.hpp`, `src/chimera/retry_executor.cpp`, `include/chimera/transaction.hpp`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `benchmarks/chimera/*.cpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
