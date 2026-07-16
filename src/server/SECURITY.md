# Security - Server Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

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