---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# LLM Streaming — Production Requirements (Proposed)

## Current status

`src/llm_streaming/` is docs-only. The checked runtime source is the
`StreamingHandler` formatting/callback helper in `src/llm/streaming_handler.cpp`
and `include/llm/streaming_handler.h`. There is no verified end-to-end streaming
runtime, cancellation API, backpressure configuration, or network integration
in the examined sources. Requirements below are acceptance proposals, not
implemented product behavior.

## Proposed ownership and behavior requirements

- Identify the canonical owner and actual caller before assigning lifecycle
  responsibilities.
- Define and test stream identity, completion, cancellation, disconnect, and
  cleanup behavior in that owner.
- Specify ordering semantics under concurrent callback invocations; the
  helper's atomic index does not serialize sink delivery.
- Define backpressure limits, error propagation, configuration, and recovery
  only if a runtime queue/controller is implemented.
- Add diagnostics and operator procedures that correspond to observable
  source behavior.

## Acceptance evidence (not yet demonstrated)

- [ ] runtime owner and caller are mapped to source and a supported transport
- [x] direct helper unit tests exist for escaping, indexes, sink behavior, and
      empty-sink exception semantics
- [ ] execute/maintain those tests and extend coverage for callback copies and
      concurrency semantics
- [ ] runtime integration tests exercise actual delivery, terminal behavior,
      cancellation, disconnect, and any backpressure implementation
- [ ] performance criteria are confirmed, measured, and reported on a stated
      hardware profile; stub benchmark labels are not baselines
- [ ] configuration, diagnostics, and operational procedures are verified
- [ ] release claims are traceable to the owning implementation and evidence

## Existing validation and build references

- Direct helper unit test:
  `tests/llm/test_streaming_handler.cpp`
- Focused stub test source:
  `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- Integration soak stub source:
  `tests/integration/test_llm_streaming_soak.cpp`
- Stub benchmark:
  `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`
- CMake registration: `tests/llm_streaming/CMakeLists.txt`,
  `tests/integration/CMakeLists.txt`, and
  `benchmarks/llm_streaming/CMakeLists.txt`

The unit test is evidence for its individual helper cases. The stub stress,
soak, and benchmark sources are not acceptance evidence for a production
runtime.

**TODO / assumption:** product owner, deployment profile, exact runtime
integration, and release milestone are not established by the sources reviewed.
