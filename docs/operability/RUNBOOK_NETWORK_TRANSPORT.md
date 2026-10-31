# RUNBOOK: Network Transport — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-10-31
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** unknown

**Audience:** Database operators, SREs, and network/transport maintainers
**Purpose:** Diagnose and recover from listener, transport, frame, authentication, and overload incidents using the configured deployment environment and source-backed component behavior.

---

## Scope and operational boundaries

The network module contains separate transports; a port being present in source does not mean that a deployment enables or exposes it. Verify the running build, configuration, bind address, and network policy before drawing conclusions.

| Transport | Source default | Availability and boundary |
|---|---:|---|
| TCP binary wire protocol | TCP 8766 | `WireProtocolServer`; actual host and port come from its configuration. |
| WebSocket upgrade | TCP 8766 | Shares the wire listener; requires the WebSocket build option and `enable_websocket_upgrade`. It is not a separate default listener. |
| UDP ingestion | UDP 8768 | `UDPServer`; datagrams are best-effort and do not provide TCP-style delivery or ordering guarantees. |
| QUIC binary wire protocol | UDP 8770 | `QuicTransport`; compiled with `THEMIS_ENABLE_HTTP3`, uses TLS 1.3, and takes certificate/key paths from its `Config`. |
| Native gRPC binary wire protocol | TCP 8771 | `GrpcTransport`; compiled with `THEMIS_ENABLE_GRPC`. It transports wire frames; it is not the gRPC API server. |

The separate gRPC API server uses port 50051; it is outside this module transport's listener configuration. The root roadmap classifies the network module as `PRODUCTION_CANDIDATE`, not production-ready ([ROADMAP.md](../../ROADMAP.md)).

The network module does not define a universal `themisdb-admin` command, log-file location, Prometheus exporter, or operator-facing metrics query tool. The observability registry has an optional sink; without an installed sink, span events are discarded. Component statistics are C++ APIs and are operationally useful only when the embedding service exposes them.

## Before intervening

1. Record the incident time window, affected clients, deployment/version, configured transport(s), expected listener address/port, and recent configuration or certificate changes.
2. Use the deployment's actual service manager and configured log destination. Do not assume a fixed log path or log-tag scheme.
3. Check the host's listening sockets and owning process with the host's available socket-inspection tools. On Linux, `ss -lntup` is one option; permissions may limit process details.
4. Compare the observed socket with the effective deployment configuration and enabled build features. Do not expose an unconfigured transport by opening firewall ports.
5. Preserve relevant logs and configuration revisions. Never copy authentication tokens, private keys, or full credential-bearing configuration into incident notes.

## Listener unavailable or startup failure

### Diagnose

- Check whether the expected process is running and whether the configured address/port is bound by that process.
- If another process owns the port, identify it through the deployment/host owner before changing either service's port.
- For wire-server startup failures, inspect the service's captured standard error for the `[WireProtocol]` startup/configuration messages. The server validates configuration and required runtime dependencies before listening; a failed check prevents startup.
- Check that the enabled transport's build option and runtime configuration agree. In particular, WebSocket upgrade is opt-in on the wire listener, while QUIC and native gRPC are build-guarded.
- For TLS failures, verify that the configured certificate and key files exist, are readable by the service identity, match, and are within their validity period. For QUIC, use the paths supplied through `QuicTransport::Config`; do not assume environment-variable names.

For a PEM certificate, its validity dates can be inspected with:

```bash
openssl x509 -in /path/to/configured-certificate.pem -noout -dates
```

### Recover

Use the deployment's normal configuration rollout or service restart procedure; this module does not provide a universal listener-restart admin command. If correcting a port, certificate, or build/config mismatch, roll out the approved configuration and restart through the owning service manager. Keep the prior known-good deployment/configuration available for rollback.

Confirm that the expected process owns the configured socket, the service remains healthy, and a client using the affected protocol can connect. Check other enabled transports separately; recovery of one listener does not prove that the others are available.

## Invalid frames or client connection failures

The wire server validates its binary protocol and returns errors for rejected operations. Do not rely on undocumented `[FRAME:*]` log tags or a built-in frame-error dashboard.

1. Confirm that the client is connecting to the correct listener and speaking the binary wire protocol rather than HTTP or another protocol.
2. Compare the client and server protocol version/configuration and note whether the failures began after a client or server rollout.
3. Where the embedding service exposes `WireProtocolServer::getStats()`, compare `total_requests`, `total_errors`, `rejected_connections`, and byte counters over the incident window. These are component counters, not a guaranteed exported metric set.
4. If one client or source is implicated, use the deployment's existing network controls and client-owner process. Do not invent a server-side peer-blocking or session-drop command.
5. If the problem follows a rollout, roll back through the normal deployment system when the previous version is known good; otherwise escalate with protocol evidence and captured errors.

