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

`src/llm_streaming/` is docs-only. The helper is in
`src/llm/streaming_handler.cpp` and `include/llm/streaming_handler.h`. A separate
HTTP/SSE inference route exists at `GET /api/v1/llm/stream`; it buffers the
complete response body before returning it, rather than delivering events
incrementally over the network. The route has no verified disconnect
cancellation or backpressure implementation, and the stub-based module
tests/benchmark do not validate it. Requirements below distinguish this current
behavior from proposed acceptance work.

## Proposed ownership and behavior requirements

- Treat `LLMApiHandler` as the owner of the current HTTP route and
  `EmbeddedLLM` as its inference callback boundary; assign any new transport
  lifecycle behavior to the relevant owner.
- Define and test stream identity, completion, cancellation, disconnect, and
  cleanup behavior in that owner.
- Specify ordering semantics under concurrent callback invocations; the
  helper's atomic index does not serialize sink delivery.
- Decide whether buffered `string_body` delivery is acceptable; if not, define
  incremental writes and their backpressure, disconnect, cancellation, and
  cleanup behavior before implementing them.
- Add diagnostics and operator procedures that correspond to observable
  source behavior.

## Acceptance evidence (not yet demonstrated)

- [x] current HTTP route, caller, and buffered response are mapped to source
- [x] direct helper unit tests exist for escaping, indexes, sink behavior, and
      empty-sink exception semantics
- [ ] execute/maintain those tests and extend coverage for callback copies and
      concurrency semantics
- [ ] route integration tests exercise actual inference callbacks, response
      buffering, terminal/error events, and the selected transport behavior
- [ ] cancellation, disconnect, and backpressure are implemented and tested if
      incremental network delivery is required
- [ ] performance criteria are confirmed, measured, and reported on a stated
      hardware profile; stub benchmark labels are not baselines
- [ ] streaming-specific configuration is either implemented/documented or
      explicitly excluded; current route parameters are documented
- [ ] diagnostics and operational procedures are verified
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

**TODO / assumption:** product owner, deployment profile, incremental-delivery
requirement, and release milestone are not established by the sources reviewed.
