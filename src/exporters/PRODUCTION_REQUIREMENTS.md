---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/exporters/exporter_interface.h; include/exporters/export_encryption.h; src/exporters/*.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporters Production Requirements

Exact model backend not verifiable from final document text alone.

## Purpose and boundary

This page gives deployment recommendations for applications that use the
exporters module. It is not a claim that the library enables encryption,
authorizes every request, validates all deployment configuration at startup,
or selects a production mode automatically. The application and deployment
remain responsible for enforcing their own data-classification and release
requirements.

## Deployment checklist

- Classify the data and configure the required field projection, supported
  predicate filters, and output destination before starting an export.
- If policy authorization is required, provide a `PolicyEngine` in
  `ExportOptions`; exporter implementations invoke the policy guard when that
  pointer is configured. A missing policy engine does not grant authorization.
- If output encryption is required, configure `ExportOptions::encryption` or
  `encryption_config` with a valid KEK identifier and `KeyProvider`. Encryption
  is optional and disabled by default.
- Provide audit context where required by the deployment. The API's audit
  logger is a caller-provided control, not a standalone audit service.
- Set explicit file, throughput, buffer, join-memory, Hub retry, and timeout
  values appropriate to the workload. Several zero-valued limits mean
  unlimited.
- For cursor streaming or incremental export, provision writable checkpoint or
  watermark paths when persisted progress is needed and coordinate state with
  artifact publication.
- Review Hub repository identity and visibility before upload. Hub transfer is
  a separate remote egress operation from local dataset generation.

## Configuration and failure notes

`ExportOptions` exposes optional policy, audit, tenant-context, and encryption
fields. Exporters that implement policy checks call `enforceExportPolicy()`
before their output/cursor activity when a policy engine is present. The Hub
client has its own optional policy and audit fields.

The module exposes typed exporter errors and upload results, but this does not
replace application monitoring, deployment-level startup validation, or
security review. No uniform `THEMIS_PRODUCTION_MODE` or
`THEMIS_ENVIRONMENT` switch is defined by the exporter headers or implementation.

## Source references

- Shared options and optional controls:
  `include/exporters/exporter_interface.h`.
- Encryption interface:
  `include/exporters/export_encryption.h`.
- Export policy call sites:
  `src/exporters/{arrow_ipc,incremental,join,jsonl_llm,parquet,streaming,huggingface}_exporter.cpp`.
- Hub upload controls:
  `include/exporters/huggingface_hub_client.h`.
- Operational recovery:
  [RUNBOOK.md](./RUNBOOK.md) and [TRUST.md](./TRUST.md).
