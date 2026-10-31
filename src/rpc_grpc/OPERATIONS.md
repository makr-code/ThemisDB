---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Operations

## Audience and operational boundary

This page is for operators and service owners deploying the runtime-loadable
gRPC plugin. The plugin runs inside its host process; this module does not
provide a separate service manager unit, CLI, HTTP health endpoint, or metrics
scrape endpoint. Apply process start/stop/restart actions through the actual
ThemisDB deployment controller.

## Readiness and exposure checks

Before rollout:

- Confirm the plugin was built with `THEMIS_PLUGIN_RPC_GRPC` and loaded from the
  host's plugin path.
- Confirm `RPCServerConfig` was supplied to the server instance. The plugin
  wrapper's JSON initializer currently ignores its argument.
- Set an explicit listener host and port. Interface defaults are
  `0.0.0.0:0`; port zero requests an OS-selected port, but the plugin does not
  report the selected port.
- For encrypted transport, set TLS enabled and verify access to all three PEM
  files. Set `auth_required` according to whether the TLS listener must require
  a client certificate. Application authorization remains the service's
  responsibility.
- Treat `extra_config["admin_port"]` as an insecure listener. It must not be
  externally reachable.
- Do not assume the interface's connection, thread-pool, timeout, rate-limit,
  or compression fields are enforced; current source does not consume them.

## Monitoring surfaces and limitations

| Surface | Available behavior | Operational interpretation |
|---|---|---|
| Lifecycle diagnostics | `[RPC-E8300]` through `[RPC-E8306]`, plus informational/warning output | Collect host process logs; exact destination depends on host logging configuration. |
| Method counters | `recordRPC` and `getMetricsText` | In-process snapshots; calls must be wired by the service/host. The source does not install a request interceptor or network endpoint. |
| Access records | Optional callback set with `setAccessLogSink` | Host supplies the sink. `recordRPC` passes an empty client common-name value. |
| Health state | `setServiceHealth` and `isServiceHealthy` | In-memory only; an untracked name returns true. This is not a remote gRPC health service. |
| Circuit breaker | `CircuitBreaker::stats()` in the separate helper | Only useful when a consumer explicitly owns and calls the helper; it is not connected automatically to `GRPCServer`. |

Metrics include per-method request/error counters and cumulative latency in
milliseconds, plus the active-connections gauge field. They do not form
latency histograms or percentiles in `getMetricsText()`. Do not map the
`grpc_server_active_connections` value to a production capacity signal without
verifying that the host updates `RPCServerStats::active_connections`.

## Incident handling

Use [`RUNBOOK.md`](RUNBOOK.md) for failure triage. In brief:

1. Preserve the current binary, configuration, sanitized logs, and client-side
   failure evidence.
2. Determine whether the problem is build/load, initialization, listener
   startup, certificate setup, service registration, or request handling.
3. Avoid weakening TLS or opening the admin listener as a diagnostic shortcut.
4. Restore the last known-good binary and configuration as a unit if rollback
   is needed; recreate the server through the host.
5. Escalate unresolved transport, credential, or shutdown failures with the
   evidence listed in the runbook.

## Recovery constraints

- TLS reload is a disruptive listener restart. Validation errors leave the
  listener unchanged; failure to start with new files triggers an attempt to
  restore the old configuration. If that rollback fails, the server remains
  stopped. Verify the presented certificate from a client before retiring old
  credential files.
- `stop()` has no module-configurable shutdown deadline. A forced process stop
  belongs to the host's approved deployment procedure.
- The optional admin listener uses insecure gRPC credentials. Isolate it with
  network controls or omit it.
- Service objects are externally owned. Stop the server before destroying
  registered services.

See [`TRUST.md`](TRUST.md), [`API.md`](API.md), and
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) before approving a
production exposure or configuration change.
