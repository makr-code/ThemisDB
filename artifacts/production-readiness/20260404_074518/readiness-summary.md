# Local Production Readiness Summary

- Generated: 2026-04-04T07:45:19.4584245+02:00
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
- **gpu-readiness-local**: FAIL
  - Details: Skipped by user
- **graph-readiness-local**: FAIL
  - Details: Skipped by user
- **sharding-readiness-local**: FAIL
  - Details: Sharding focused targets build failed (exit=1)
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_074518\sharding_bench_build.log
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_074518\beta_modules.txt

## Result

Failed gates: 12
