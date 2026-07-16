# Local Production Readiness Summary

- Generated: 2026-04-04T07:20:19.2690304+02:00
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
  - Details: Failed to parse GPU benchmark JSON: The property 'Count' cannot be found on this object. Verify that the property exists.
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_071951\gpu_bench_run.log
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260404_071951\beta_modules.txt

## Result

Failed gates: 10
