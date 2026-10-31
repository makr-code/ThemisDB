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

# Exporters Incident Runbook

Exact model backend not verifiable from final document text alone.

## Scope

This runbook covers ThemisDB data export runs implemented in `src/exporters`.
It does not cover telemetry/metrics exporter backends. Begin with the failed
request, exporter name, collection, user, selected format, output destination,
and the returned `ExportStats` or typed exception. Preserve those details for
the incident record while redacting tokens and sensitive record contents.

## Policy denial

**Signals:** `PolicyDeniedException` / `ERR_EXPORT_POLICY_DENIED`,
policy-denial metric or `[EXPORT_DENIED]` log entry.

1. Stop retrying with unchanged credentials or policy.
2. Confirm the request's `collection_name`, `requesting_user`, attached policy
   engine, and tenant context at the caller boundary.
3. Inspect the denial reason and policy/audit record through the configured
   governance controls. Do not log or attach exported payloads to the incident.
4. Retry only after an authorized policy or request correction. A denial is
   fail-closed and not resumable.

## Incomplete output or write/size failure

**Signals:** `ExportIOException`, `SizeLimitException`,
`EXPORT_WRITE_FAILED`, `STREAM_INTERRUPTED`, or missing/partial artifact.

1. Stop downstream ingestion and mark the destination artifact incomplete.
2. Check destination availability, permissions, remaining capacity, configured
   `max_file_size_bytes`, compression, and any filesystem error context.
3. Preserve the partial artifact for diagnosis only if policy permits; do not
   promote it as a valid dataset.
4. For streaming, verify the cursor/checkpoint and last successfully
   acknowledged chunk. A checkpoint offset alone does not establish that the
   destination file contains a complete prefix.
5. For incremental export, confirm the watermark did not advance after the
   failed write. The implementation is intended to leave it unchanged on I/O
   failure.
6. After correcting the cause, write to a fresh or known-safe destination and
   validate the completed artifact before publishing it.

## Template, schema, or join failure

**Signals:** `SchemaValidationException`, `FormatException`,
`ConfigException`, `SCHEMA_MISMATCH`, missing-column or ambiguous-field errors.

1. Capture the exporter type, format, config, and sanitized error context.
2. Check mapped field names and required values; use
   `JSONLLLMExporter::validateTemplate()` on representative records before
   rerunning a template workload.
3. For Parquet/Arrow/Hugging Face, compare input values with configured schema
   hints or features and check whether schema inference used a representative
   batch.
4. For joins, verify `setRightCollection()` ran, key fields exist, output-name
   collisions are aliased, and the right-side memory limit is adequate.
5. Correct the schema/configuration and rerun into a separate destination.
   Do not relax field filtering or validation merely to make the export pass.

## Hugging Face Hub upload failure

**Signals:** `HubUploadResult.success == false`, non-success HTTP status, retry
warning, `exporter_hub_upload_failures_total`, or rate-limit counter.

1. Verify local dataset output first; isolate upload failure from dataset
   generation.
2. Check Hub endpoint reachability, repository configuration, token resolution
   and scope, configured timeout, and policy/audit context. Never print the
   bearer token.
3. For HTTP 401/403, resolve credentials or authorization before retrying.
   For HTTP 413, split the file or reduce shard size. For HTTP 429, honor the
   server's `Retry-After` guidance and configured retry policy.
4. After retry exhaustion, verify whether any remote files were already
   uploaded before starting a new attempt; the operation may have partially
   succeeded remotely.
5. Escalate persistent endpoint/service failures to the owning platform team
   with sanitized status, timestamps, retry outcome, and repository ID.

## Rollback and escalation

Keep the last known-good artifact and its matching source/watermark state until
the replacement is fully validated. If the new artifact is invalid, stop
consumers from using it and restore the known-good artifact; restore checkpoint
or watermark state only as a coordinated recovery so records are not silently
skipped. Escalate suspected data exposure immediately through the project
security incident process. For other incidents, include the typed error code,
exporter/config version, metrics snapshot, and exact recovery actions.

See [OPERATIONS.md](./OPERATIONS.md), [TRUST.md](./TRUST.md), and
[`include/exporters/exporters_api_contract.h`](../../include/exporters/exporters_api_contract.h).
