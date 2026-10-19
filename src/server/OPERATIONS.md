---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Server Operations Guide

## Overview

The `server` module is the runtime boundary of ThemisDB. In operations, it is the layer that surfaces request health, auth outcomes, quota behavior, and overload policy to the rest of the system. This makes it a high-visibility module for both customer-facing availability and internal reliability.

## Operational focus areas

### 1. Request admission

The combined behavior of `api_gateway.cpp`, `rate_limiting_middleware.cpp`, and `load_shedder.cpp` controls whether traffic is admitted. Operators should expect explicit 401, 403, 429, 503, and 504 responses under configured conditions.

### 2. Session health

WebSocket, MQTT, PostgreSQL-wire, and gRPC session code should be monitored for protocol disconnects, stale sessions, and elevated teardown rates. These are often the first signs of connection or resource exhaustion.

### 3. Endpoint health

Handlers under `src/server/` should be assessed for latency, failure rate, and auth policy conformance. Most operational problems manifest as route-level rejection or abnormal downstream latency rather than a storage-layer failure.

## Monitoring signals

Useful signals include:

- request volume and rate-limit rejection counts,
- load-shed decisions and CPU/memory pressure,
- auth failures by provider and endpoint,
- protocol disconnect and session teardown counts,
- per-endpoint latency and error rate,
- versioned-route normalization anomalies.

## Standard response patterns

### Elevated 429 rates

Likely causes:

- overload or quota exhaustion,
- client misbehavior or credential reuse,
- route-specific configuration mismatch.

Response:

- inspect the limiter config,
- identify the hot path and client distribution,
- confirm whether the workload is legitimate or a traffic misfire.

### Elevated 503 rates

Likely causes:

- CPU or memory saturation,
- queue backlog,
- upstream backend or sharding degradation.

Response:

- inspect load-shedder state,
- reduce load or fail over traffic,
- verify that the unhealthy dependency chain is not silently being proxied through the server layer.

### Elevated auth failures

Likely causes:

- expired or inactive credentials,
- bad issuer/audience mapping,
- misconfigured role or scope binding.

Response:

- validate config and token metadata,
- confirm that endpoint requirements match the caller identity,
- examine the active auth provider without expanding permissions.

## Maintenance guidance

- keep gateway and auth logic versioned and predictable,
- validate routing changes with endpoint-level tests,
- avoid broadening access contracts in hotfixes,
- maintain enough runtime visibility to distinguish auth, overload, and protocol faults.

## Release and change considerations

Any server change that affects routing, auth, or request admission should be treated as a release-critical change. These changes are not purely cosmetic; they affect public behavior and can produce visible user-facing failures if misconfigured.

## Summary

The server module should be operated as a guarded runtime boundary: if it is healthy, the rest of ThemisDB is more likely to remain stable under traffic and partial failures. If it is unhealthy, the first symptoms will appear as explicit authentication, overload, or route behavior rather than as deep internal logic faults.
