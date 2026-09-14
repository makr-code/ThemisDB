# Wave Closure Package Index

This directory defines the canonical closure-package format for Wave A → B → C → D execution.

## Purpose

- Keep every open gate in one source-validated package.
- Block status promotion from documentation-only claims.
- Require reproducible references (CI run, artifacts, benchmark outputs, audit references).

## Required package artifacts

Each closure package must publish:

1. `*_closure_manifest.json` (machine-readable gate state)
2. `*_closure_manifest.md` (review summary)
3. CI report artifact for the run
4. Evidence artifact archive for the run

## Required manifest fields

- `generated_at`
- `wave`
- `module`
- `workflow`
- `run_id`
- `overall_status`
- `gates` (gate-id -> status)
- `evidence_references` (artifact/log/report links or names)
- `source_validation` (`code`, `tests`, `ci`, `benchmarks`)
- `notes`

## Gate policy

- Wave order is strict: **A → B → C → D**.
- A wave can move to `evidence-captured` only when all mandatory source-validation signals are `true`.
- Root roadmap/audit status updates must reference closure manifests, not checkbox-only claims.


## Repository-tracked baseline manifests

- Canonical path: `audit/evidence/waves/manifests/`
- Initial tracked manifests:
  - `gpu_wave_a_closure_manifest.json`
  - `transaction_wave_b_closure_manifest.json`
  - `security_wave_c_closure_manifest.json`
  - `operability_wave_d_closure_manifest.json`

These tracked manifests define the current closure baseline and are updated whenever authoritative CI run IDs and evidence artifacts are refreshed.

## Validation automation

- Validator script: `tools/ci/validate_wave_closure_packages.py`
- Unit tests: `tests/test_wave_closure_packages.py`
- CI workflow: `.github/workflows/14-wave-closure-governance.yml`

The validator enforces required manifest fields and strict A→B→C→D evidence-captured ordering before status promotion.

## Current automated producers

- `.github/workflows/13-wave-a-gpu-ci-execution.yml`
- `.github/workflows/13-wave-b-transaction-ci-execution.yml`
- `tools/ci/generate_wave_closure_manifest.py`
