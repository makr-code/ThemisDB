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

# BSI C5 Evidence Manifests

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Scope

This directory is the canonical location for release/run-level C5 evidence manifests.

## Current release candidate

The [v2.5.0 release-candidate manifest](v2.5.0-rc-candidate/manifest.json) is an evidence index assembled from actual workflow run `42321944686` and its available SBOM artifacts. The source run is on `develop`, failed overall, and skipped C5 manifest packaging; the manifest is not a v2.5.0 release record. Its [validation report](v2.5.0-rc-candidate/manifest_validation.json) confirms schema and cross-field consistency only. All controls remain partial pending release-specific runtime evidence, signatures, and human review.

The [v2.4.0 manifest](v2.4.0/manifest.json) remains historical baseline evidence and must not be copied as current release evidence.

## Required path convention

- Per release/tag: `audit/evidence/c5/<release>/manifest.json`
- CI artifact fallback (non-tag): `develop-run-<run_id>/manifest.json`

## Required manifest minimum fields

- `generated_at`
- `release_id`
- `workflow`
- `run_id`
- `ref`
- `scope`
- `control_status`
- `evidence_references`
- `notes`

## Producer

- `tools/ci/generate_c5_evidence_manifest.py`
- Workflow integration: `.github/workflows/compliance-supply-chain.yml`
- Schema/run-reference validation: `tools/ci/validate_c5_evidence_manifest.py`

## Related attachments

- `SHARED_RESPONSIBILITY_MATRIX.md`
- `INCIDENT_DRILL_EVIDENCE_INDEX.md`
- `KEY_LIFECYCLE_AUDIT_EVENTS.md`
