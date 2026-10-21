---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Trust and Security Boundaries

## Trust boundary

The `api` module sits directly at the external boundary of ThemisDB. It processes client traffic, validates protocol state, and decides which requests are allowed to proceed. This makes it a critical trust boundary even though it does not own the database engine itself.

## Trust assumptions

The module assumes:

- external clients may send malformed or malicious payloads
- protocol capability availability may vary by deployment profile
- auth, token, and scope validation is required before privileged streams are activated
- observability exporters can fail or experience backpressure

These assumptions are reflected in the code: malformed GraphQL documents are rejected, WebSocket CDC access requires a valid auth context, and the gRPC server refuses unsafe startup states.

## Controls already enforced

- GraphQL input validation with limits on size, depth, field count, and AST complexity
- fail-closed error handling for unsupported syntax and invalid documents
- gRPC startup validation before the socket is bound
- WebSocket CDC authorization gating on path and scope
- bounded telemetry export behavior to avoid request-path saturation

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
