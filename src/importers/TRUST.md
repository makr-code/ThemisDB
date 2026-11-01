---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "src/importers/SECURITY.md; include/importers/importer_interface.h; include/importers/huggingface_ingest_plugin.h; src/importers/huggingface_ingest_plugin.cpp"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Trust Model

This page identifies trust boundaries for teams operating or extending
importers. The existing module threat model is in
[`SECURITY.md`](./SECURITY.md); public option and error surfaces are in
[`importer_interface.h`](../../include/importers/importer_interface.h).

## Boundaries and controls

| Boundary | Risk | Control and residual responsibility |
|---|---|---|
| External source to parser | Malformed, oversized, invalidly encoded, or adversarial records can consume resources or trigger conversion failures. | Use source validation, schema checks, UTF-8 and row/statement limits where supported. A zero row/statement limit means unlimited in `ImportOptions`; set a bound deliberately for untrusted data. |
| Caller to import operation | An importer may write records without a host policy check if the optional callback is omitted. | The host supplies and tests `permission_check`; a false result denies `("import", "write")`. This callback is a policy hook, not built-in identity authentication. |
| Source connector credentials | Connector configuration may contain secrets or grant broad source access. | Inject credentials through the deployment's secret mechanism, grant least privilege, redact config and logs, and rotate credentials through the owning service. Importers documentation and diagnostic artifacts must not expose them. |
| Importer to destination | Mapping or conflict policy can overwrite or misroute records. | Restrict table/schema scope, review identity mappings, select conflict behavior explicitly, and validate target state before downstream use. |
| Error, quarantine, checkpoint, or dead-letter output | Raw rows and error locations may contain personal, regulated, or proprietary data. | Store these artifacts with restricted access and retention, sanitize logs and issue reports, and preserve evidence only under the data owner's policy. |
| Optional MDM or embedding hooks | User-supplied matching, redaction, quality, and embedding callbacks execute at a data boundary. | Review callback implementation and data egress. A configured hook is not proof that redaction, model inference, or authorization is effective. |

## HuggingFace legal-data path

`HuggingFaceIngestConfig` defaults the allowed-license set to the exact,
lowercase strings `cc-by-4.0`, `mit`, and `apache-2.0`. The implementation
trims surrounding whitespace but compares case-sensitively; normalize
metadata to these spellings before import. An empty `allowed_licenses` set
accepts any license under the current implementation, and a supplied
`license_gate_hook` replaces the default set check. Do not use an empty set as
a fail-closed configuration.

PII redaction is optional and is applied only when its hook is present. It is
not a compliance guarantee. The current row-fetch methods return caller-supplied
seed/changed rows and are marked in source as a non-production adapter
scaffold; the plugin should not be treated as a live HuggingFace client.
Its embedding callback currently receives an empty vector, not model output.
See [`huggingface_ingest_plugin.cpp`](./huggingface_ingest_plugin.cpp) and
[embedding boundaries](./EMBEDDING.md).

## Incident handling

If unauthorized data, an unexpected export, or a credential exposure is
suspected:

1. Stop or cancel affected jobs through the owning service and revoke the
   narrowest implicated permission or credential.
2. Preserve sanitized error, audit, checkpoint, and destination-change
   evidence under the incident retention policy; restrict raw payload access.
3. Identify the source scope, affected destination entities, callback/provider
   egress, and whether quarantine or dead-letter files contain the same data.
4. Do not resume or replay until authorization, mapping, license, redaction,
   and idempotency conditions have been reviewed.
5. Escalate through the project security process; follow
   [module security](./SECURITY.md) and the [incident runbook](./RUNBOOK.md).

This module-level trust summary does not replace deployment IAM, network
segmentation, secret storage, destination authorization, or legal review.
