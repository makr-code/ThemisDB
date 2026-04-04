# Local Production Readiness Summary

- Generated: 2026-04-03T20:05:42.4699364+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200148\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning count=18 (min=18); bench_text_extraction count=20 (min=20); VersionCreation1MiB=15,134ms<= 50,000ms; Diff1MiB=19,647ms<= 50,000ms; Retrieval=0,320us<= 5,000us; PDF1MiB=2,277ms<= 500,000ms; DOCX1MiB=4,152ms<= 500,000ms; HTML1MiB=93,652ms<= 500,000ms; Plain1MiB=0,221ms<= 500,000ms
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200148\content_bench_content_versioning.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200148\beta_modules.txt

## Result

Failed gates: 4
