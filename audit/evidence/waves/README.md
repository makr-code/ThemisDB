# Wave Closure Package Index

This directory defines the canonical closure-package format for Wave A → B → C → D execution.

## Purpose

- Keep every open gate in one source-validated package.
- Block status promotion from documentation-only claims.
- Require reproducible references (CI run, artifacts, benchmark outputs, audit references).
- Keep the active closure backlog in `WAVE_CLOSURE_BACKLOG.md`.

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
- Required closure families are declared in `closure_manifest_policy.json`.


## Repository-tracked baseline manifests

- Canonical path: `audit/evidence/waves/manifests/`
- Initial tracked manifests:
  - `gpu_wave_a_closure_manifest.json`
  - `gpu_wave_a_closure_manifest.md`
  - `transaction_wave_b_closure_manifest.json`
  - `transaction_wave_b_closure_manifest.md`
  - `wave_a_exit_closure_manifest.json`
  - `wave_a_exit_closure_manifest.md`
  - `wave_b_module_hardening_closure_manifest.json`
  - `wave_b_module_hardening_closure_manifest.md`
  - `wave_b_integration_closure_manifest.json`
  - `wave_b_integration_closure_manifest.md`
  - `wave_ab_gap_closure_manifest.json`
  - `wave_ab_gap_closure_manifest.md`
  - `security_wave_c_closure_manifest.json`
  - `security_wave_c_closure_manifest.md`
  - `operability_wave_d_closure_manifest.json`
  - `operability_wave_d_closure_manifest.md`

These tracked manifests define the current closure baseline and are updated whenever authoritative CI run IDs and evidence artifacts are refreshed.

## Validation automation

- Validator script: `tools/ci/validate_wave_closure_packages.py`
- Unit tests: `tests/test_wave_closure_packages.py`
- CI workflow: `.github/workflows/gate-wave-closure.yml`

The validator enforces required manifest fields and strict A→B→C→D evidence-captured ordering before status promotion.
It also enforces policy-required manifest presence plus expected wave/module mapping.

## Current automated producers

- `.github/workflows/build-wave-a-gpu.yml`
- `.github/workflows/build-wave-b-transaction.yml`
- `tools/ci/generate_wave_closure_manifest.py`
