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

# RPC gRPC Runbook

## Scope and safety

This runbook covers diagnosis of the `src/rpc_grpc` runtime plugin, not the
separate built-in API gRPC server. Use the deployment's own supervisor to
inspect or restart the host process; this module does not define a
`themis-grpc` service, CLI, or HTTP admin API.

Before changing credentials or listeners, record the host revision,
configured listener addresses, recent `[RPC-E...]` / `[RPC-W...]` output, and
the last known-good configuration. Do not copy private key contents into
tickets or logs.

## Plugin absent or unavailable

1. Confirm the configured build enabled `THEMIS_PLUGIN_RPC_GRPC`.
2. Check CMake output for the gRPC package. The module build returns without
   defining its shared target when gRPC is not found.
3. Confirm the produced plugin file and manifest are in the host's configured
   plugin search path.
4. If the plugin loads but a server cannot be created, inspect host plugin
   load diagnostics; `createServer()` creates a `GRPCServer` instance and does
   not open a listener.

## Initialization or start failure

1. Check that `RPCServerConfig::host` is non-empty.
2. If TLS is enabled, confirm the process can read the configured certificate,
   private key, and CA files. The implementation rejects missing or unreadable
   files during initialization and reports TLS setup failures with
   `[RPC-E8302]`.
3. Check `[RPC-E8300]` for an already-running start or late registration,
   `[RPC-E8301]` for a null service, and `[RPC-E8306]` for server-builder or
   internal failures.
4. Verify the configured host/port with platform socket tools and deployment
   logs. The module does not expose a port-selection result for port `0`, so
   do not assume the selected ephemeral port from `getAddress()`.
5. If `tls_enabled` is false, the server intentionally uses insecure
   credentials and emits `[RPC-W/GAP-016]`; stop and correct the configuration
   if encrypted transport was required.

Do not respond to a TLS error by disabling TLS. Invalid TLS credentials do not
fall back to the main listener's insecure credentials.

## Admin listener exposure

When `extra_config["admin_port"]` is a valid integer in `1..65535`, the source
adds a second gRPC listener using `InsecureServerCredentials()` and logs
`[RPC-W/GAP-016]`. It is not a module-provided HTTP `/health` or `/metrics`
endpoint. If that listener is reachable from an untrusted network, remove the
admin-port configuration or restrict ingress at the host/network boundary,
then verify the listener is no longer reachable.

## TLS credential reload

`reloadTls(cert, key, ca)` returns false if the server is stopped, TLS is
disabled, a path is empty, a file cannot be loaded, or credential construction
fails. Those validation failures leave the active listener untouched. After
validation, reload drains and stops the listener before starting it with the
new files, so plan for temporary unavailability and in-flight RPC draining.

If the new listener fails to start, the method attempts to start the old
configuration again and still returns false. If rollback fails, the server is
left stopped; inspect `[RPC-E8302]`/`[RPC-E8306]` diagnostics and recover through
the host's controlled start procedure. A true result means the server start
path succeeded with the new files, but operators should still verify the
presented certificate from a client before retiring the old files. Serialize
reload with other lifecycle and service-registration calls.

## Request, stream, and metrics symptoms

- `recordRPC` counters update only when that method is called. The module does
  not install an automatic metrics interceptor.
- `getMetricsText()` returns an in-process snapshot; it does not create an
  HTTP scrape route. `setAccessLogSink()` must be configured by the host for
  access records to be emitted.
- `BidiStreamAdapter::write()` is synchronous, does not propagate the
  underlying `Write()` boolean, and has no built-in deadline. For stalled
  streams, inspect the service's cancellation and stream-lifetime handling;
  do not infer a transport error from the adapter's `true` return alone.
- The module's health map is in-process; an unknown service name is reported
  healthy by `isServiceHealthy()`. Do not use that method as evidence that a
  remote service endpoint is healthy.

## Shutdown and rollback

`stop()` calls `grpc::Server::Shutdown()` and drains the idle completion queue
if present. The module does not expose a shutdown grace-period setting. Allow
the host's normal shutdown sequence to finish; if it does not, collect logs
and process diagnostics before applying the deployment's approved forced-stop
procedure.

For rollback after a failed reload, the method attempts to restart using its
previous configuration. If it reports that both the new start and rollback
failed, restore the last known-good plugin binary and configuration together,
then recreate the server through the host.

## Escalation

Escalate with sanitized host logs, revision/build options, gRPC/Protobuf
versions, listener configuration (without secrets), client-observed status,
and whether the failure reproduces after a controlled restart. Report security
exposure through the project [security policy](../../SECURITY.md).

Related pages: [`OPERATIONS.md`](OPERATIONS.md), [`TRUST.md`](TRUST.md),
[`OPERATOR_RUNBOOK.md`](OPERATOR_RUNBOOK.md), and the broader
[RPC/gRPC operational runbook](../../docs/operability/RUNBOOK_RPC_GRPC.md).
