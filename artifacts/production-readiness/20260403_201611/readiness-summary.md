# Local Production Readiness Summary

- Generated: 2026-04-03T20:19:18.3799214+02:00
- Build preset: msvc-ninja-release
- Repeat count: 20

## Gates

- **openapi-completeness-local**: PASS
  - Details: All source route hints in src/server are documented in openapi/openapi.yaml
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_201611\openapi-completeness.json
- **content-focused-local**: FAIL
  - Details: Skipped by user
- **content-processor-coverage-local**: PASS
  - Details: Processor coverage=92,31% (12/13); threshold=80,00%
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_201611\content_processor_coverage.json
- **content-benchmark-local**: PASS
  - Details: bench_content_versioning count=18 (min=18); bench_text_extraction count=20 (min=20); bench_content_processor_paths count=12 (min=12); VersionCreation1MiB=13,971ms<= 50,000ms; Diff1MiB=15,613ms<= 50,000ms; Retrieval=0,368us<= 5,000us; PDF1MiB=2,626ms<= 500,000ms; DOCX1MiB=4,968ms<= 500,000ms; HTML1MiB=82,044ms<= 500,000ms; Plain1MiB=0,321ms<= 500,000ms; OfficePath1MiB=1,319ms<= 750,000ms; OcrPath1MiB=0,689ms<= 750,000ms; ArchivePath1MiB=0,000ms<= 750,000ms
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_201611\content_bench_processor_paths.json
- **cluster-chaos-local**: FAIL
  - Details: Skipped by user
- **sla-99.99-local-proxy**: FAIL
  - Details: Skipped by user
- **pentest-report-local**: FAIL
  - Details: Skipped by user
- **beta-module-exit**: PASS
  - Details: 6 beta modules accepted via -AllowBetaModules
  - Evidence: C:\VCC\themis\artifacts\production-readiness\20260403_201611\beta_modules.txt

## Result

Failed gates: 4
