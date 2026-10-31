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

# API Module Runbook

## Purpose

This runbook covers the operator-facing failure modes most likely to appear in the `api` module. It is meant to help diagnose transport-level incidents without needing to read the full implementation of every adapter. The emphasis is on explicit action: identify the failure class, confirm the boundary, recover the service, and only then adjust configuration or code.

## Scenario 1: malformed GraphQL request

### Symptoms

- GraphQL parser rejects the document with a syntax or validation error
- client sees a validation error instead of a normal query result
- logs show query size, depth, field, or AST limit violations

### Evidence

Inspect the parser result and the limits passed by the caller to `Parser::parse()` in `src/api/graphql.cpp`.
The module does not guarantee that logs include query size or limit values.

### Recovery

1. Validate the payload size and nesting structure.
2. Named fragments and spreads (after their definitions) and the `@include`/`@skip` field
   directives are supported. Inline fragments and other directives are rejected; also check
   configured size, depth, field, and AST-node limits.
3. If this is a legitimate query, increase the operational limit only in a controlled change and re-run validation tests.
4. If the request is malicious or malformed, reject it and collect the trace ID for the incident record.

## Scenario 2: gRPC startup or registration failure

### Symptoms

- server fails to start
- gRPC service not serving requests
- log shows invalid host, missing service, or TLS misconfiguration

### Evidence

The failure path is in `src/api/grpc_server.cpp`: initialization validates host, port, TLS settings, and service count before startup.

### Recovery

1. Confirm the configured port is in range and the host is non-empty.
2. Ensure a service is registered before calling `start()`.
3. Recheck TLS certificate and key paths if `tls_enabled` is set.
4. Validate `max_message_size_bytes` is within the expected bounds and the config file is available if overriding values from `config/networking/grpc.yaml`.

## Scenario 3: unauthorized or malformed WebSocket CDC stream

### Symptoms

- connection upgrade is rejected
- change stream returns unauthorized or not-found response
- stream never becomes active despite a valid WebSocket handshake

### Evidence

The validation logic in `src/api/ws_handler.cpp` enforces the valid `cdc:subscribe` scope and the expected endpoint paths.

### Recovery

1. Verify the client token is present and valid.
2. Confirm the endpoint path matches `/v2/changes` or `/v2/cdc/stream`.
3. Check `from_sequence` and `key_prefix` query values for invalid numerical or encoded input.
4. If the problem is a configuration mismatch, correct the auth provider or route mapping before retrying.

## Scenario 4: OTLP exporter backlog or drop pressure

### Symptoms

- telemetry latency spikes
- exporter logs show queue pressure or dropped spans
- requests remain healthy but telemetry is sparse or unavailable

### Evidence

`src/api/otlp_exporter.cpp` documents bounded exporter behavior and explicit drop warnings.

### Recovery

1. Check exporter queue depth and flush cadence.
2. Verify that the collector endpoint is reachable and not overloaded.
3. Reduce the transmission burst or split exporter workload if the environment is under sustained load.
4. Treat missing telemetry as an operational issue separate from the business request path unless the service health check confirms a correlated failure.

## Recovery and escalation guidance

- Prefer the smallest configuration change that restores a known-safe state.
- Keep incident records tied to a request ID or connection ID.
- Escalate to the runtime owner when an auth or authorization problem impacts a production user path.
- If the API module is implicated in a broader service regression, collect the relevant GraphQL/gRPC/WebSocket logs before changing any request path rules.
