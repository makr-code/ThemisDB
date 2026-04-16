# Local Production Readiness Summary

- Generated: 2026-04-04T08:21:39.8056817+02:00
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
  - Details: Failed to parse sharding benchmark JSON: The property 'ops_per_sec' cannot be found on this object. Verify that the property exists.
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_075047\sharding_bench_run.log
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_075047\beta_modules.txt

## Result

Failed gates: 12
