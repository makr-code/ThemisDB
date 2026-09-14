# BSI C5 Evidence Manifests

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Scope

This directory is the canonical location for release/run-level C5 evidence manifests.

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

## Related attachments

- `SHARED_RESPONSIBILITY_MATRIX.md`
- `INCIDENT_DRILL_EVIDENCE_INDEX.md`
- `KEY_LIFECYCLE_AUDIT_EVENTS.md`
