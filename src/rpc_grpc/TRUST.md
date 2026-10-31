---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "security"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Trust Model

## Trust boundaries

The plugin terminates gRPC transport for service implementations registered
by its host. The host supplies configuration, service objects, certificate
paths, access-log sinks, and any application-level identity/authorization
policy. This module does not define the business authorization rules for
registered services.

| Boundary | Trust assumption and control |
|---|---|
| Client to main listener | With `tls_enabled=true`, gRPC SSL credentials are built from configured certificate, key, and CA material. `auth_required=true` requires a client certificate; with it false, the source configures server-side TLS without requesting one. |
| Client to main listener with TLS disabled | The implementation selects `InsecureServerCredentials()` and logs that traffic is unencrypted. This is an explicit insecure mode, not a secure fallback. |
| Client to optional admin listener | A valid `extra_config["admin_port"]` creates a second listener on the configured host using insecure credentials. The source provides no listener-level authentication for it. |
| Host to registered service | `registerService(void*)` casts to `grpc::Service*`; the caller must provide a valid object and keep it alive until server shutdown. |
| Plugin to key material | The process reads PEM files by path. Keep the private key in a protected file accessible only to the service identity; never place key bytes in logs or support tickets. |
| Access-log sink | The host owns the callback and its destination. Method names, status, duration, and optional client common name can be sensitive operational metadata. |

## Authentication and authorization limits

`auth_required` is passed to the gRPC TLS credential builder as the
client-certificate requirement. It does not implement user authentication,
token validation, per-method authorization, rate limiting, or an authentication
interceptor. Enforce those controls in the registered service or a separately
verified trusted proxy.

The generic `RPCServerConfig` declares rate-limit and connection-limit fields,
but this implementation does not apply them. The plugin capability flag
`thread_safe` is not a replacement for serializing lifecycle and service
registration operations.

## Credential lifecycle and exposure

- TLS-enabled initialization requires readable certificate, key, and CA
  files. File-read or credential-construction failures reject startup rather
  than changing the main listener to insecure transport.
- TLS-disabled configuration remains unencrypted and should be limited to
  controlled test or explicitly isolated environments.
- The admin listener remains insecure even when the main listener uses TLS.
  Do not bind it to a reachable interface unless network controls prevent
  untrusted access.
- `reloadTls` stops and drains the current listener, then starts a replacement
  with the new credentials. This creates downtime. If the replacement fails,
  the old configuration is restarted when possible; if rollback also fails,
  the listener remains stopped. Verify the served certificate from a client
  before retiring the previous credential files.
- Error messages can include file paths. Keep path names free of embedded
  secrets and restrict log access.

## Observability and data handling

`recordRPC` emits an access record only when a sink is configured. The record
contains a timestamp, method, status code, duration, and client common name
(empty when invoked through `recordRPC`). Sanitize and restrict retention of
the sink's output according to the host's logging policy. Metric method labels
can have high cardinality; avoid unbounded user-supplied method values.

The optional admin port does not expose an HTTP endpoint in this module.
`getMetricsText()` is an in-process function, and `isServiceHealthy()` reads an
in-memory map (unknown names default to healthy). Do not expose these facts as
remote health or metrics guarantees.

## Incident response

For suspected exposure, isolate or remove the insecure admin listener and
restrict ingress to the main listener while preserving encrypted transport.
For key compromise, follow the deployment's secret-rotation procedure, stage
replacement material with restrictive permissions, use the listener-restart
reload or a controlled host restart, and verify the certificate from a client
before deleting prior material. Preserve sanitized logs and configuration
metadata; do not attach private keys.

Report vulnerabilities through the project [security policy](../../SECURITY.md).
Operational steps are in [`RUNBOOK.md`](RUNBOOK.md); interface semantics are
in [`API.md`](API.md) and production constraints in
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).
