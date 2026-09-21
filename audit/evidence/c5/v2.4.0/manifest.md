# BSI C5 Evidence Manifest — v2.4.0

- Generated at: 2026-09-21T05:05:33.546203+00:00
- Workflow: `compliance-supply-chain.yml`
- Run ID: `manual-2026-09-21`
- Branch/Ref: `develop`
- Scope: `bsi-c5-2026-delta`

## C5 Control Status

| Control | Status |
| --- | --- |
| `OPS-04` | `pass` |
| `SEC-01` | `pass` |
| `IAM-02` | `pass` |
| `BCM-03` | `partial` |
| `LGM-01` | `pass` |
| `SCM-01` | `pass` |

## Evidence References

| Type | Reference |
| --- | --- |
| `sanitizer_evidence` | `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md` |
| `pentest_evidence` | `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md` |
| `key_lifecycle_events` | `audit/evidence/c5/KEY_LIFECYCLE_AUDIT_EVENTS.md` |
| `incident_drill_index` | `audit/evidence/c5/INCIDENT_DRILL_EVIDENCE_INDEX.md` |
| `sbom_supply_chain` | `.github/workflows/compliance-supply-chain.yml` |
| `shared_responsibility` | `audit/evidence/c5/SHARED_RESPONSIBILITY_MATRIX.md` |

## Notes

v2.4.0 baseline manifest. All C5 control infrastructure is in place. BCM-03 drill records pending Q4 2026 quarterly cadence.
