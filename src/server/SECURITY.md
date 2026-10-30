---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security - Server Module

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page describes server controls, not product-wide certification.

## Security Scope

Security in the server module focuses on authentication and request-control middleware, explicit rejection and overload behavior, bounded endpoint exposure, and observable failure handling for client-facing transports and APIs.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthenticated or misrouted request handling | gateway and auth middleware behavior |
| request flood and overload | rate-limiting, adaptive throttling, and load-shedding surfaces |
| hidden endpoint failure or unsafe degradation | explicit handler and health/error signaling |
| protocol/session abuse on server-owned transports | bounded session/runtime behavior for WebSocket, MQTT, PostgreSQL wire, and gRPC |

## Implemented Security Controls

- authentication and authorization decisions are handled in dedicated middleware paths.
- request throttling and overload behavior are isolated into server-owned control surfaces.
- health/error services keep server-state signaling explicit during degraded operation.
- endpoint and session behavior remain separated from unrelated storage or query internals.

## Security Follow-ups

- deepen validation for overload and request-flood edge scenarios across mixed endpoint traffic.
- tighten diagnostics consistency across auth, throttle, and health/error rejection paths.
- broaden protocol-specific hardening coverage for server-owned session surfaces.
- `ApiAuthConfig::createSecureDefaults()` enables authentication and assigns scopes to protected routes. `createDevDefaults()` disables authentication and removes endpoint-specific restrictions; never use the development profile on an untrusted interface.
- Health/version and several status endpoints are public in the secure defaults and remain rate-limited; restrict their exposure if their metadata is sensitive.
- Kerberos configuration exposes a `fallback_to_basic` option. Review it explicitly; a weaker fallback must not silently broaden accepted identities.
- Bearer/API tokens, JWTs, provider secrets, and TLS private keys are sensitive runtime inputs. Keep them out of source, logs, command arguments, and public diagnostics.

## Incident Handling

For suspected ingress/auth compromise, restrict or disable the affected route/listener, revoke exposed credentials, preserve auth/audit and rate-limit evidence, and verify the active auth profile and per-route scopes before restoring access. Follow [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Sourcecode Verification (Module: server/security)

- Verified files:
  - src/server/auth_middleware.cpp
  - src/server/api_auth_config.cpp
  - src/server/rate_limiting_middleware.cpp
  - src/server/adaptive_rate_limiter.cpp
  - src/server/load_shedder.cpp
  - src/server/health_error_service.cpp
  - src/server/websocket_session.cpp
  - src/server/mqtt_session.cpp
- Verified controls:
  - dedicated auth and request-control middleware
  - explicit overload and health signaling
  - bounded server-owned session/runtime surfaces

- Additional source references:
  - `src/server/api_auth_config.cpp`
  - `src/server/auth_middleware.cpp`
  - `src/server/voice_api_handler.cpp`
  - [Trust and security boundaries](TRUST.md)