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

# Server Runbook

## Purpose

This runbook is for diagnosing, recovering, and escalating incorrect runtime behavior in the `server` module. It is grounded in the request-lifecycle, gateway, auth, and overload-control logic implemented in the module.

## Triage workflow

### 1. Classify the symptom

Start by identifying whether the client is failing due to:

- authentication (401/403),
- rate limiting (429),
- overload or queue pressure (503),
- timeout / deadline expiration (504),
- protocol/session disconnect,
- routing mismatch or versioned path issue.

### 2. Check the request path

Review the inbound request path and route normalization logic in `api_gateway.cpp`. Confirm whether the version prefix was stripped correctly and whether request dispatch still matches the expected target.

### 3. Check auth and policy gates

Inspect whether `AuthMiddleware` was configured correctly, whether the token or credential is valid, and whether the scope mapping matches the endpoint.

### 4. Check quota and pressure state

If the client sees 429 or 503, review rate-limit configuration and the load-shedder metrics. Confirm whether CPU, memory, or queue depth crossed the allowed thresholds.

## Recovery playbooks

### Authentication failure

Actions:

1. verify the caller's token or credential mechanics,
2. confirm the selected auth provider is enabled and configured,
3. confirm issuer/audience/scope expectations match endpoint requirements,
4. check for expired or revoked credentials.

### Rate-limit exhaustion

Actions:

1. identify the client key and the affected path prefix,
2. inspect the endpoint override configuration,
3. confirm whether the limiter is using the default or an override bucket,
4. increase quota only if the operational load is legitimate and the policy change is intentional.

### Overload or saturation

Actions:

1. check CPU and memory pressures,
2. inspect queue depth and load-shedder thresholds,
3. confirm whether the service is rejecting low-priority traffic intentionally,
4. reduce request concurrency or shift load away from the overloaded node.

### Session or protocol failures

Actions:

1. check the session registry and transport lifecycle,
2. confirm that the client connection is still valid,
3. compare logs from the protocol-specific session file,
4. determine whether the disconnect is local, upstream, or network-driven.

## Rollback and exit conditions

Rollback is the correct move when a change introduces one of the following:

- auth bypass or broadening of endpoint access,
- route normalization regressions,
- broader than intended rate-limit or overload behavior,
- session disconnects that impact unrelated traffic,
- abnormal growth in 503/429 responses under stable load.

When rollback is needed, revert the smallest server-level change and validate the route policy again before redeploying.

## Escalation criteria

Escalate to the owning runtime or platform team when:

- auth failures affect multiple clients across unrelated endpoints,
- 503 rates remain elevated after load shedding is enabled,
- protocol sessions are failing in a way that causes cross-session disruption,
- changes in request normalization or routing break API compatibility.

## Summary

The server runbook is intentionally decisive: classify the symptom, verify the gating layer, and only then decide whether the proper response is config correction, traffic management, or a controlled rollback.
