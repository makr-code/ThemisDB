# Local Production Readiness Summary

- Generated: 2026-04-03T19:04:38.6951201+02:00
- Build preset: msvc-ninja-release
- Repeat count: 1

## Gates

- **cluster-chaos-local**: PASS
  - Details: Phase4 slice repeated until-fail:1; tests=8; failed=0
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_190429\phase4_ctest.log
- **sla-99.99-local-proxy**: PASS
  - Details: Pass-rate=100,000% (threshold 99.99%) based on repeated phase4 suite
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_190429\phase4_ctest.junit.xml
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: FAIL
  - Details: 9 beta modules still listed in ROADMAP.md
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_190429\beta_modules.txt

## Result

Failed gates: 2
