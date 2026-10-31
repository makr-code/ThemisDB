---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Network Module Runbook

## Scope and safety

This runbook covers diagnosis of the `src/network/` listener, frame,
authentication, rate-limit, and routing paths. It assumes an operator has
access to the host and the deployment’s normal process supervisor. The module
does not define a universal `themisdb-admin` command or a universal metric
exporter; use only commands and dashboards actually installed in the
deployment.

Do not disable authentication, frame limits, or TLS checks as a recovery
shortcut. Do not print or attach auth tokens, private keys, or full credential
configuration to incident reports. If compromise is suspected, isolate the
listener and follow the project [security reporting process](../../SECURITY.md).

## Triage order

1. Identify which listener and protocol are affected from the deployment
   configuration. The native wire server defaults to TCP 8766; `UDPServer`
   defaults to UDP 8768; `QuicTransport` defaults to UDP 8770. These defaults
   can be overridden and optional transports may not be compiled/enabled.
2. Check process health and listener binding on the host. For Linux, `ss -lntup`
   shows TCP and UDP sockets; compare the result with the configured host/port.
3. Inspect the application log around the incident time for the component
   prefix (`[WireProtocol]`, `[UDPServer]`, `[QuicTransport]`, or the active
   transport). Correlate with server/transport `getStats()` snapshots where
   the owning service exposes them.
4. Determine if failures are isolated to one client, all clients on one
   transport, or shared dependencies. Avoid changing limits until the source
   of saturation is known.
5. After a corrective action, confirm the listener is bound, the relevant
   error/rejection counters stop increasing abnormally, and a representative
   client request succeeds.

## Symptoms and response

### Listener does not start or accept connections

- Verify bind address, port ownership, runtime feature flags, and process
  startup logs.
- For `WireProtocolServer`, confirm transport-security preflight passed and
  the dependencies checked by `start()` are configured (including a query
  engine and spatial index or geo-query callback).
- For QUIC, confirm the deployment has enabled `THEMIS_ENABLE_HTTP3`, that
  the configured certificate/key can be read, and that UDP traffic is allowed
  at the host/network boundary. QUIC startup logs distinguish TLS-context
  creation from successful bind/listen.
- Correct configuration or certificate problems through the normal
  deployment process. Restart the owning service only after validating the
  corrected configuration.

### Invalid or oversized frames increase

- Check the peer and protocol version in the application’s existing logs.
- A wire frame is rejected when it fails validation or exceeds the configured
  frame bound; do not raise the bound solely to suppress errors.
- If failures begin after a client rollout, compare protocol versions and
  payload construction before rolling back or blocking an address.
- Retain timestamped logs and the affected listener/client version for
  security triage.

### Authentication failures or unexpected unauthenticated access

- Confirm `require_auth`, the configured `SCRAM-SHA-256` mechanism, and
  secret provisioning in the deployment’s secret manager.
- The wire-server startup path rejects invalid required-auth configuration.
  Revoke and rotate a suspected exposed token rather than reusing it.
- If a listener is reachable from an untrusted network without its intended
  auth policy, remove it from traffic until the configuration is corrected.

### Rate limiting, connection limit, or backpressure

- Compare the affected peer’s connection/request rate with configured
  `max_connections`, `max_connections_per_ip`,
  `max_requests_per_second`, and `max_requests_per_minute`.
- Inspect active connection and rejection/request counters through the
  owning server’s `getStats()`. Lowering a limit can worsen impact; change it
  only after checking worker capacity and downstream saturation.
- Preserve rate-limit and backpressure rejection behavior while investigating;
  do not bypass admission controls to restore apparent throughput.

### Circuit breaker or routing failures

- Inspect the breaker state, rejection counters, and configured thresholds;
  an OPEN state indicates the configured failure policy is preventing calls.
- For topology selection, inspect the healthy-shard input and region/zone
  metadata. An empty selected endpoint means no eligible healthy shard was
  available under the configured policy; the router does not establish a
  connection or perform health checks.
- Restore the underlying dependency/topology first. Resetting a breaker or
  forcing cross-region routing without confirming safety can expose requests
  to the same failing dependency or unexpected locality.

## Recovery and rollback

Use the process supervisor or orchestrator that owns the server to restart a
failed service after its configuration is corrected. For in-process lifecycle
management, call the owning component’s `stop()` and wait for it to complete
before `start()`; concurrent start/stop is not supported by the UDP and QUIC
transport contracts. Roll back a release when a reproducible protocol
regression is tied to that release and configuration/certificate issues have
been excluded.

Preserve application logs, listener configuration with secrets redacted,
relevant statistics, client/protocol versions, and the timeline. Escalate
suspected credential or key exposure to security response; escalate repeatable
wire incompatibility to the network/API maintainers.

See [`OPERATIONS.md`](OPERATIONS.md), [`TRUST.md`](TRUST.md), and the
source-defined interfaces in [`API.md`](API.md).
