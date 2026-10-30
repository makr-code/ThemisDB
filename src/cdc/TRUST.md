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
Source level: "level1"
SOT domain: "security"
Canonical source references: "src/cdc/SECURITY.md; src/cdc/PRODUCTION_REQUIREMENTS.md; include/cdc/cdc_delivery_contract.h; include/cdc/icdc_transport.h; src/cdc/cdc_ws_handler.cpp; tests/cdc/test_cdc_subscription_auth.cpp"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC trust boundaries

This page identifies CDC-specific trust boundaries from its public contracts
and implementation. The root
[`SECURITY.md`](../../SECURITY.md) remains the project security reporting
authority; this page does not claim that CDC provides end-to-end authorization.

## Boundaries and controls

| Boundary | Source-backed control | Residual responsibility |
|---|---|---|
| Application/service caller to CDC admin or subscription surface | The server subscription-auth tests exercise missing/invalid credentials and CDC read/admin scopes. | Deploy and configure the owning server's authorization policy; the C++ CDC transport interface itself has no auth contract. |
| Event producer to persisted changefeed | `recordEvent` assigns sequence/timestamp and persists the serialized event; key/value limits are defined by the delivery contract. | Callers must validate event content and avoid placing credentials or unnecessary sensitive data in event values/metadata. |
| CDC delivery to consumer | At-least-once acknowledgement, replay and failure-class rules are explicit in `cdc_delivery_contract.h`. | Consumers must tolerate duplicates and protect their own processing and acknowledgement paths. |
| CDC to external transport | `ICDCTransport` exposes start/stop/publish; failure classes and backpressure policy are in the contract. Kafka has compile-time optional support. | Operators must secure transport credentials/endpoints and verify the actual build/runtime configuration. |
| Retention, purge and redaction | Admin APIs expose explicit operations; redaction marks/scrubs matching event values and tests check the audit-related behavior. | Authorization, approval, data lifecycle policy and audit-storage configuration remain deployment concerns. |
| Logs and diagnostics | CDC returns structured diagnostics and error context. | Protect operational output: keys, event metadata, identifiers, paths and failure details may be sensitive. Redact before sharing. |

## Fail-closed expectations

`isCdcFailClosedClass` returns true for backend unreachable, authentication
failure, invalid payload, required backpressure, and internal error. The
contract requires delivery state not to advance until acknowledgement and
requires explicit diagnostics for failure paths. A transient failure is not
automatically safe to ignore: any fail-open behavior needs the documented retry
policy and operator opt-in specified by the contract.

## Review checklist

- Verify server authorization and tenant scoping at the exposed API boundary;
  do not infer them from `ICDCTransport` or `CDCAdmin` method presence.
- Validate resource and retention controls for the deployment. Header defaults
  do not prove that the configured production policy is safe.
- Check event payload/metadata and exported diagnostic data for sensitive
  information.
- Keep adapter credentials under the hosting service's secret-management
  policy; do not place secrets in this guide, logs, or test fixtures.
- Treat roadmap hardening as open until source/test evidence supports a
  stronger claim.

## Source references

- [CDC security summary](./SECURITY.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- [Normative failure and delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [Transport interface](../../include/cdc/icdc_transport.h)
- [WebSocket handler implementation](./cdc_ws_handler.cpp)
- [Subscription authorization tests](../../tests/cdc/test_cdc_subscription_auth.cpp)
- [Project security policy](../../SECURITY.md)

**Exact model backend not verifiable from final document text alone.**
