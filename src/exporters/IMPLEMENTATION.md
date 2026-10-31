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

# Exporters Implementation Guide

Exact model backend not verifiable from final document text alone.

## Ownership and execution

Public types and caller-facing contracts live in `include/exporters/`;
implementations live in `src/exporters/`. The general call contract is
`IExporter::exportEntities(entities, options)`, returning `ExportStats`.
Implementations own format serialization and exporter-specific state, not
source query planning or downstream artifact ingestion.

At a high level, an exporter validates its options and authorization context,
selects/project records, serializes them with the selected format path, writes
the artifact, and reports counts, errors, byte totals, and duration. Exact
ordering of filtering and format-specific checks is implementation-specific;
policy enforcement must occur before opening an output or cursor when a
`PolicyEngine` is configured.

## Implementer invariants

- Reject unsupported formats and invalid configuration before output where
  the format contract requires it.
- Preserve format-specific null semantics and deterministic ordering defined
  by `include/exporters/exporters_api_contract.h`.
- Apply `include_fields`, `exclude_fields`, and configured predicates
  consistently in the exporter path being changed. Join and incremental
  variants have regressions specifically for filter parity.
- Respect explicit file, throughput, buffer, error-count, and per-exporter
  memory bounds. A zero value means unlimited for some shared limits; do not
  assume it is a safe production default.
- For streaming, preserve chunk order and advance a checkpoint only from the
  cursor state successfully processed by the implementation.
- For incremental export, do not advance the watermark after a partial scan or
  failed output. The current implementation intentionally leaves it unchanged
  on I/O failure.
- For joins, initialize right-side state with `setRightCollection()` before
  export and fail closed on missing setup or budget overflow.
- When both streaming encryption options are present, `encryption_config`
  takes precedence; do not encrypt the same output through both paths.

## Failure behavior

Use the typed exporter exceptions in `include/exporters/exporter_errors.h`
for configuration, schema, policy, output, and size-limit failures. The
canonical error taxonomy in `exporters_api_contract.h` distinguishes
unsupported format, write failure, stream interruption, schema mismatch,
quota exceeded, missing column, schema serialization failure, cancellation,
and internal error. Only `STREAM_INTERRUPTED` is marked resumable by
`isResumableError()`; do not make authorization denial or caller abort
resumable.

If a write fails after bytes have been emitted, expose the failure and ensure
the caller can identify/discard incomplete output. Do not report a complete
artifact or persist a later watermark for an incomplete run.

## Tests and change path

Add focused regressions alongside the owning exporter in `tests/exporters/`.
Cover valid serialization, empty inputs, null/missing fields, invalid
configuration, policy denial before output, partial writes, configured limits,
and checkpoint/watermark recovery where applicable. Use the existing
`tests/exporters/test_exporters_contract_hardening_focused.cpp` and
`test_exporters_phase23_hardening_focused.cpp` as shared contract references.
Run focused tests first, then the documentation and benchmark checks relevant
to changed behavior.

See [API.md](./API.md) for the public contracts,
[MODULE_GUIDE.md](./MODULE_GUIDE.md) for module navigation, and
[RUNBOOK.md](./RUNBOOK.md) for incident response.
