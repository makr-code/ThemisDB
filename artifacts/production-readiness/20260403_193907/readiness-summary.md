# Local Production Readiness Summary

- Generated: 2026-04-03T19:42:21.5312123+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_193907\openapi-completeness.json
- **content-focused-local**: PASS
  - Details: Focused content tests; tests=4; failed=0
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_193907\content_focused_ctest.log
- **content-benchmark-local**: FAIL
  - Details: Benchmark executable not found: C:\VCC\themis\build-msvc-ninja-release\bin\bench_content_versioning.exe
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_193907\content_bench_build.log
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_193907\beta_modules.txt

## Result

Failed gates: 4
