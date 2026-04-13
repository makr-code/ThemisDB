# Local Production Readiness TODO (No CI)

This checklist is executable locally and maps to the three open system-wide readiness gates.

## How to run

```powershell
pwsh -File scripts/operations/Invoke-LocalProductionReadiness.ps1 \
  -BuildPreset msvc-ninja-release \
  -RepeatCount 20
```

Optional full security run (requires bash + reachable target):

```powershell
pwsh -File scripts/operations/Invoke-LocalProductionReadiness.ps1 \
  -BuildPreset msvc-ninja-release \
  -RepeatCount 20 \
  -RunPentest \
  -PentestTarget "127.0.0.1:8080"
```

## Gate checklist

- [ ] Gate 0: OpenAPI completeness for server routes
  - Artifact: `artifacts/production-readiness/<timestamp>/openapi-completeness.json`
  - Pass criteria: no undocumented route hints in `src/server/**/*.cpp`

- [ ] Gate A: Cluster-level chaos/fault-injection validation passes locally
  - Artifact: `artifacts/production-readiness/<timestamp>/phase4_ctest.log`
  - Pass criteria: repeated `phase4` CTest suite has zero failed tests

- [ ] Gate B: 99.99% uptime SLA proxy passes locally
  - Artifact: `artifacts/production-readiness/<timestamp>/phase4_ctest.junit.xml`
  - Pass criteria: local pass-rate from repeated phase4 suite >= 99.99%

- [ ] Gate C: Penetration-test evidence exists
  - Artifact: `security/pentest/LOCAL_PENTEST_REPORT.md` or generated report under `security/pentest/reports/`
  - Pass criteria: report exists and no unresolved critical findings

- [ ] Gate D: Beta-module exit check is green
  - Artifact: `artifacts/production-readiness/<timestamp>/beta_modules.txt`
  - Pass criteria: no beta modules listed in `roadmap.md` (or explicit waiver documented)

## Manual follow-up (release manager)

- [ ] Archive `readiness-summary.json` and `readiness-summary.md` for the release
- [ ] Update `docs/development/CURRENT_STATUS.md` and `docs/development/DEVELOPMENT_STATUS.md` with final gate evidence
- [ ] Create release sign-off note referencing all generated artifacts
