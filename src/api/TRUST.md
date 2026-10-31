---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Trust and Security Boundaries

## Trust boundary

The `api` module supplies transport adapters and request-policy components used at ThemisDB's
external boundary. The primary HTTP listener and general route dispatch are owned by `src/server/`;
API policy components affect only the paths where the host runtime installs them.

## Trust assumptions

The module assumes:

- external clients may send malformed or malicious payloads
- protocol capability availability may vary by deployment profile
- auth, token, and scope validation is required before privileged streams are activated
- observability exporters can fail or experience backpressure

These assumptions are reflected in the code: malformed GraphQL documents are rejected,
`WsChangeHandler` checks CDC authorization before returning an upgrade decision, and the gRPC
server validates its startup configuration. These source-level checks do not prove that every
deployment route has the expected middleware installed.

## Controls already enforced

- GraphQL parser limits on query size, depth, field count, and AST node count; AQL resolver integration has a separate complexity estimator
- fail-closed error handling for unsupported syntax and invalid documents
- gRPC startup validation before the socket is bound
- WebSocket CDC authorization gating on path and scope
- bounded telemetry export behavior to avoid request-path saturation
- `QueryAllowList::isAllowed()` checks membership independently of its `enabled_` flag; its production request-enforcement call site remains unverified

## Risks and residual concerns

The main residual risk is not a catastrophic memory bug; it is a policy mismatch or misconfiguration that allows the wrong input to reach a downstream subsystem. This is why the API module must keep its validation rules explicit and auditable.

## Incident handling

If an incident implicates the API layer:

1. classify the path: GraphQL, gRPC, WebSocket, or exporter-related
2. confirm whether the failure was auth, policy, overload, or malformed input
3. review the request trace and correlation ID before changing enforced policies
4. restore a safe config or rollback an invalid change with the smallest possible blast radius

## Trust posture summary

The API module is effective when it behaves as a strict boundary layer: reject invalid input, preserve explicit failure semantics, and keep optional transport features from silently hiding degraded or unsafe state.
