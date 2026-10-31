---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Exporters Operations

Exact model backend not verifiable from final document text alone.

## Operational boundary

The module produces export files/streams and provides a separate
`HuggingFaceHubClient` for remote dataset upload. It does not own source
collection lifecycle, scheduling, downstream ingestion, Hub availability, or
the deployment's metrics backend. Operators should monitor the application
that invokes the exporter and correlate its request identity with the
exporter's own result and counters.

## Before enabling a production export

- Set explicit output destination, include/exclude fields, filters, and
  resource limits. Shared zero-valued size and throughput limits mean
  unlimited.
- Attach the policy engine and audit logger required by the deployment.
  Populate collection and requesting-user context; if tenant isolation is
  required, provide and enforce tenant context.
- Configure encryption with a valid KEK identifier and `KeyProvider` for
  sensitive output. Encryption is optional in `ExportOptions` and disabled
  unless configured.
- For streaming/incremental exports, provision writable checkpoint/watermark
  paths and define how artifact and state backups are coordinated.
- For joins, estimate right-side memory needs and retain the configured budget.
- For Hub upload, choose repository visibility intentionally, provision
  credentials through the approved key/token mechanism, and set retry and
  timeout values to the service's operating limits.
- Confirm required format dependencies are enabled in the build. Arrow and
  Parquet availability can change the active code path.

## Monitoring

Collect the `ExportStats` returned by each run: total, exported, failed and
skipped entities, bytes written, duration, error details, and ETA where
available. Where the application exposes `ExporterMetrics`, inspect its
structured/string reports for error totals, throughput/latency summaries,
checkpoint and delta counters, PII/encryption activity, policy denials,
rate-limit hits, and Hub upload failures.

Use trend changes as triage signals rather than as standalone proof of
successful delivery. In particular, successful local serialization does not
prove that a remote upload completed, and successful upload does not prove that
the consumer accepts the schema. `ExporterMetrics` is an in-process module
surface; this documentation does not assert a particular Prometheus, OTLP, or
admin endpoint.

## Recovery, rollback, and change control

On failure, follow [RUNBOOK.md](./RUNBOOK.md). Preserve the previous
known-good artifact and matching incremental state until validation of the
replacement is complete. Validate file structure, record counts, required
fields, and consumer compatibility before switching downstream consumers.

After rollback, reconcile the source watermark/checkpoint with the artifact
that consumers will actually read; mismatched state can skip or duplicate
records. Record the exporter config, artifact identity/checksum, state
location, error code, and disposition in the deployment's change/incident
record. Do not delete partial outputs or state files until the recovery plan
has confirmed they are no longer needed.

## Escalation

Escalate policy denials to the data owner/governance team, output or resource
failures to the service/storage operator, schema incompatibility to the
exporter and consumer owners, Hub connectivity/HTTP failures to the external
integration owner, and suspected sensitive-data exposure to the security
incident process. Include sanitized diagnostics; do not include exported
records, access tokens, or raw key material.
