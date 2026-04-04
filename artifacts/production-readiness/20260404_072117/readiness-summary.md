# Local Production Readiness Summary

- Generated: 2026-04-04T07:21:41.4045943+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: FAIL
  - Details: Skipped by user
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-processor-coverage-local**: FAIL
  - Details: Skipped by user
- **content-benchmark-local**: FAIL
  - Details: Skipped by user
- **geo-readiness-local**: FAIL
  - Details: Skipped by user
- **process-readiness-local**: FAIL
  - Details: Skipped by user
- **gpu-readiness-local**: PASS
  - Details: GPU benchmark: 24 entries (min=10); P2P #1800 present=True; NVLink #1802 present=True; coverage=44/30
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_072117\gpu_hardware_capability.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_072117\beta_modules.txt

## Result

Failed gates: 9
