# Local Production Readiness Summary

- Generated: 2026-04-03T19:46:06.8476761+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194258\openapi-completeness.json
- **content-focused-local**: PASS
  - Details: Focused content tests; tests=4; failed=0
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194258\content_focused_ctest.log
- **content-benchmark-local**: FAIL
  - Details: Benchmark run failed (exit=-1073741515)
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194258\content_bench_run.log
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194258\beta_modules.txt

## Result

Failed gates: 4
