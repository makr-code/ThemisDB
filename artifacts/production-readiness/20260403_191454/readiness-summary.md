# Local Production Readiness Summary

- Generated: 2026-04-03T19:14:54.8700389+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_191454\openapi-completeness.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: FAIL
  - Details: 6 beta modules still listed in ROADMAP.md
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_191454\beta_modules.txt

## Result

Failed gates: 4
