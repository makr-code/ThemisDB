# ThemisDB Execution Board — 2026-08-09

**Scope source (canonical):**
- `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md`
- `/home/runner/work/ThemisDB/ThemisDB/FUTURE_ENHANCEMENTS.md`
- `/home/runner/work/ThemisDB/ThemisDB/NEXT_PHASE_IMPLEMENTATION_PLAN.md`

**Control mode:** sequential between phases, parallel only within active phase.

## 1) Role Assignment

| Role | Assigned Agent | Responsibility |
|---|---|---|
| Gap validation | `gap-verifier` | Validate open findings, remove false positives, prioritize remediation waves |
| Implementation | `themisdb-implementer` | Deliver production changes in batch blocks |
| Review | `themisdb-reviewer` | Findings-first correctness/security/regression review per block |
| Docs/Governance sync | `themisdb-doc-orchestrator` | Keep roadmap/release/versioning/governance docs synchronized |
| Optional cadence audit | `doc-orchestrator` | Cross-check documentation cadence and drift |

## 2) Active Phase Tracker

| Phase | Objective | Owner(s) | Status | Exit Criteria |
|---|---|---|---|---|
| Phase A | GA closure (D-11 human sign-off gate) | `themisdb-doc-orchestrator`, `themisdb-reviewer` | `[~]` | Section 9 remains explicit human-only, no governance contradictions |
| Phase B | Private plugin externalization closure | `themisdb-implementer`, `gap-verifier`, `themisdb-reviewer`, `themisdb-doc-orchestrator` | `[~]` | Plugin readiness checklist fully green |
| Phase C | LLM Wiki enterprise plugin closure | `themisdb-implementer`, `themisdb-reviewer`, `themisdb-doc-orchestrator` | `[ ]` | LWP test/perf gates pass, signed plugin verification active |
| Phase D | Remaining root backlog in waves | all | `[ ]` | Full gate package PASS per wave |

## 3) Backlog Queue (Open Canonical Items)

### Phase A (current)
- [~] Normalize GA governance texts so v2.4.0 path and D-11 human sign-off remain unambiguous across roadmap/release/versioning/sign-off docs.
- [ ] Keep Section 9 in `/home/runner/work/ThemisDB/ThemisDB/docs/governance/GA_PROMOTION_SIGN_OFF.md` as human-only approval block.

### Phase B
- [~] Align private plugin submodule branch governance with canonical branch model (`develop`, `community`, `enterprise`, `hyperscaler`, `military`) and remove legacy `main` lane references where applicable.
- [ ] Close remaining private-plugin release policy items (community no-credential guardrails, fail-closed manifest/runtime gates, commit-pin and packaging policy evidence).
- [ ] Complete synchronized governance updates in:
  - `/home/runner/work/ThemisDB/ThemisDB/BRANCHING_STRATEGY.md`
  - `/home/runner/work/ThemisDB/ThemisDB/RELEASE_STRATEGY.md`
  - `/home/runner/work/ThemisDB/ThemisDB/DOCUMENTATION_GOVERNANCE.md`
  - `/home/runner/work/ThemisDB/ThemisDB/VERSIONING.md`
  - `/home/runner/work/ThemisDB/ThemisDB/CHANGELOG.md`
  - `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md`
  - `/home/runner/work/ThemisDB/ThemisDB/FUTURE_ENHANCEMENTS.md`

### Phase C
- [ ] Freeze LLM Wiki ABI policy and config schema documentation.
- [ ] Complete error/edge-case and guardrail backlog.
- [ ] Complete Phase B (RocksDB) activation and integration/performance gates.
- [ ] Finalize operator/developer/migration docs for plugin GA.

### Phase D
- [ ] Wave 1: open release-critical `[ ]` backlog items first.
- [ ] Wave 2: high-priority production hardening.
- [ ] Wave 3: medium-term enhancements.

## 4) Gate Board

| Gate | Current State | Evidence |
|---|---|---|
| Build | `[~]` | release-critical baseline documented; re-validation required on active head before GA promotion |
| Tests | `[~]` | Wave 5/6/7/8/9 evidence referenced in roadmap/sign-off docs |
| Benchmarks | `[~]` | Wave manifests and gate reports linked in governance docs |
| Security | `[~]` | sanitizer + pentest bundles available; no new CRITICAL required per cut |
| Docs/Governance | `[~]` | cross-doc drift remediation in progress (Phase A) |

## 5) Risk Register

| ID | Risk | Impact | Mitigation | Owner | Status |
|---|---|---|---|---|---|
| R-01 | Version/governance drift across root docs | Wrong promotion decisions | enforce canonical v2.4.0 wording + explicit D-11 human-only gate | `themisdb-doc-orchestrator` | `[~]` |
| R-02 | Legacy branch naming in private submodule metadata | Governance non-conformance | migrate `main` branch references to canonical lane strategy | `themisdb-implementer` | `[~]` |
| R-03 | Private/public boundary leakage in community lanes | Security/compliance exposure | keep fail-closed checks + no-credential community policy | `themisdb-reviewer` | `[ ]` |

## 6) Evidence Index

- `/home/runner/work/ThemisDB/ThemisDB/docs/governance/GA_PROMOTION_SIGN_OFF.md`
- `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md`
- `/home/runner/work/ThemisDB/ThemisDB/NEXT_PHASE_IMPLEMENTATION_PLAN.md`
- `/home/runner/work/ThemisDB/ThemisDB/RELEASE_STRATEGY.md`
- `/home/runner/work/ThemisDB/ThemisDB/VERSIONING.md`