## Authentication failures

Authentication behavior is transport-specific. `WireProtocolServer::Config` defaults `require_auth` to true; its startup validation requires a valid configured authentication secret when authentication is required. The wire protocol permits an unauthenticated `HELLO` as its first message, but authenticated operations enforce authentication. Do not generalize this contract to UDP, QUIC, gRPC, WebSocket, or the separate API server without checking that component's configuration and handler.

- Check the effective authentication mode, secret provisioning/rotation state, and client authentication flow without printing the secret.
- Check service startup output for a wire-server authentication-configuration validation failure.
- If `WireProtocolServer::getStats()` is exposed by the embedding service, compare its `auth_failures` count with the incident window and baseline.
- If a secret may have been exposed, follow the credential-rotation and security-incident procedures for the owning service; do not weaken authentication to restore availability.

## Rate limiting and overload

The wire server's configuration includes per-IP limits (`max_requests_per_second` and `max_requests_per_minute`), connection limits, frame-size limits, and request timeouts. `UDPServer` applies its own source rate limiting and logs rate-limited or malformed datagrams with an anonymized source. These controls are not evidence of a shared global limiter across all transports.

1. Determine whether the impact is isolated to one source, one transport, or the whole service. Correlate client errors with process resource use and host-level socket/network counters available in the deployment.
2. For the wire listener, compare `WireProtocolServer::getStats()` counters if exposed; for UDP, use the owning service's available `UDPServer` statistics and logs. These are implementation APIs, not a promised dashboard or metric name.
3. Check configured per-connection/per-IP limits and capacity before changing them. Do not raise limits or queue sizes reflexively; that can increase resource pressure.
4. Apply mitigation through existing ingress controls or an approved configuration rollout. Scale or shed traffic according to the service's deployment runbook.
5. Verify client errors and resource pressure return to the deployment's established baseline. If no baseline or component statistics are available, record that limitation and escalate rather than reporting an unverified recovery.

## Downstream failures and circuit breakers

`AdaptiveCircuitBreaker` is a component, not a global control plane for every outbound network path. The module does not establish a universal breaker dashboard, log-tag format, admin reset command, or automatic fallback contract.

- Identify the specific caller/component that owns the breaker and downstream request; inspect its configured endpoint, dependency health, and available component-level breaker state/statistics.
- Verify downstream health independently using the owning service's supported health check. A local network probe alone does not establish that the dependency's application path is healthy.
- Do not force a breaker closed or bypass it to clear an alert. Allow the component's configured recovery behavior or follow the owning component's documented recovery procedure.
- If no operator-facing state or recovery interface exists, capture the caller, downstream, time window, and available errors and escalate to that component's owner.

## Graceful shutdown, rollback, and escalation

`WireProtocolServer::stop()` closes connections, waits for in-flight requests, and shuts down its thread pool. This is not a documented `DRAINING` protocol state, and the module does not provide a universal live drain or listener restart command.

1. If the deployment supports load-balancer removal, use its established procedure before stopping an instance.
2. Stop or restart the owning service through its service manager; observe the configured shutdown timeout and preserve logs if it does not exit cleanly.
3. Confirm another healthy instance or an approved maintenance window before removing capacity. Validate each enabled listener after restart.
4. Roll back using the deployment system's last known-good release/configuration if the change caused the incident. Do not substitute guessed runtime configuration commands.
5. Escalate unresolved startup, protocol, authentication, or downstream issues to the owning network/service team with a sanitized configuration diff, version/build options, listener evidence, timestamps, and relevant logs.

## Evidence and observability

Keep a concise incident record containing the deployment/version, enabled transport and effective bind address, client-side symptom, relevant sanitized log excerpts, socket/process observations, configuration changes, and before/after results.

`NetworkObservabilityRegistry` supports a caller-registered sink and in-process network counters. A null sink discards span events; the network module itself does not guarantee that spans or counters are exported, retained, or queryable. Verify the embedding application's actual sink/exporter and query interface before using telemetry in an incident procedure.

The D1 span names and `otel-query` examples previously listed for this runbook are not an operational interface implemented by the network module and are intentionally not presented as runnable diagnostics here.

---

**Runbook Version:** 1.1
**Owner:** Network/Transport Team, Operations Team
**Next Review:** 2027-03-01
