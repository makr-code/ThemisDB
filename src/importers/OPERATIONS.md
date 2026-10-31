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
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/PRODUCTION_REQUIREMENTS.md; src/importers/PERFORMANCE_EXPECTATIONS.md"
Related_Reference: "makr-code/ThemisDB#7124"
---

> Exact model backend not verifiable from final document text alone.

# Importers Operations Guide

This page is for service owners planning, observing, and reviewing importer
jobs. Connector-specific deployment and capacity settings belong to the
connector configuration and source-system runbooks; shared option names are
defined in
[`importer_interface.h`](../../include/importers/importer_interface.h).

## Before scheduling a job

- Verify connector support and build/runtime flags. `ARROW_ENABLED` is required
  for Parquet; Kafka, S3, and CDC paths have separate capability gates.
- Estimate input row and byte sizes, batch memory, destination write capacity,
  and source-side connection limits. `ImportOptions::batch_size` defaults to
  1,000. The separate versioned contract header declares a 10,000-row default
  contract batch constant, a configurable maximum, and a 10 GiB default file
  size; these values are not interchangeable with per-connector runtime
  defaults.
- Select explicit conflict, schema-validation, row-error, timeout, and
  cancellation behavior. Validate mappings and source access before a
  production write.
- Allocate restricted storage for checkpoint, quarantine, and delta-hash
  files. These files can contain operational state or imported data.
- Ensure the host supplies an authorization callback when job-level import
  authorization is required. The callback is optional in the shared API.

## Observability and job health

`ImportStats` exposes record counts, conflict counters, duration, warnings,
and structured errors. `metrics_callback` can emit connector-defined row,
error, table, or duration measurements; `tracing_callback` can record major
operation spans. Configure the callbacks at the calling service and avoid
high-cardinality labels containing record values, source credentials, or
personal data.

Monitor at minimum:

- imported, failed, skipped, and quarantined records together;
- duration and throughput against the source volume and connector baseline;
- connector availability, timeout and validation error rates;
- checkpoint age/progress and quarantine/dead-letter growth;
- target commit/visibility and downstream replication behavior.

The contract declares a per-node maximum of 32 concurrent import jobs and a
five-second cross-node replication-lag SLO reference. Confirm that the runtime
coordinator enforces the applicable limit and that the deployment's actual
replication behavior meets its own SLO; constants are not a substitute for
observed capacity evidence.

## Routine operation and change control

1. Run preflight validation and, where appropriate, a dry run.
2. Record source identity, options, build/connector version, checkpoint
   location, and the expected target scope.
3. Start the job with explicit resource and failure policy; retain progress and
   structured error output.
4. Confirm counters reconcile with the intended input, resolve skipped,
   failed, or quarantined records, and validate target data before downstream
   consumers proceed.
5. For a connector or schema upgrade, test representative old/new source
   schemas and rollback/resume behavior before rollout.

Do not treat Wikipedia's portable JSON database export or the HuggingFace
plugin's in-memory state as durable production database persistence. Their
current behavior and limitations are documented in
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) and
[`EMBEDDING.md`](./EMBEDDING.md).

## Incident and recovery references

Use the [runbook](./RUNBOOK.md) for code-specific triage and safe recovery.
Use [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for module
production constraints and [`SECURITY.md`](./SECURITY.md) for existing module
security scope.
