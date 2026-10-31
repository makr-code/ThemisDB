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

# LLM Streaming Module Roadmap

## Current Status

`src/llm_streaming/` is docs-only. The checked implementation is a formatting
and callback helper in `src/llm/streaming_handler.cpp`, declared in
`include/llm/streaming_handler.h`; its actual production caller and transport
integration were not established. The module stress test, integration soak,
and dedicated benchmark use in-process stubs and do not call this helper.

- [~] Document the helper API and its limits.
- [ ] Identify and document the runtime owner, caller, and transport integration.
- [~] Maintain direct helper unit coverage; add runtime integration tests if an
      owning path is established.
- [ ] Establish verified runtime behavior and release evidence before making
      production-readiness claims.

### Source Reality Check (2026-10-31)

- `src/llm_streaming/` contains Markdown files and `.gitkeep`, not `.cpp` or
  `.h` implementation files.
- The helper supports SSE string formatting, a `[DONE]` string, HTTP chunk
  framing, and a sink callback adapter. It does not implement session
  lifecycle, cancellation, backpressure, server routing, or gRPC.
- The stub tests model counters, a bounded queue, and synthetic sequence checks.
  Their metrics and outcomes are not evidence of runtime performance.
- `tests/llm/test_streaming_handler.cpp` tests the helper directly, but not
  end-to-end transport, cancellation, backpressure, or concurrent sink order.
- The module-focused test CMake globs local `test_*.cpp` stubs, registers them
  at stress tier with a 300-second timeout, and does not link the `llm`
  implementation.

## In Progress / Planned

- [~] Maintain source-backed API documentation and explicitly mark unknowns.
- [ ] Trace the helper's real caller and determine the canonical runtime owner.
- [ ] Specify callback concurrency and whether the sink must preserve order.
- [ ] Decide whether lifecycle, cancellation, disconnect, backpressure, and
      diagnostics are in scope for the owning runtime.
- [x] Basic unit tests exercise the helper's public formatting/callback API.
- [ ] Add tests for callback-copy/concurrency semantics and any established
      end-to-end path.

## Implementation Phases

### Phase 1: API and ownership clarification

- [x] Inventory the public `StreamingHandler` declarations and implementation.
- [~] Record a provisional ownership map; actual caller/integration remains
      unverified.
- [ ] Confirm callback concurrency and sink contract.

### Phase 2: Runtime integration (planned; owner not established)

- [ ] Identify approved canonical runtime owner and actual request-to-sink path.
- [ ] Implement only the lifecycle and transport behavior assigned to that owner.
- [ ] Define cancellation, completion, and error propagation based on source.

### Phase 3: Reliability and security validation (planned)

- [ ] Test JSON/SSE formatting, sink errors, callback copies, and concurrent use.
- [ ] If runtime features exist, validate cancellation, disconnect,
      backpressure, cleanup, and delivery ordering at their actual boundary.
- [ ] Document configuration and observability only when implemented.

### Phase 4: Tests (planned)

- [x] Direct unit tests for `StreamingHandler` exist.
- [ ] Extend direct helper tests for callback copies and concurrent invocation.
- [ ] Add integration coverage only after a canonical runtime owner and actual
      request-to-sink path are established.
- [ ] Keep stub stress and soak coverage clearly separate from helper/runtime
      evidence.

### Phase 5: Performance / Hardening (planned)

- [ ] Confirm whether benchmark source labels are intended requirements; they
      are currently stub labels, not executable gates or verified thresholds.
- [ ] Define measurable performance criteria and a hardware profile after
      product ownership and runtime behavior are established.
- [ ] Capture representative runtime measurements and hardening evidence before
      setting release thresholds.
- [ ] Validate any implemented concurrency, cancellation, backpressure, and
      cleanup behavior at its actual runtime boundary.

### Phase 6: Documentation & Acceptance (planned)

- [ ] Align public API and operational docs with verified runtime behavior.
- [ ] Document the approved runtime owner, caller, configuration, and supported
      transport from canonical sources.
- [ ] Require traceable implementation, tests, and operational evidence for
      any production claim.

## Production Readiness Checklist

- [ ] Runtime owner, caller, and supported transport are source-traceable.
- [ ] Public helper is directly tested; integration behavior is tested where
      applicable.
- [ ] Any lifecycle, cancellation, backpressure, and ordering guarantees are
      implemented and validated in the owning path.
- [ ] Runtime performance criteria and representative measurements are
      documented.
- [ ] Configuration, diagnostics, and operational procedures are verified.

## Known Issues & Limitations

1. `src/llm_streaming/` has no runtime implementation.
2. The helper's atomic index does not serialize concurrent sink delivery.
3. Stress/soak fixtures and benchmark operate on local stubs, not the helper.
4. No runtime configuration, lifecycle/error API, metrics, or deployment
   limits were established by the sources reviewed.
5. Values printed by the stub benchmark are labels, not executable gates,
   verified thresholds, release targets, or results.

## Breaking Changes

No runtime contract change is established by this documentation. Any future
API or runtime changes must be recorded against the canonical implementation
and its tests.
