---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Server Trust and Security Boundaries

## Trust boundary

The `server` module is the trust boundary between external traffic and internal ThemisDB runtime execution. It is the first component that decides whether an inbound request is valid, permitted, and safe to process. Because of this, server quality is a direct security property.

## Boundaries and risks

### Auth boundary

`AuthMiddleware` is responsible for validating external identity and scope. This is the primary trust boundary for access control. The risk if this boundary fails is broad and immediate: unauthorized requests can reach server handlers and downstream modules.

### Admission boundary

`RateLimitingMiddleware` and `LoadShedder` protect the server from overload and abusive request patterns. The risk is service degradation, expensive backend work, and denial-of-service conditions when this layer is misconfigured or bypassed.

### Routing boundary

`APIGateway` decides whether a request is local or sharded. The risk here is path confusion, invalid route selection, and inconsistent API behavior across versions. The code aims to normalize and validate before dispatch.

### Protocol boundary

WebSocket, MQTT, PostgreSQL-wire, and gRPC endpoints each own transport state and may be attacked differently from HTTP requests. The risk is session hijack, stale connections, or service exhaustion if protocol lifecycle is not maintained carefully.

## Controls in place

The server module relies on several hard controls:

- explicit auth and authorization checks,
- content- and endpoint-specific rate limiting,
- load-based rejection of low-priority traffic,
- normalization of versioned paths before route selection,
- clear response status codes that signal invalid requests or overload,
- protocol-local session ownership to avoid global cross-session state leakage.

## Operational trust assumptions

The server layer assumes the following:

- auth providers are configured correctly and are not bypassed,
- rate limit and queue thresholds remain aligned with real production workload,
- session handling is bounded to protocol-specific state,
- service-level denials remain explicit and distinguishable from backend issues,
- operators have enough visibility to separate auth, quota, and overload incidents.
- production uses secure authentication defaults rather than the development profile, which disables auth and endpoint-specific restrictions.
- route-level public exceptions (including health/status endpoints) are intentional and their metadata is safe to expose.
- any Kerberos `fallback_to_basic` setting is explicitly reviewed as a change to the authentication boundary.

## Incident handling

If an incident is suspected in this module:

1. classify whether it is auth, rate-limit, overload, route, or session-related,
2. verify the request path and auth state at the gateway layer,
3. inspect quota and load-shed metrics before changing operational policy,
4. confirm whether the issue is client-facing or service-facing,
5. apply the smallest rollback or configuration correction that restores explicit safety guarantees.

## Summary

The `server` module mediates external activity, but its effective posture depends on the selected auth profile, route policy, enabled protocol, and deployment configuration. `createDevDefaults()` is not suitable for an untrusted production-facing interface; secure defaults still include specific public, rate-limited status endpoints.

## Secret handling and incident response

Treat tokens, provider credentials, and TLS private keys as bearer secrets. Restrict their source, runtime exposure, and rotation permissions. During an incident, identify the affected route and protocol, revoke credentials, preserve audit evidence, and verify scope enforcement and public-route exposure before reopening traffic. Report vulnerabilities privately through [project SECURITY.md](../../SECURITY.md); use [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Source references

- `src/server/auth_middleware.cpp`
- `src/server/api_auth_config.cpp`
- `src/server/rate_limiting_middleware.cpp`
- `src/server/voice_api_handler.cpp`
- [Module security summary](SECURITY.md)
