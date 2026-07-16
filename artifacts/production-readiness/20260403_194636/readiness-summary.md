# Local Production Readiness Summary

- Generated: 2026-04-03T19:49:40.0402445+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194636\openapi-completeness.json
- **content-focused-local**: PASS
  - Details: Focused content tests; tests=4; failed=0
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194636\content_focused_ctest.log
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning executed; benchmark_count=18
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194636\content_bench_content_versioning.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_194636\beta_modules.txt

## Result

Failed gates: 3
