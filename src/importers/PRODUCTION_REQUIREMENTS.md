---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/SECURITY.md; src/importers/OPERATIONS.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# Importers deployment guidance and evidence limits

This page is advisory, not a canonical product requirement or security
certification. The previous version stated source authentication, rate limiting,
fail-closed startup validation, audit guarantees, and `THEMIS_PRODUCTION_MODE` /
`THEMIS_ENVIRONMENT` switches without a confirming importer contract or current
build reference. Those statements are withdrawn; no such guarantees or variables
are established here.

## Source-backed configuration considerations

- `ImportOptions::max_row_size_bytes` and `max_statement_size_bytes` default to
  zero, which the header documents as unlimited. `import_timeout_ms` also
  defaults to zero (disabled). Set explicit bounds where the deployment requires
  them, and verify that the chosen connector consumes each option.
- `permission_check` is optional and host-supplied; the shared interface calls
  it with `("import", "write")` when present. It is not built-in authentication.
- Checkpoint, quarantine, and delta-hash paths are caller-configured. Apply
  deployment-specific access control, retention, and redaction.
- Feature availability depends on build configuration. Parquet requires
  `ARROW_ENABLED`; verify other optional connectors in their CMake/source paths.

## Operational review checklist (recommendations, not guarantees)

- [ ] Confirm source, connector, and runtime capability for the exact job.
- [ ] Set row/statement/time limits intentionally and verify connector support.
- [ ] Supply and test host authorization policy when required.
- [ ] Protect checkpoint/quarantine/delta artifacts and sanitize diagnostics.
- [ ] Validate target state, replay behavior, and recovery for the selected connector.

TODO: define any normative production/security requirements in a canonical,
reviewed contract backed by implementation and deployment evidence. See
[operations](./OPERATIONS.md), [trust model](./TRUST.md), and
[security scope](./SECURITY.md).
