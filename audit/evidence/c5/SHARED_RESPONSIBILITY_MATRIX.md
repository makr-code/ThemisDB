# BSI C5 Shared Responsibility Matrix

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
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
