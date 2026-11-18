---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-11-16"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/api/api_transport_contracts.h`, `include/api/api_transport_policy.h`, `include/api/grpc_server.h`, `include/api/ws_handler.h`, `include/api/api_error_taxonomy.h`
Related issues: #7093, #7167

# API transport contract

## Shared transport contract

`ITransportContract` exposes adapter capabilities, supported versions, failure classification, and an adapter name. The shared constants declare API versions v1/v2, a 10 MiB maximum payload, a 4096-byte path limit, and an 8192-byte header-value limit. The policy middleware may lower the payload/path limits, but normalization caps them at the shared maxima.

`TransportFailureClass` distinguishes malformed requests, oversized payloads, unsupported versions, content-type issues, unauthorized requests, rate limiting, unavailable capabilities, and internal errors. Keep these classes distinct when mapping responses.

## Adapter surfaces

- `TransportPolicyMiddleware` wraps an `IHttpHandler`; its `handle()` returns `Result<HttpResponse>`.
- `GrpcApiServer` exposes `initialize`, service registration, `start`, `stop`, and `isRunning`; initialization/start report success as `bool`.
- `WsChangeHandler::validate()` returns an `UpgradeDecision`; its fields include upgrade decision, rejection status/reason, and parsed subscription context.
- GraphQL and other protocol APIs are defined in their matching `include/api/` headers; confirm the declaration before relying on behavior not listed here.

## gRPC search cancellation

- `VectorSearch`, `FilteredVectorSearch`, and `HybridSearch` map the gRPC
  deadline to the query request context and poll `ServerContext::IsCancelled()`
  without creating a watcher thread.
- These RPCs use the cancellation-aware `IVectorIndex::search` overload.
  Existing implementations use its checkpoint-only default (before/after the
  legacy synchronous call); in-flight interruption requires an implementation
  that polls the request context. An observed stop returns gRPC `CANCELLED` or
  `DEADLINE_EXCEEDED`, with no partial hit list returned.
- `FullTextSearch` and the sparse AQL stage of `HybridSearch` call
  `IQueryEngine::execute(query, bind_vars_json, RequestContext, StopReason&)`.
  The default implementation is checkpoint-only (check before/after the legacy
  call, result discarded at the caller when a stop is observed); AQL engines that
  do not override it cannot be interrupted mid-call. Cooperative in-flight
  cancellation requires an override that polls `RequestContext::shouldStop()`.
  Any observed stop returns `CANCELLED`/`DEADLINE_EXCEEDED`, `success=false`
  and no hits; for `HybridSearch`, hits from the dense stage are cleared too.
- Unary `ExecuteAQL` and server-streaming `StreamAQL` build a request context
  from the gRPC `ServerContext` (polled synchronously, no worker thread) and call
  the same context-aware `IQueryEngine::execute`. Order: transaction/empty-query/
  engine-wired checks (unchanged), stop check before dispatch, execute, stop
  check, then polling after JSON parsing and between rows. Cancellation maps to
  `CANCELLED` and deadline to `DEADLINE_EXCEEDED`; `ExecuteAQL` additionally
  returns `success=false`, an error code 499/408 and no rows (rows cleared on any
  stop), `StreamAQL` stops emitting rows and returns the status. A failed
  `writer->Write` returns the observed cancellation/deadline status, or
  `CANCELLED` if no stop reason is observed. Legacy synchronous engines remain
  checkpoint-only: a running call is not interrupted, only its result is
  discarded when a stop is observed afterwards. Bind variables are still not
  forwarded (`"{}"`), as before.
- HTTP search routes and other gRPC search methods do not yet bind a request
  context; see the QRY-07 status in `src/search/ROADMAP.md`.

## Integration obligations and edge cases

- Check payload/path bounds and API-version/content-type requirements before dispatch where the shared policy applies.
- Treat unsupported version, malformed input, missing credentials, and unavailable adapter capability as explicit failures.
- `ITransportContract` defaults to no capabilities; callers must inspect a concrete adapter rather than assume optional features.
- Test boundary values and rejected requests using `tests/api/test_api_transport_hardening.cpp` and the nearest protocol test.

See `IMPLEMENTATION.md` and `MODULE_GUIDE.md` for source navigation.
