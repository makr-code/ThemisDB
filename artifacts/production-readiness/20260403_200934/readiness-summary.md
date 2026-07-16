# Local Production Readiness Summary

- Generated: 2026-04-03T20:13:29.0394870+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200934\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning count=18 (min=18); bench_text_extraction count=20 (min=20); bench_content_processor_paths count=12 (min=12); VersionCreation1MiB=14,493ms<= 50,000ms; Diff1MiB=15,077ms<= 50,000ms; Retrieval=0,317us<= 5,000us; PDF1MiB=2,201ms<= 500,000ms; DOCX1MiB=3,624ms<= 500,000ms; HTML1MiB=83,313ms<= 500,000ms; Plain1MiB=0,205ms<= 500,000ms; OfficePath1MiB=1,174ms<= 750,000ms; OcrPath1MiB=0,591ms<= 750,000ms; ArchivePath1MiB=0,000ms<= 750,000ms
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200934\content_bench_processor_paths.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_200934\beta_modules.txt

## Result

Failed gates: 4
