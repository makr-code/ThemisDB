---
Author: "ThemisDB Contributors"
Created: "2026-09-14"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# BSI C5 Shared Responsibility Matrix

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-11-09
**Status:** active

## Scope

Matrix for C5 control ownership across deployment models.

## Deployment model responsibilities

| Control Area | Self-hosted | Managed | Hybrid |
|---|---|---|---|
| Identity & access operations | Customer | Provider | Shared |
| Host and network patching | Customer | Provider | Shared |
| Application hardening (ThemisDB runtime) | Customer | Provider | Shared |
| Audit retention policy execution | Customer | Provider | Shared |
| Incident response execution | Customer | Provider | Shared |
| Backup/restore operations | Customer | Provider | Shared |
| Key rotation operations | Customer | Provider | Shared |
| SBOM/provenance publication | Customer | Provider | Shared |

## Evidence linkage rule

Each quarterly audit bundle must link this matrix together with the corresponding `audit/evidence/c5/<release>/manifest.json`.

## v2.5.0 Release-Candidate Evidence Linkage (2026-11-09)

The [candidate manifest](v2.5.0-rc-candidate/manifest.json) references the actual develop SBOM run and records the missing release-level signatures and runtime traces. It does not show deployment evidence for any model below.

| Deployment Model | Release Evidence Link | Model-Specific Deployment Evidence | Status / Remediation |
|---|---|---|---|
| Self-hosted | [v2.5.0 candidate manifest](v2.5.0-rc-candidate/manifest.json) | No signed self-hosted deployment, operation, retention, or restore evidence linked | Open; ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| Managed | [v2.5.0 candidate manifest](v2.5.0-rc-candidate/manifest.json) | No provider deployment attestation or provider-operated control evidence linked | Open; ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| Hybrid | [v2.5.0 candidate manifest](v2.5.0-rc-candidate/manifest.json) | No deployment-specific shared-operator evidence or signed hand-off records linked | Open; ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |

The shared-responsibility allocation above is a model of responsibilities, not proof that any provider or customer performed those controls.
