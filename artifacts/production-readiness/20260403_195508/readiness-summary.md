# Local Production Readiness Summary

- Generated: 2026-04-03T19:58:18.0123412+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_195508\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning executed; benchmark_count=18 (min=18); BM_VersionCreation/1048576=15,219ms (<= 50,000ms); BM_DiffComputation/1048576=17,970ms (<= 50,000ms); BM_VersionRetrieval=0,328us (<= 5,000us)
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_195508\content_bench_content_versioning.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_195508\beta_modules.txt

## Result

Failed gates: 4
