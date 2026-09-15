---
Author: ThemisDB Maintainers
Created: 2026-09-15
Last Updated: 2026-09-15
Status: draft
---
# Wave C/D Governance Risk — CodeQL Python Skip

Status: open  
Scope: Wave C + Wave D closure governance  
Context: CodeQL Python analysis may be skipped due to database-size limits in this environment.

## Risk statement

- A CodeQL Python result of `0 alerts` is **not** sufficient evidence when the Python database was skipped.
- Until non-skipped Python scans are reproducible, Wave C/D security sign-off must treat Python CodeQL coverage as partial.

## Temporary compensating controls

- Keep fail-closed security evidence anchored in existing gates and audit bundles:
  - `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
  - `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
  - `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- Do not promote C/D status to `evidence-captured` if Python CodeQL is skipped for relevant security/governance changes.

## Follow-up implementation track

1. Run Python CodeQL incrementally by scoped change sets/modules instead of global wide scans.
2. Record run IDs and conclusions in Wave C/D manifests (`security_wave_c_closure_manifest.json`, `operability_wave_d_closure_manifest.json`).
3. Require at least one reproducible non-skipped Python CodeQL run per relevant security/governance change batch.

## Exit condition

- Risk can be closed only when Python CodeQL is reproducibly non-skipped for relevant Wave C/D closure changes and evidence is attached in manifests.
