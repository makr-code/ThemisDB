---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
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

## Integration obligations and edge cases

- Check payload/path bounds and API-version/content-type requirements before dispatch where the shared policy applies.
- Treat unsupported version, malformed input, missing credentials, and unavailable adapter capability as explicit failures.
- `ITransportContract` defaults to no capabilities; callers must inspect a concrete adapter rather than assume optional features.
- Test boundary values and rejected requests using `tests/api/test_api_transport_hardening.cpp` and the nearest protocol test.

See `IMPLEMENTATION.md` and `MODULE_GUIDE.md` for source navigation.
