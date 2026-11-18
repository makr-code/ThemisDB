---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-11-18"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# API Module Contract

This is a source-backed summary of selected public contracts for `include/api/`. It does not
promise that every API surface is installed or enabled in a deployment. The primary HTTP listener,
route dispatch, authentication integration, query execution, and storage are owned by other
modules.

## Contract and validation

| API | Input and behavior | Failure / limitation |
|---|---|---|
| `kApiMajorVersion`, `kSupportedApiVersions` | The declared API major is `2`; the transport version header accepts `v1` and `v2`. An absent or empty version is accepted. | Acceptance of an absent header does not declare which default version an individual route implements. |
| `TransportContractValidator::validate()` | Rejects an empty method/path, path longer than 4096 bytes, body larger than 10 MiB, unsupported non-empty version, and a missing Content-Type for a non-empty POST/PUT/PATCH body. | It does not enforce the declared 8192-byte header-value constant, check a Content-Type value, authenticate, authorize, or validate every protocol adapter. |
| `ITransportContract` defaults | `supportedVersions()` returns `v1`/`v2`; `capabilities()` returns `None`; `classifyFailure()` returns `None`. | The default `classifyFailure()` is not validation. Callers must use `TransportContractValidator` or an adapter's enforcing policy; `None` from this default is not evidence that a request or capability is valid. |
| `TransportPolicyMiddleware::handle()` | Opt-in `IHttpHandler` decorator. Checks non-empty method/path, configured path and body bounds, an optional `X-API-Version` header, and non-empty Content-Type for non-empty POST/PUT/PATCH bodies. It forwards valid requests to the inner handler. | It returns a `Result` error on policy rejection and does not itself produce an HTTP response. It checks header presence, not media-type validity; it does not enforce authentication or authorization. The host must install it in the request chain. |
| `TransportPolicyConfig` | Payload/path limits are capped at the shared maxima (10 MiB / 4096 bytes); lower limits are allowed. The version and Content-Type checks can be disabled. | It is not a global server configuration. The header-size constant is not checked by this policy. |
| `TransportPolicyMiddleware::requiresAuthentication()` | Delegates to the wrapped handler. | A `false` result is possible when the wrapped handler does not require authentication; the policy decorator does not add authentication. A null wrapped handler throws `std::invalid_argument` at construction. |

`ApiErrorTaxonomy` maps `MalformedRequest` and `UnsupportedVersion` to HTTP 400,
`PayloadTooLarge` to 413, missing/mismatched Content-Type to 415, `Unauthorized` to 401,
`RateLimitExceeded` to 429, `CapabilityUnavailable` to 501, and `InternalError` to 500. The
middleware currently creates failures for malformed request, oversized payload, unsupported
version, and missing Content-Type; it does not create every taxonomy class.

## Optional adapters and lifecycle

- gRPC and WebSocket surfaces are build-gated by `THEMIS_ENABLE_GRPC` and
  `THEMIS_ENABLE_WEBSOCKET`; their presence in headers or source does not prove availability in a
  running build. gRPC service methods can return `UNIMPLEMENTED` when required collaborators are
  not wired.
- The main HTTP listener and general route/middleware installation live in `src/server/`. API
  helper tests do not establish that production routes install the intended policy or auth chain.
- `WsChangeHandler::validate()` returns an upgrade decision; the server's HTTP adapter performs
  the actual WebSocket protocol upgrade. CDC authorization is checked by that handler before it
  returns an upgrade decision.
- GraphQL parsing accepts named fragments and `@include`/`@skip`; inline fragments and other
  directives are rejected. Parsing/execution support does not by itself establish schema-level
  authorization or production `QueryLimits` configuration.

## OTLP exporter bounds and loss semantics

`OtlpExporter` is disabled by default. When enabled, `start()` launches the background flush
thread; `stop()` signals it, drains queued spans, and joins the thread. `enqueue()` is a no-op
when disabled. The queue is bounded by `max_queue_size`; when full, the oldest span is dropped and
the dropped counter is incremented. Export failures do not change database-operation results.
The current stop-time drain may put more than `batch_size` spans in one export request, and the
join waits for export retries. Owners must stop producers before calling `stop()`; concurrent
enqueue/shutdown behavior is not established by a production lifecycle test.
At maximum configuration, one batch may occupy the worker for up to 16 minutes (11 × 60-second
requests plus ten 30-second retry waits); shutdown can take longer while draining queued spans.

The constructor rejects `max_queue_size == 0`, `batch_size == 0`, non-positive flush intervals,
timeouts outside 1–60000 ms, retry counts outside 0–10, and initial back-off outside 1–30000 ms.
There are at most 11 attempts per batch; back-off doubles but is capped at 30000 ms. HTTP 429/503
and selected curl transport errors are retried; other HTTP failures are counted as dropped after
the current attempt. Dropped spans are observable through `droppedSpanCount()` and logs.

The endpoint and TLS/auth values are not all validated at construction. An invalid or unreachable
collector can therefore result in exported spans being dropped rather than preventing database
startup; operators must monitor drop counts and configure the endpoint separately.

## Compatibility and release gates

- The v2.5.0 target is conditionally compatible: public signatures and declared `v1`/`v2`
  behavior remain unchanged, but exporter configurations outside the new documented ranges,
  including retry counts above 10, now throw during construction. Update those deployment settings
  before rollout.
- A v3.0.0 breaking contract is not authorized by this target alone. Before changing/removing
  symbols or changing default negotiation/error shapes, inventory consumers, approve migration
  notes, and add compatibility/conformance tests.
- Correctness gates: test supported/unsupported/absent versions, invalid request boundaries,
  policy rejection before inner dispatch, and exporter config limits. Tests in
  `test_api_degraded_mode.cpp` and `test_api_wave_d_stress.cpp` use fixture/simulation paths and
  are not production-adapter integration evidence.
- Runtime qualification gates: run focused API tests in every supported build profile; test
  production route/auth wiring and enabled/disabled optional transports; retain a dated benchmark
  result with workload, offered load, build/host details, p50/p95/p99, throughput, and errors.
  Benchmark source comments and target labels alone are not a performance pass.

## Evidence sources

- Contract: `include/api/api_transport_contracts.h`,
  `include/api/api_transport_policy.h`, `include/api/api_error_taxonomy.h`
- Implementation: `src/api/api_transport_policy.cpp`, `src/api/otlp_exporter.cpp`,
  `src/api/ws_handler.cpp`, `src/api/themisdb_grpc_service.cpp`
- Focused tests: `tests/api/test_api_phase4_concurrency.cpp`,
  `tests/api/test_api_degraded_mode.cpp`, `tests/api/test_api_otlp_exporter_config.cpp`
- Test registration: `tests/api/CMakeLists.txt` (auto-discovers `test_*.cpp`; CTest label `api`)
- Performance evidence limits: `src/api/PERFORMANCE_EXPECTATIONS.md` and
  `src/api/ROADMAP.md`
